//============================================================================
// Name        : xmap_buffer.cpp
// Author      : Ulrik Kofoed Pedersen
// Version     :
// Copyright   : GPL/LGPL
// Description : XMAP buffer parser.
//============================================================================

#include <fstream>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

#include "NDArray.h"
#include "xmap_buffer.h"

//============================================================================
//
//     Spectrum class implementation.
//         - inheritance from the XmapChannelData class
//
int Spectrum::parse(unsigned short *pixel_buf, unsigned int buflen, unsigned int channel)
{
	int retcode = 0;
	unsigned int ch = 0;
	unsigned int offset = 0;
	unsigned int num_data_words_total = 0;
	unsigned short num_elements[MAX_CHANNELS_PER_CARD];


	// first call parent class to do basic parsing
	retcode = XmapChannelData::parse(pixel_buf, buflen, channel);
	if (retcode != 0) return retcode;

	//printf("  Spectrum::parse: pixel_buf=%p buflen=%d channel=%d\n", pixel_buf, buflen, channel);

	// sanity check if the pixel buffer contains enough elements to supply the data
	num_data_words_total = this->offset_data;
	for (ch = 0; ch<MAX_CHANNELS_PER_CARD; ch++)
	{
		num_data_words_total += pixel_buf[8 + ch];
		num_elements[ch] = pixel_buf[8+ch];
	}
	if (num_data_words_total-1 > buflen){
		fprintf(stderr, "Spectrum::parse: ERROR: buffer is not long enough to contain reported data (%d < %d)\n",
				num_data_words_total, buflen);
		retcode = -1;
		return retcode;
	}

	this->num_elements = num_elements[this->card_ch_index];

	// Calculate the offset of this channels spectrum data
	offset = this->offset_data;
	for (ch = 0; ch < this->card_ch_index; ch++)
	{
		offset += num_elements[ch];
	}

	unsigned short *ptr = (pixel_buf+offset);
	// take a copy of the data in the pixel_buf into our spectrum data vector
	this->spectrum_data.assign(ptr, (ptr + num_elements[this->card_ch_index]));

	return retcode;
}

int Spectrum::copy_data(unsigned short *dest, size_t max_elements)
{

    size_t copy_elements = this->spectrum_data.size();
    if (copy_elements > max_elements) copy_elements = max_elements;
    //printf("Spectrum::copy_data: dest=%p, copyelements=%d maxelements=%d )\n", dest, copy_elements, max_elements);

    vector<unsigned short>::const_iterator begin = this->spectrum_data.begin();
    copy( begin, begin + copy_elements, dest );
    return copy_elements;
}

void Spectrum::report(int level)
{
	// call base class report first
	XmapChannelData::report(level);

	if (level < 2) {printf("\n"); return;}

	printf("    %5d\n", this->spectrum_data.size());

}

size_t Spectrum::data_size()
{
    return this->spectrum_data.size();
}

//============================================================================
//
//     XmapChannelData base class implementation.
//

int XmapChannelData::parse(unsigned short *pixel_buf, unsigned int buflen, unsigned int channel)
{
	int retcode = 0;
	unsigned int offset;
	this->channel_id = channel;
	this->card_ch_index = channel%MAX_CHANNELS_PER_CARD;

	//printf("  XmapChannelData::parse: pixel_buf=%p buflen=%d channel=%d(%d)\n", pixel_buf, buflen, channel, this->card_ch_index);

	// parse the statistics
	offset = this->offset_statistics + (this->card_ch_index * NUM_STATS_WORDS);
	this->stats.real_time = (pixel_buf[offset] + (pixel_buf[offset+1]<<16)) * MAPPING_CLOCK_PERIOD;
	offset += 2;
	this->stats.trigger_live_time = (pixel_buf[offset] + (pixel_buf[offset+1]<<16)) * MAPPING_CLOCK_PERIOD;
	offset += 2;
	this->stats.triggers = (pixel_buf[offset] + (pixel_buf[offset+1]<<16));
	offset += 2;
	this->stats.events = (pixel_buf[offset] + (pixel_buf[offset+1]<<16));

	// calculate extra statistics same way as it's done in the dxp module
	if (this->stats.triggers > 0.)
		this->stats.energy_live_time = (this->stats.trigger_live_time * this->stats.events) / this->stats.triggers;
	else
		this->stats.energy_live_time = this->stats.trigger_live_time;
	if (this->stats.trigger_live_time > 0.)
		this->stats.icr = this->stats.triggers / this->stats.trigger_live_time;
	else
		this->stats.icr = 0.;
	if (this->stats.real_time > 0.)
		this->stats.ocr = this->stats.events / this->stats.real_time;
	else
		this->stats.ocr = 0.;

	return retcode;
}


void XmapChannelData::report(int level)
{
	if (level < 1) return;
	printf("  %3d    %5d    %.4f    %.4f    %10d    %10d",
	       this->channel_id, this->num_elements,
	       this->stats.real_time, this->stats.trigger_live_time,
	       this->stats.triggers, this->stats.events);

}



//============================================================================
//
//     XmapPixel class implementation.
//


int XmapPixel::parse (unsigned short *pixel_buf, unsigned int buflen, ChannelInfo_t chinfo[MAX_CHANNELS_PER_CARD])
{
	int retcode = 0;
	int offset = 0;
	unsigned int blocksize = 0;


	if (buflen < MAX_HEADER_SIZE) {
		retcode = -1;
		return retcode;
	}

	offset = this->offsets.pixel_number;
	this->pixel_id = pixel_buf[offset] + (pixel_buf[offset+1] << 16);

	//printf(" XmapPixel::parse: pixel_buf=%p buflen=%d id=%d\n", pixel_buf, buflen, this->pixel_id);

	offset = this->offsets.mapping_mode;
	this->mapping_mode = (MappingMode_t) pixel_buf[offset];

	offset = this->offsets.pixel_block_size;
	blocksize = pixel_buf[offset] + (pixel_buf[offset+1] << 16);

	if (buflen < blocksize)
	{
		fprintf(stderr, "XmapPixel::parse: ERROR: buflen smaller than reported blocksize (%d<%d)\n",buflen, blocksize);
		retcode = -1;
		for (int i=0; i<100; i++) printf("%6d  0x%04X\n", pixel_buf[i], pixel_buf[i]);

		return retcode;
	}

	for (unsigned int ch = 0; ch < MAX_CHANNELS_PER_CARD; ch++)
	{
		unsigned int ch_id = chinfo[ch].detector_element;

		// Check if Channel Data is already available for this channel
		// -and to remove memory leak, delete the existing data channel
		// before overwriting with a new one (thanks valgrind!!!)
		if (this->_channels.find(ch_id) != this->_channels.end()) {
			fprintf(stderr, "XmapPixel::parse: WARNING: channel %d already exist in pixel: %d\n", ch_id, this->pixel_id);
			continue; // TODO: hack because it seems when a buffer is not written fully it has old crap left in it....
			delete this->_channels[ch_id];
			this->_channels.erase(ch_id);
		}

		switch (this->mapping_mode){
		case full_spectrum:
			// If using reference counted pointers...
			//this->channels[ch_id] = tr1::shared_ptr<XmapChannelData> (new Spectrum());

			// just using regular pointers for the moment as we dont have boost on
			// Windows XP development PC, build server or target PC.
			this->_channels[ch_id] = new Spectrum();
			break;
		default:
			fprintf(stderr, "XmapPixel::parse: ERROR: mapping mode: %d is not supported\n", (int)this->mapping_mode);
			retcode = -1;
			return retcode;
		}

		this->_channels[ch_id]->parse(pixel_buf, buflen, ch_id);
	}

	return retcode;
}


void XmapPixel::report(int level)
{
	if (level < 1) return;

	//printf("  ==== XmapPixel report ====\n");
	printf("-- %06d ---------------------------------------------------------------\n", this->pixel_id);
	//printf("  Number of channels: %d\n", this->_channels.size());

	// report on each channel
	ChannelMap_t::iterator it;
	for(it = this->_channels.begin(); it != this->_channels.end(); ++it)
	{
		it->second->report(level-1);
	}
}

unsigned int XmapPixel::get_pixel_id(unsigned short *pixel_buf)
{
	unsigned short pix_id = 0;
	unsigned int offset = XmapPixel::offsets.pixel_number;
	pix_id = pixel_buf[offset] + (pixel_buf[offset+1] << 16);
	return pix_id;
}

unsigned int XmapPixel::get_pixel_id()
{
	return this->pixel_id;
}

XmapPixel::~XmapPixel()
{
	// Delete all the items of the channels map
	// Note that this can be discarded if switching to use clever reference counted pointers!
	//printf("pixel %d deleting %d channels\n", this->pixel_id, this->_channels.size());
	ChannelMap_t::iterator it;
	for(it = this->_channels.begin(); it != this->_channels.end(); ++it)
	{
	    if (it->second != NULL) {
	        delete it->second;
	        this->_channels[it->first] = NULL;
	    }
	}
	this->_channels.clear();
}

int XmapPixel::num_channels()
{
    return this->_channels.size();
}

size_t XmapPixel::data_size()
{
    size_t max_size = 0;
    ChannelMap_t::iterator it;
    for(it = this->_channels.begin(); it != this->_channels.end(); ++it)
    {
        max_size = max(max_size, it->second->data_size());
    }
    return max_size;
}

int XmapPixel::copy_data(NDArray * dest)
{

    NDArrayInfo_t info;
    dest->getInfo(&info);
    size_t max_elements = dest->dataSize / info.bytesPerElement;
    //printf("XmapPixel::copy_data: pdata=%p max_elements=%d bytesPerElement=%d\n",
    //       dest->pData, max_elements, info.bytesPerElement);

    XmapPixel::ChannelMap_t::const_iterator it;
    size_t num_elements = 0;
    NDAttributeList *pattrlist = NULL;
    int ret=0;
    unsigned short* pdata = (unsigned short*)dest->pData;
    string attr_name;
    string attr_desc;
    char * str_ch_id = new char(10);

    // Check whether the spectrum data for each channel will fit in the destination NDArray
    size_t spectrumbytes = 0;
    for (it = this->_channels.begin(); it != this->_channels.end(); ++it)
    {
    	Spectrum* chdata = dynamic_cast<Spectrum*>(it->second);
    	spectrumbytes += (chdata->data_size() * sizeof(unsigned short));
    }
    if (spectrumbytes > (size_t)dest->dataSize)
    {
    	printf("XmapPixel:copy_data: WARNING: NDArray destination does not have enough space for spectrum data\n");
    	printf("                     spectrumbytes=%d dst=%p size=%d\n", spectrumbytes, dest, dest->dataSize);
    	return -1;
    }

    attr_name = "num_datapoints";
    attr_desc = "Number recorded datapoints in this pixel";
    attr_desc += " [count]";
    dest->pAttributeList->add(attr_name.c_str(), attr_desc.c_str(),
                   NDAttrUInt32, &this->num_datapoints);

    for (it = this->_channels.begin(); it != this->_channels.end(); ++it)
    {
        Spectrum* chdata = dynamic_cast<Spectrum*>(it->second);
        pdata = (unsigned short*)dest->pData + num_elements;

        ret = chdata->copy_data(pdata, max_elements - num_elements);
        if (ret < 0) {
            printf("XmapPixel::copy_data: Warning unable to copy data...");
            //delete str_ch_id;
            break;
        }
        num_elements += ret;

        // Write the statistics as NDAttributes in the NDArray
        pattrlist = dest->pAttributeList;
        if (pattrlist != NULL)
        {
            //printf("XmapPixel::copy_data: ch:%d copying statistics as NDAttributes\n", it->first);
            sprintf(str_ch_id, "%d", it->first);
            //snprintf(str_ch_id, 10, "%d", it->first);

            attr_name = "real_time_ch_";
            attr_name += str_ch_id;
            attr_desc = "Real time channel ";
            attr_desc += str_ch_id;
            attr_desc += " [seconds]";
            pattrlist->add(attr_name.c_str(), attr_desc.c_str(),
                           NDAttrFloat64, &chdata->stats.real_time);

            attr_name = "trigger_live_time_ch_";
            attr_name += str_ch_id;
            attr_desc = "Trigger live time channel ";
            attr_desc += str_ch_id;
            attr_desc += " [seconds]";
            pattrlist->add(attr_name.c_str(), attr_desc.c_str(),
                           NDAttrFloat64, &chdata->stats.trigger_live_time);

            attr_name = "triggers_ch_";
            attr_name += str_ch_id;
            attr_desc = "Number of triggers ";
            attr_desc += str_ch_id;
            attr_desc += " [count]";
            pattrlist->add(attr_name.c_str(), attr_desc.c_str(),
                           NDAttrUInt32, &chdata->stats.triggers);

            attr_name = "events_ch_";
            attr_name += str_ch_id;
            attr_desc = "Number of events ";
            attr_desc += str_ch_id;
            attr_desc += " [count]";
            pattrlist->add(attr_name.c_str(), attr_desc.c_str(),
                           NDAttrUInt32, &chdata->stats.events);

            attr_name = "energy_live_time_ch_";
            attr_name += str_ch_id;
            attr_desc = "Energy live time channel ";
            attr_desc += str_ch_id;
            attr_desc += " [seconds]";
            pattrlist->add(attr_name.c_str(), attr_desc.c_str(),
                           NDAttrFloat64, &chdata->stats.energy_live_time);

            attr_name = "icr_ch_";
            attr_name += str_ch_id;
            attr_desc = "Input count rate channel ";
            attr_desc += str_ch_id;
            attr_desc += " [Hz]";
            pattrlist->add(attr_name.c_str(), attr_desc.c_str(),
                           NDAttrFloat64, &chdata->stats.icr);

            attr_name = "ocr_ch_";
            attr_name += str_ch_id;
            attr_desc = "Output count rate channel ";
            attr_desc += str_ch_id;
            attr_desc += " [Hz]";
            pattrlist->add(attr_name.c_str(), attr_desc.c_str(),
                           NDAttrFloat64, &chdata->stats.ocr);


/*          printf( "Real time channel         =%f\n", chdata->stats.real_time );
            printf( "Trigger live time channel =%f\n", chdata->stats.trigger_live_time );
            printf( "Number of triggers        =%d\n", chdata->stats.triggers );
            printf( "Number of events          =%d\n", chdata->stats.events );
            printf( "Energy live time channel  =%f\n", chdata->stats.energy_live_time );
            printf( "Input count rate channel  =%f\n", chdata->stats.icr );
            printf( "Output count rate channel =%f\n", chdata->stats.ocr );*/
        }
    }
    delete str_ch_id;

    return num_elements;
}

//============================================================================
//
//     XmapBuffer class implementation.
//

XmapBuffer::~XmapBuffer(){}

int XmapBuffer::statistics( VecStats* vecstats, unsigned int channel )
{
    PixelMap_t::iterator it;
    XmapStats stats;

    // Sanity check: do we have any pixels at all?
    if (this->_pixels.size() < 1) return -1;
    // Sanity check: do we have any data on the requested channel?
    if (this->_pixels[0].channels().count(channel) == 0) return -1;

    // Run through all pixels and append the various statistics fields
    // to the vectors which are to be returned
    for (it = this->_pixels.begin(); it != this->_pixels.end(); ++it)
    {
        stats = it->second.channels()[channel]->stats;
        vecstats->energy_live_time.push_back( stats.energy_live_time );
        vecstats->events.push_back(stats.events);
        vecstats->icr.push_back( stats.icr );
        vecstats->ocr.push_back( stats.ocr );
        vecstats->real_time.push_back( stats.real_time );
        vecstats->trigger_live_time.push_back( stats.trigger_live_time );
        vecstats->triggers.push_back( stats.triggers );
    }
    return 0;
}

unsigned short XmapBuffer::overrun()
{
	return this->bufptr[this->offsets.buffer_overrun];
}

void XmapBuffer::report(int level)
{
	if (level < 1) return;
	printf("==== XmapBuffer report ====\n");
	printf("Number of buffers parsed: %d\n", this->num_buffers);
	printf("Number of pixels: %d\n", this->_pixels.size());

	// print report for each pixel
	PixelMap_t::iterator it;
	printf("  ch_id  # bins    RT      trig LT       triggers        events\n");
	for (it = this->_pixels.begin(); it != this->_pixels.end(); ++it)
	{
		it->second.report(level-1);
	}
}

int XmapBuffer::parse(unsigned short *xmap_buf, unsigned int buflen)
{
	int retcode = -1;
	unsigned int i = 0;
	unsigned int ch = 0;
	unsigned short *ptr = NULL;

	//printf("XmapBuffer::parse: xmap_buf=%p buflen=%d\n", xmap_buf, buflen);

	// search for the buffer header tags
	for (i = 0; (i < buflen-MAX_HEADER_SIZE) && (retcode != 0); i++)
	{
		retcode = i;
		if ( (xmap_buf[i]   == this->BUF_TAG_0) &&
			 (xmap_buf[i+1] == this->BUF_TAG_1) )
		{
			// found start of header
			this->bufptr = xmap_buf+i;
			//for (int i=0; i<25; i++) printf("0x%p %6d  0x%04X\n", this->bufptr, this->bufptr[i], this->bufptr[i]);
			retcode = 0;
		}
	}
	// return the buffer count if we did not find tags
	if (retcode > 0) return retcode;

	// Fill in the information regarding each channel from the buffer header
	ChannelInfo_t chinfo[MAX_CHANNELS_PER_CARD];
	ptr = xmap_buf+i-1;
	unsigned short max_pix_data_len = MAX_HEADER_SIZE;
	for (ch = 0; ch < MAX_CHANNELS_PER_CARD; ch++)
	{
		int offset = this->offsets.det_info_start;
		chinfo[ch].detector_channel = ptr[offset + (ch * 2) + 0];
		chinfo[ch].detector_element = ptr[offset + (ch * 2) + 1];
		offset += MAX_CHANNELS_PER_CARD * 2;
		chinfo[ch].channel_size = ptr[offset + ch];
		//max_pix_data_len += chinfo[ch].channel_size;
	}

	// search for pixel tags or end of buffer
	unsigned int prevPixId = XmapPixel::get_pixel_id(xmap_buf + MAX_HEADER_SIZE);;
	for (i = 0; i <= buflen-max_pix_data_len; i++)
	{
		retcode = i;

		// check for pixel tag
		if ((xmap_buf[i]   == this->PIX_TAG_0) &&
			(xmap_buf[i+1] == this->PIX_TAG_1))
		{
			unsigned short *pix_buf = xmap_buf + i;
			unsigned int pix_id = XmapPixel::get_pixel_id(pix_buf);
			//printf("XmapBuffer::parse: found pix tag at offset %d id=%d\n", i, pix_id);

			// the map [] operator has the interesting feature of creating the requested key using default constructor
			// if the key does not already exist. So the following line simply runs the pixel parser on either a new
			// XmapPixel object or an old one if the key already exist...
			this->_pixels[pix_id].parse(pix_buf, buflen-i, chinfo);
			this->_pixels[pix_id].num_datapoints = pix_id - prevPixId;
			if (pix_id == prevPixId) this->_pixels[pix_id].num_datapoints = 1;
			prevPixId = pix_id;
		}
	}

	// finally increment counter
	this->num_buffers++;
	return retcode;
}
