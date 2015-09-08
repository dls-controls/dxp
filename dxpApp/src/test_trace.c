#include "handel.h"
#include <epicsThread.h>

#define MAX_ITERATIONS 100
#define NUM_CHANNELS 24
#define TRACE_LEN 4096
#define TRACE_TIME 100
#define CHECK_STATUS(status) if (status) {printf("Error=%d chan=%d\n", status, chan);}

int main(int argc, char **argv)
{
    int iter, chan=0;
    int data[TRACE_LEN];
    double info[2];
    int status;

    printf("Initializing ...\n");
    status = xiaSetLogLevel(2);
    CHECK_STATUS(status);
    status = xiaSetLogOutput("C:\\XMAP\\log.txt");
    CHECK_STATUS(status);
    status = xiaInit("i12-edxd-6cards.ini");
    /*status = xiaInit("xmap16.ini");*/
    CHECK_STATUS(status);
    printf("starting system ...\n");
    status = xiaStartSystem();
    CHECK_STATUS(status);

    /* first ensure the system is fully stopped before we start our test*/
    for (chan=0; chan<NUM_CHANNELS; chan++)
    {
    	printf("stopping run chan %d\n", chan);
    	status = xiaStopRun(chan);
        CHECK_STATUS(status);
    }

    for (iter=0; iter<MAX_ITERATIONS; iter++) {
        printf("Iteration=%d\n", iter);
        for (chan=0; chan<NUM_CHANNELS; chan++) {
        	/* A sleep of 0.1 makes next to no difference.
        	 * The 0.5s sleep makes a rather large difference although does not
        	 * completely get rid of the problem (and of course makes it all *very* slow)	 */
        	epicsThreadSleep(0.5);

            info[0] = 0.;
            /* Trace time is in ns */
            info[1] = TRACE_TIME;

            status = xiaDoSpecialRun(chan, "adc_trace", info);
            CHECK_STATUS(status);

            status = xiaGetSpecialRunData(chan, "adc_trace", data);
            CHECK_STATUS(status);
        }
    }

    return(xiaExit());
}
