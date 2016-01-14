//============================================================================
// Name        : xmap_buffer_plugin.cpp
// Author      : Ulrik Kofoed Pedersen
// Version     :
// Copyright   : GPL/LGPL
// Description : AreaDetector plugin to parse XMAP buffers into NDArrays
//============================================================================
#include <epicsTypes.h>
#include <asynStandardInterfaces.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <epicsString.h>

#include "NDPluginDriver.h"
#include "xmap_buffer.h"

class XmapBufferPlugin : public NDPluginDriver {
public:
	XmapBufferPlugin(const char *portname, int queueSize,
			const char *NDArrayPort, int NDArrayAddr,
			int maxBuffers);
	void processCallbacks(NDArray *pArray);
	//virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

protected:
#define FIRST_XMAP_BUFFER_PARAM xbNumPixels
	int xbNumPixels;
	int xbSentPixels;
#define LAST_XMAP_BUFFER_PARAM xbSentPixels
#define NUM_XMAP_BUFFER_PARAM (&LAST_XMAP_BUFFER_PARAM - &FIRST_XMAP_BUFFER_PARAM + 1)

};
#define str_xbNumPixels "NUMPIXELS"
#define str_xbSentPixels "SENTPIXELS"

XmapBufferPlugin::XmapBufferPlugin(const char *portname, int queueSize,
            const char *NDArrayPort, int NDArrayAddr,
            int maxBuffers)
:NDPluginDriver(portname, queueSize, 0,
                NDArrayPort, NDArrayAddr, 1, 2,
                maxBuffers, -1,
                asynGenericPointerMask,
                asynGenericPointerMask,
                ASYN_CANBLOCK, 1, 0, 0)
{
    printf("XmapBufferPlugin: portname=%s ndarrport=%s\n", portname, NDArrayPort);
    asynStatus status;
    createParam(str_xbNumPixels, asynParamInt32, &xbNumPixels);
    createParam(str_xbSentPixels, asynParamInt32, &xbSentPixels);

    setIntegerParam( xbNumPixels, 0);
    setIntegerParam(xbSentPixels, 0);
    setStringParam(NDPluginDriverPluginType, "XmapBuffer");
    printf("Connecting to array port\n");
    status = this->connectToArrayPort();
    printf("Done with constructor\n");
}


void XmapBufferPlugin::processCallbacks(NDArray* pArray)
{
    XmapBuffer buf;
    // First call the base class method
    NDPluginDriver::processCallbacks(pArray);

    int sent_pixels = 0;
    setIntegerParam(xbSentPixels, sent_pixels);

    NDArrayInfo_t info;
    pArray->getInfo(&info);

    // Parse the full pArray buffer into the XmapBuffer,
    // taking a copy of the data along the way
    buf.parse((unsigned short*)pArray->pData, info.nElements);
    if (buf.overrun() > 0)
    {
    	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
    	          "XmapBufferPlugin: WARNING: Buffer overrun of %d detected. Readout from detector hardware is not able to keep up with datarate. Slow down!\n",buf.overrun());
    }
    //buf.report(4);

    XmapBuffer::PixelMap_t& pixelmap = buf.pixels();

    setIntegerParam(xbNumPixels, (int)pixelmap.size());
    callParamCallbacks();

    XmapBuffer::PixelMap_t::iterator it;
    XmapPixel *ptr_pix;
    NDArray *ptr_ndarr;
    int retcode = 0;
    size_t dims[2] = {0,0};

    for (it = pixelmap.begin(); it != pixelmap.end(); ++it)
    {
        ptr_pix = &(it->second);
        dims[1] = ptr_pix->num_channels();
        dims[0] = ptr_pix->data_size();
        ptr_ndarr = this->pNDArrayPool->alloc(2, dims, pArray->dataType, 0, NULL);
        if (ptr_ndarr == NULL)
        {
        	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
        			"XmapBufferPlugin: Could not allocate NDArray from the array pool. Report:\n");
        	this->pNDArrayPool->report(stdout, 2);
        	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
        			"XmapBufferPlugin: Dropping the remaining part of the pixel (%d spectrums)\n",
        			pixelmap.size() - it->first);
        	break; // Escape the for loop
        }

        // Copy the root attribute list. This may not be a good idea as the
        // same attributes will be repeated many times in the output datafile...
        // Todo: decide whether or not to make this copy
        pArray->pAttributeList->copy( ptr_ndarr->pAttributeList );

        int arrayCounter = 0;
        epicsTimeStamp timeStamp;
        epicsTimeGetCurrent(&timeStamp);
        ptr_ndarr->epicsTS = timeStamp;
        ptr_ndarr->timeStamp = timeStamp.secPastEpoch + timeStamp.nsec / 1.e9;
        setTimeStamp(&ptr_ndarr->epicsTS);
        setDoubleParam(NDTimeStamp, ptr_ndarr->timeStamp);
        setIntegerParam(NDEpicsTSSec, ptr_ndarr->epicsTS.secPastEpoch);
        setIntegerParam(NDEpicsTSNsec, ptr_ndarr->epicsTS.nsec);

        getIntegerParam(NDArrayCounter, &arrayCounter);
        // Counter is incremented by the base NDPluginDriver::processCallbacks call
        // Therefore on the first iteration we do not need to increment it here.
        if (it != pixelmap.begin()){
          arrayCounter++;
          setIntegerParam(NDArrayCounter, arrayCounter);
        }
        ptr_ndarr->uniqueId = arrayCounter;
        setIntegerParam(NDUniqueId, pArray->uniqueId);

        retcode = ptr_pix->copy_data(ptr_ndarr);
        if (retcode <= 0) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            		"XmapBufferPlugin: no bytes copied to NDArray: %d\n",
            		retcode);
        }

        this->unlock();
        doCallbacksGenericPointer(ptr_ndarr, NDArrayData, 0);
        this->lock();
        // decrement the reference counter in the array that was just sent on the queues
        ptr_ndarr->release();

        sent_pixels++;
        setIntegerParam(xbSentPixels, sent_pixels);
        callParamCallbacks();
    }

}



extern "C" int XmapBufferConfigure(const char *portname, int queueSize,
		    const char *NDArrayPort, int NDArrayAddr,
            int maxBuffers)
{
    printf("XmapBufferConfigure: portname: %s ndarrport: %s\n", portname, NDArrayPort);
    XmapBufferPlugin* ptr = new XmapBufferPlugin(portname, queueSize,
            NDArrayPort, NDArrayAddr, maxBuffers);
    printf("XmapBufferConfigure done!\n");
    ptr = NULL;
    return (asynSuccess);
}
/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg3 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg4 = { "maxBuffers",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4};
static const iocshFuncDef initFuncDef = {"XmapBufferConfigure", 5, initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    printf("initCallFunc: portname: %s ndarrpot: %s\n", args[0].sval, args[2].sval);
    XmapBufferConfigure(epicsStrDup( args[0].sval ), args[1].ival,
                        epicsStrDup( args[2].sval ), args[3].ival, args[4].ival);
}

extern "C" void XmapBufferRegister(void)
{
    iocshRegister(&initFuncDef, initCallFunc);
}

extern "C" {
epicsExportRegistrar( XmapBufferRegister );
}
