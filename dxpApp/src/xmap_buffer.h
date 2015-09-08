//============================================================================
// Name        : xmap_buffer.cpp
// Author      : Ulrik Kofoed Pedersen
// Version     :
// Copyright   : GPL/LGPL
// Description : XMAP buffer parser.
//============================================================================

#ifndef XMAP_BUFFER_H_
#define XMAP_BUFFER_H_

#include <iostream>
#include <vector>
#include <map>
//#include "NDArray.h"

// forward declaration
class NDArray;

// Use reference counted pointers
// See how to include this: sharedPtr.h in /dls_sw/work/R3.14.11/support/pvDataCPP/include/pv
//#include <memory>
//#include <tr1/memory>

using namespace std;


#define MAX_CHANNELS_PER_CARD  4
#define MAPPING_CLOCK_PERIOD   320e-9
#define MEGABYTE               1048576
#define MAX_HEADER_SIZE        256
#define OFFSET_STATS_START     32
#define NUM_STATS_WORDS        8


enum MappingMode_t {no_mode = 0, full_spectrum=1, multiple_sca, standard_list, sparse_list};

typedef struct XmapStats {
	// statistics
	double real_time;
	double trigger_live_time;
	unsigned int triggers;
	unsigned int events;

	// calculated statistics
	double energy_live_time;
	double icr; // input count rate
	double ocr; // output count rate

}XmapStats;

typedef struct ChannelInfo_t {
	unsigned short detector_channel;
	unsigned short detector_element;
	unsigned short channel_size;
}ChannelInfo_t;


class XmapChannelData {
public:
	XmapChannelData():offset_data(MAX_HEADER_SIZE),offset_statistics(OFFSET_STATS_START){};
	virtual ~XmapChannelData(){};
	virtual void report(int level);
	virtual int parse(unsigned short *pixel_buf, unsigned int buflen, unsigned int channel);
	virtual int copy_data(unsigned short * dest, size_t max_elements){return -1;};
	virtual size_t data_size(){return 0;}; // Number of data elements in dataset
	unsigned int channel_id;    // The channel ID on system [0..n]
	unsigned int card_ch_index; // the channel index on card [0..3]

	XmapStats stats;
protected:
	unsigned short num_elements;
	unsigned int offset_data;
	unsigned int offset_statistics;
};

class Spectrum : public XmapChannelData {
public:
	Spectrum(){this->offset_data = MAX_HEADER_SIZE;};
	virtual ~Spectrum(){};
	virtual int parse(unsigned short *pixel_buf, unsigned int buflen, unsigned int channel);
	virtual void report(int level);
	int copy_data(unsigned short * dest, size_t max_elements);
	size_t data_size();
private:
	vector <unsigned short> spectrum_data;
};

class XmapPixel {
public:
	XmapPixel():pixel_id(0),mapping_mode(no_mode){/*printf("--new pixel--\n");*/};
	~XmapPixel();
	static unsigned int get_pixel_id(unsigned short *pixel_buf);
	unsigned int get_pixel_id();
	int parse (unsigned short *pixel_buf, unsigned int buflen, ChannelInfo_t chinfo[MAX_CHANNELS_PER_CARD]);
	void report(int level);
    // just using regular pointers for the moment as we dont have boost on
    // Windows XP development PC, build server or target PC.
    typedef map <unsigned int, XmapChannelData*> ChannelMap_t;
    XmapPixel::ChannelMap_t& channels() {return this->_channels;};

    int copy_data(NDArray* dest);
    int num_channels();
    size_t data_size();
    unsigned int num_datapoints; // Number of datapoints recorded in this pixel. Should normally be 1 if HW/SW can keep up.
                                 // However, if acquisition is too fast for readout, multiple datapoints may be recorded in the last pixel in the buffer.

protected:
	unsigned int pixel_id;
	MappingMode_t mapping_mode;

	// if using clever reference counted shared pointers...
	//typedef map < unsigned int, tr1::shared_ptr<XmapChannelData> > ChannelMap_t;


	// map of channels with data
	ChannelMap_t _channels;

private:
	int parse_stats();

	static struct offsets {
		static const unsigned int pixel_header_size = 2;
		static const unsigned int mapping_mode = 3;
		static const unsigned int pixel_number = 4;
		static const unsigned int pixel_block_size = 6;
		static const unsigned int channels_num_elements = 8;
		static const unsigned int channels_stats = 32;
	}offsets;
};

typedef struct VecStats {
    vector<double> real_time;
    vector<double> trigger_live_time;
    vector<unsigned int> triggers;
    vector<unsigned int> events;
    vector<double> energy_live_time;
    vector<double> icr; // input count rate
    vector<double> ocr; // output count rate
}VecStats;

class XmapBuffer {
public:
	XmapBuffer():num_buffers(0),bufptr(NULL){};
	~XmapBuffer();
	int parse(unsigned short *xmap_buf, unsigned int buflen);
	void report(int level);

	typedef map <unsigned int, XmapPixel> PixelMap_t;
	XmapBuffer::PixelMap_t& pixels(){return this->_pixels;};

	int statistics( VecStats* vecstats, unsigned int channel );
	unsigned short overrun();

protected:
	unsigned int num_buffers;

	// map of pixels (index, pixel)
	PixelMap_t _pixels;

private:
	unsigned short *bufptr;
	static const unsigned short BUF_TAG_0 = 0x55AA;
	static const unsigned short BUF_TAG_1 = 0xAA55;
	static const unsigned short PIX_TAG_0 = 0x33CC;
	static const unsigned short PIX_TAG_1 = 0xCC33;

	static struct offsets {
		static const unsigned int buffer_header_size = 2;
		static const unsigned int mapping_mode = 3;
		static const unsigned int run_number = 4;
		static const unsigned int seq_buffer = 5;
		static const unsigned int buf_ab = 7;
		static const unsigned int num_pixels = 8;
		static const unsigned int start_pixel = 9;
		static const unsigned int det_info_start = 12;
		static const unsigned int buffer_overrun = 24;
	}offsets;

};


#endif /* XMAP_BUFFER_H_ */
