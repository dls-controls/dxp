//============================================================================
// Name        : test_xmap_buffer.cpp
// Author      : Ulrik Kofoed Pedersen
// Version     :
// Copyright   : GPL/LGPL
// Description : Simple test application to verify the operation of the
//               XMAP buffer parser classes.
//============================================================================
#include <iostream>
#include <fstream>
#include <cstdlib>

#include "NDArray.h"
#include "xmap_buffer.h"

// first a couple of hand-crafted test buffers
unsigned short testbuf1[] = {
		// buffer header
		21930,
		43605,
		256,
		1,
		30752,
		0,
		0,
		0,
		2, // number of pixels
		0,
		0,
		0,
		0,0,
		1,1,
		2,2,
		3,3,
		6, 6, 6, 6, // channel sizes

		0,0,0,0,0, // unused/reserved

		// 1st pixel header
		13260,52275, // tags
		256, // header size
		1, //mapping mode
		0,0, // pixel number
		280,0, // block size
		6,6,6,6, // data size
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // unused
		49108,4, // ch0 realtime
		47776,4, // ch0 livetime
		0,0,  // ch0 triggers
		1277,0,  // ch0 output events
		49108,4, // ch1 realtime
		47966,4, // ch1 livetime
		1,0,  // ch1 triggers
		1071,0,  // ch1 output events
		49108,4, // ch2 realtime
		47569,4, // ch2 livetime
		2,0,  // ch2 triggers
		1277,0,  // ch2 output events
		49107,4, // ch3 realtime
		47714,4, // ch3 livetime
		3,0,  // ch3 triggers
		1278,0,  // ch3 output events
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // unused
		0,1,2,3,4,5,        //ch0 spectrum
		6,7,8,9,10,11,      //ch1 spectrum
		12,13,14,15,16,17,  //ch2 spectrum
		18,19,20,21,22,23,  //ch3 spectrum

		// 2nd pixel header
		13260,52275, // tags
		256, // header size
		1, //mapping mode
		1,0, // pixel number
		280,0, // block size
		6,6,6,6, // data size
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // unused
		43108,4, // ch0 realtime
		43776,4, // ch0 livetime
		100,0,  // ch0 triggers
		1377,0,  // ch0 output events
		43108,4, // ch1 realtime
		43966,4, // ch1 livetime
		200,0,  // ch1 triggers
		1371,0,  // ch1 output events
		43108,4, // ch2 realtime
		43569,4, // ch2 livetime
		300,0,  // ch2 triggers
		1377,0,  // ch2 output events
		43107,4, // ch3 realtime
		43714,4, // ch3 livetime
		400,0,  // ch3 triggers
		1378,0,  // ch3 output events
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // unused
		10,11,12,13,14,15,        //ch0 spectrum
		16,17,18,19,110,111,      //ch1 spectrum
		112,113,114,115,116,117,  //ch2 spectrum
		118,119,210,211,212,123  //ch3 spectrum

};

unsigned short testbuf2[] = {
		// buffer header
		21930,
		43605,
		256,
		1,
		30752,
		0,
		0,
		0,
		2, // number of pixels
		0,
		0,
		0,
		0,4,
		1,5,
		2,6,
		3,7,
		6, 6, 6, 6, // channel sizes

		0,0,0,0,0, // unused/reserved

		// pixel header
		13260,52275, // tags
		256, // header size
		1, //mapping mode
		0,0, // pixel number
		280,0, // block size
		6,6,6,6, // data size
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // unused
		49108,4, // ch0 realtime
		47776,4, // ch0 livetime
		400,0,  // ch0 triggers
		1277,0,  // ch0 output events
		49108,4, // ch1 realtime
		47966,4, // ch1 livetime
		500,0,  // ch1 triggers
		1071,0,  // ch1 output events
		49108,4, // ch2 realtime
		47569,4, // ch2 livetime
		600,0,  // ch2 triggers
		1277,0,  // ch2 output events
		49107,4, // ch3 realtime
		47714,4, // ch3 livetime
		700,0,  // ch3 triggers
		1278,0,  // ch3 output events
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // unused
		0,1,2,3,4,5,        //ch0 spectrum
		6,7,8,9,10,11,      //ch1 spectrum
		12,13,14,15,16,17,  //ch2 spectrum
		18,19,20,21,22,23,  //ch3 spectrum

		// 2nd pixel header
		13260,52275, // tags
		256, // header size
		1, //mapping mode
		1,0, // pixel number
		280,0, // block size
		6,6,6,6, // data size
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // unused
		43108,4, // ch0 realtime
		43776,4, // ch0 livetime
		401,0,  // ch0 triggers
		1377,0,  // ch0 output events
		43108,4, // ch1 realtime
		43966,4, // ch1 livetime
		501,0,  // ch1 triggers
		1371,0,  // ch1 output events
		43108,4, // ch2 realtime
		43569,4, // ch2 livetime
		601,0,  // ch2 triggers
		1377,0,  // ch2 output events
		43107,4, // ch3 realtime
		43714,4, // ch3 livetime
		701,0,  // ch3 triggers
		1378,0,  // ch3 output events
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // unused
		10,11,12,13,14,15,        //ch0 spectrum
		16,17,18,19,110,111,      //ch1 spectrum
		112,113,114,115,116,117,  //ch2 spectrum
		118,119,210,211,212,123  //ch3 spectrum
};

/** Load a dataset from a text file where each line represent a 16bit word from the XMAP buffer
 * The destination pointer dest must be pre-allocated with dest_len number of 16bit words
 * before calling this function.
 */
int load_dataset_txt(const char * filename, unsigned short *dest, unsigned int dest_len)
{
	int retcode = 0;
	string line;
	ifstream datafile (filename);
	unsigned int i=0;
	if (datafile.is_open())
	{
		while ( datafile.good() && (i<dest_len) )
		{
			getline (datafile,line);
			//cout << line << endl;
			dest[i] = strtoul(line.c_str(), NULL, 0);
			i++;
		}
		datafile.close();
		retcode = i;
	}
	return retcode;
}

int main() {
	int retcode = 0;
	cout << "XMAP buffer parser!" << endl; // prints !!!Hello World!!!

	unsigned short *testfilebuf = NULL;
	unsigned int testfilelen = 1024*1024;
	testfilebuf = (unsigned short*)calloc(testfilelen, sizeof(unsigned short));
	retcode = load_dataset_txt("/home/up45/sandbox/data1.txt", testfilebuf, testfilelen);
	printf("retcode: %d\n", retcode);
	for (int i=0; i<10; i++) printf("%6d  0x%04X\n", testfilebuf[i], testfilebuf[i]);


	XmapBuffer buf;
	printf("Parsing data1.txt....\n");
	buf.parse(testfilebuf, testfilelen);
	free(testfilebuf);
	//buf.report(4);

	//return 0;


	printf("Parsing testbuf1....\n");
	buf.parse(testbuf1, sizeof(testbuf1)/2);
	printf("Parsing testbuf2...\n");
	buf.parse(testbuf2, sizeof(testbuf2)/2);

	buf.report(2);

	printf("Getting pixelmap reference\n");
	XmapBuffer::PixelMap_t& pm = buf.pixels();

	printf("pixel0 report:\n");
	pm[0].report(2);

	printf("Testing copy\n");
	unsigned short dest[3] = {0,0,0};
	XmapChannelData *xcd = pm[0].channels()[0];
	Spectrum* sp;
	sp = dynamic_cast<Spectrum*> (xcd);
	sp->report(2);
	sp->copy_data(dest, 3);
	printf("dest: %d %d %d\n", dest[0], dest[1], dest[2]);

	printf("Testing copy to NDArray\n");
	NDArray ndarr;
	ndarr.dataSize = 500;
	ndarr.dataType = NDUInt16;
	ndarr.ndims = 2;
	ndarr.initDimension(ndarr.dims+0, (int)pm[0].num_channels());
	ndarr.initDimension(ndarr.dims+1, pm[0].data_size());
	ndarr.pData = calloc(ndarr.dataSize, sizeof(char));

	pm[0].copy_data( &ndarr );
	ndarr.report(stdout, 11);

	return 0;
}
