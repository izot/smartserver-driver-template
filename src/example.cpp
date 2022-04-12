#include <time.h>
#include <stdlib.h>
#include <cassert>
#include <unistd.h>

#include "libidl.h"
#include "example.h"

#ifndef CDNAME
#error Must define CDNAME when compiling this example.cpp file
#endif

extern Idl *idl;

// For this example there is only one activeCB.  In real-life driver this
// should be a linked list of activeCBs and the IdiProcAsynchThreadFunction
// will cycle through the list of activeCBs until no more available.
static IdiActiveCB activeCB;

// Dummy value and priority array for sake of this CDNAME
// Would be read/writing to designated datapoint instead

double dpValue = 0;
char *prio_array = NULL;


static int IdiBlockWhileBusy(IdiActiveCB& pActCb);
static int IdiGenericResultFsm(IdiActiveCB& pActCb);


/* IdiStart: Custom driver startup function called from main.cpp  */

int IdiStart() 
{
	/* Your custom IDL driver can start up any other driver-specific  */
	/* actions here.  For example, you could open a connection to a   */
	/* serial port or a USB interface.  You should return a 0 here    */
	/* if your code started up your driver properly or else return 1. */

	printf("The " CDNAME " IDL driver is connected and ready...\n");

    return 0;
}

/* dp_read_cb: Callback function registered with the IDL Library  */
/* which triggers when a data point read occurs.  This checks if  */
/* the custom driver (idi) is busy and if not simulates a dp read.*/

int dp_read_cb(int request_index, IdlDev *dev, IdlDatapoint *dp, void *context)
{
    int idlError = IErr_Success;
    printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    printf(" dev.info.product: %s\n"				// Print out selected fields to the console
        " dev.unid: %s\n"
        " dp.name: %s\n"
        ,dev->info.product,
        dev->unid,
        dp->name
    );

    if (activeCB.action == IdiaNone) {
        // Setup read
        activeCB = (IdiActiveCB){0};
        activeCB.action = IdiaDpread;
        activeCB.ReqIndex = request_index;
        activeCB.dev = dev;
        activeCB.dp = dp;
        activeCB.timeout = IDI_ACTION_NORMAL_TIMEOUT;
        activeCB.context = context;
    }
    else {
        // ideally this should not happen if we implement adaquate queue of activeCBs
        idlError = IErr_IdiBusy;
    }

    return idlError;
}

/* dp_write_cb: Callback function registered with the IDL Library */
/* which triggers when a data point write occurs.  This checks if */
/* the custom driver (idi) is busy and if not simulates a dp write.*/

int dp_write_cb(int request_index, IdlDev *dev, IdlDatapoint *dp, int prio, int relinquish, double value)
{
    int idlError = IErr_Success;

    printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    printf(" dev.info.product: %s\n"				// Print out selected fields to the console
        " dev.unid: %s\n"
        " dp.name: %s\n"
        ,dev->info.product,
        dev->unid,
        dp->name
    );

    if (activeCB.action == IdiaNone) {
        // Setup write
        activeCB = (IdiActiveCB){0};
        activeCB.action = IdiaDpwrite;
        activeCB.ReqIndex = request_index;
        activeCB.dev = dev;
        activeCB.dp = dp;
        activeCB.prio = prio;
        activeCB.relinquish = relinquish;
        activeCB.value = value;
        activeCB.timeout = IDI_ACTION_NORMAL_TIMEOUT;
    }
    else {
        // ideally this should not happen if we implement adaquate queue of activeCBs
        idlError = IErr_IdiBusy;
    }

    return idlError;
}

/* dev_create_cb: Callback function registered with the IDL Library  */
/* which triggers when a device of this protocol type is created.    */

int dev_create_cb(int request_index, IdlDev *dev, char *args, char *xif_dp_array)
{
    int idlError = IErr_Success;

    printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    printf(" dev.state: %d\n"				// Print out selected fields to the console
        " dev.info.name: %s\n"
		" dev.info.manufacturer: %s\n"	
        " dev.info.product: %s\n"
        " dev.unid: %s\n"
        " dev.handle: %s\n"
        " dev.type: %s\n"
		" args: %s\n"
        ,dev->state,
        dev->info.name,
        dev->info.manufacturer,
        dev->info.product,
        dev->unid,
        dev->handle,
        dev->type,
		(args) ? args : "NULL"
    );
	  
    if (activeCB.action == IdiaNone) {
        // Setup provision
        activeCB = (IdiActiveCB){0};
        activeCB.action = IdiaCreate;
        activeCB.ReqIndex = request_index;
        activeCB.dev = dev;
        activeCB.args = args;
        activeCB.timeout = IDI_ACTION_LONG_TIMEOUT;
    }
    else {
        // ideally this should not happen if we implement adaquate queue of activeCBs
        idlError = IErr_IdiBusy;
    }

    return idlError;
}

/* dev_provision_cb: Callback function registered with the IDL Library  */
/* which triggers when a request occurs to provision a device that uses */
/* this protocol / driver. 												*/

int dev_provision_cb(int request_index, IdlDev *dev, char *args)
{
    int idlError = IErr_Success;

    printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    printf(" dev.state: %d\n"				// Print out selected fields to the console
        " dev.info.name: %s\n"
		" dev.info.manufacturer: %s\n"	
        " dev.info.product: %s\n"
        " dev.unid: %s\n"
        " dev.handle: %s\n"
        " dev.type: %s\n"
		" args: %s\n"
        ,dev->state,
        dev->info.name,
        dev->info.manufacturer,
        dev->info.product,
        dev->unid,
        dev->handle,
        dev->type,
		(args) ? args : "NULL"
    );

    if (activeCB.action == IdiaNone) {
        // Setup provision
        activeCB = (IdiActiveCB){0};
        activeCB.action = IdiaProvision;
        activeCB.ReqIndex = request_index;
        activeCB.dev = dev;
        activeCB.args = args;
        activeCB.timeout = IDI_ACTION_LONG_TIMEOUT;
    }
    else {
        // ideally this should not happen if we implement adaquate queue of activeCBs
        idlError = IErr_IdiBusy;
    }

    return idlError;
}

/* dev_deprovision_cb: Callback function registered with the IDL */
/* Library which triggers when a request occurs to deprovision a */
/* device that uses this protocol / driver. 					 */

int dev_deprovision_cb(int request_index, IdlDev *dev)
{
    int idlError = IErr_Success;
//    printf("%s: UNID: %s\n", __FUNCTION__, (dev && dev->unid) ? dev->unid : "NULL");

    printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    printf(" dev.state: %d\n"				// Print out selected fields to the console
        " dev.info.name: %s\n"
		" dev.info.manufacturer: %s\n"	
        " dev.info.product: %s\n"
        " dev.unid: %s\n"
        " dev.handle: %s\n"
        " dev.type: %s\n"
        ,dev->state,
        dev->info.name,
        dev->info.manufacturer,
        dev->info.product,
        dev->unid,
        dev->handle,
        dev->type
    );

    if (activeCB.action == IdiaNone) {
        // Setup deprovision

        /*
        *  Deprovision this device
        */
        activeCB = (IdiActiveCB){0};
        activeCB.action = IdiaDeprovision;
        activeCB.ReqIndex = request_index;
        activeCB.dev = dev;
        activeCB.timeout = IDI_ACTION_LONG_TIMEOUT;
    }
    else {
        // ideally this should not happen if we implement adaquate queue of activeCBs
        idlError = IErr_IdiBusy;
    }

    return idlError;
}

/* dev_replace_cb: Callback function registered with the IDL Library */
/* which triggers when a request occurs to replace a device that	 */
/* uses this protocol / driver. 					 				 */

int dev_replace_cb(int request_index, IdlDev *dev, char *args)
{
    int idlError = IErr_Success;
//    printf("%s: UNID: %s, ARGS: %s\n", __FUNCTION__,
//        (dev && dev->unid) ? dev->unid : "NULL", (args) ? args : "NULL");
		
    printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    printf(" dev.state: %d\n"				// Print out selected fields to the console
        " dev.info.name: %s\n"
		" dev.info.manufacturer: %s\n"	
        " dev.info.product: %s\n"
        " dev.unid: %s\n"
        " dev.handle: %s\n"
        " dev.type: %s\n"
		" args: %s\n"
        ,dev->state,
        dev->info.name,
        dev->info.manufacturer,
        dev->info.product,
        dev->unid,
        dev->handle,
        dev->type,
		(args) ? args : "NULL"
    );

    if (activeCB.action == IdiaNone) {
        // Setup replace

        /*
        *  Slot in the new device and deprovision the old device
        */
        
        activeCB = (IdiActiveCB){0};
        activeCB.action = IdiaReplace;
        activeCB.ReqIndex = request_index;
        activeCB.dev = dev;
        activeCB.args = args;
        activeCB.timeout = IDI_ACTION_LONG_TIMEOUT;
    }
    else {
        // ideally this should not happen if we implement adaquate queue of activeCBs
        idlError = IErr_IdiBusy;
    }

    return idlError;
}

/* dev_delete_cb: Callback function registered with the IDL Library  */
/* which triggers when a device of this protocol type is deleted.    */

int dev_delete_cb(int request_index, IdlDev *dev)
{
    int idlError = IErr_Success;
	
    printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    printf(" dev.state: %d\n"				// Print out selected fields to the console
        " dev.info.name: %s\n"
		" dev.info.manufacturer: %s\n"	
        " dev.info.product: %s\n"
        " dev.unid: %s\n"
        " dev.handle: %s\n"
        " dev.type: %s\n"
        ,dev->state,
        dev->info.name,
        dev->info.manufacturer,
        dev->info.product,
        dev->unid,
        dev->handle,
        dev->type
    );

    if (activeCB.action == IdiaNone) {
        // Setup delete

        /*
        *  Delete this device
        */

        activeCB = (IdiActiveCB){0};
        activeCB.action = IdiaDelete;
        activeCB.ReqIndex = request_index;
        activeCB.dev = dev;
        activeCB.timeout = IDI_ACTION_LONG_TIMEOUT;
    }
    else {
        // ideally this should not happen if we implement adaquate queue of activeCBs
        idlError = IErr_IdiBusy;
    }

    return idlError;
}

/* Returns an integer in the range [0, n).
 *
 * Uses rand(), and so is affected-by/affects the same seed.
 */
uint randint(uint n) {
    // Chop off all of the values that would cause skew...
    int end = RAND_MAX / n; // truncate skew
    assert (end > 0);
    end *= n;

    // ... and ignore results from rand() that fall above that limit.
    // (Worst case the loop condition should succeed 50% of the time,
    // so we can expect to bail out of this loop pretty quickly.)
    int r;
    while ((r = rand()) >= end);

    return r % n;
}

/* IsProcessingDone: a function returning true 60% of a time to simulate a busy condition */
bool IsProcessingDone() {
    uint digit =  randint(9);
    // printf("random digit: %u\n", digit);
    return (digit < 6) ? true : false;
}


/* IdiProcAsynchThreadFunction: Example thread called from main.cpp to */
/*  allow typical custom driver to receive and process async incoming  */
/*  I/O data.  In this example, this routine is used to drive an FSM   */
/*  code to simulate asynchronous processing of any callback routines. */

void *IdiProcAsynchThreadFunction(void* argA)
{
    printf("The " CDNAME " IDI Process Asynchronous Requests driver thread started...\r\n");

    srand(time(NULL));   // Initialization, should only be called once.

    while(1)
    {
		/* Custom code for processing callback requests asynchronously */
        /* Ideally we should implement an adaquate queue of activeCBs  */
        IdiBlockWhileBusy(activeCB);


		usleep(50000); 			// 50 ms
    }

    return NULL;
}

int IdiGenericResultFsm(IdiActiveCB& aCB)
{
    int idlError = IErr_Success;

    switch(aCB.action) {
        case IdiaNone:
            break;
        case IdiaCreate:
            if (aCB.dev && aCB.dev->handle) {
                /*
                *  Create a device
                */
               if (!IsProcessingDone()) {
                   /* simulate a busy condition (not done) */
                   idlError = IErr_IdiBusy;
               }
            } else {
                printf("%s: Malformed IdlDev", __FUNCTION__);
                idlError = IErr_Failure;
            }

            if (idlError != IErr_IdiBusy) {
                IdiFree(aCB.args);      // required
                IdlDevCreateResult(aCB.ReqIndex, aCB.dev, idlError);
                aCB.action = IdiaNone;
            }
            break;
        case IdiaDelete:
            if (aCB.dev && aCB.dev->handle) {
                /*
                *  Delete a device
                */
               if (!IsProcessingDone()) {
                   /* simulate a busy condition (not done) */
                   idlError = IErr_IdiBusy;
               }
            } else {
                printf("%s: Malformed IdlDev", __FUNCTION__);
                idlError = IErr_Failure;
            }

            if (idlError != IErr_IdiBusy) {
                IdiFree(aCB.args);      // required
                IdlDevDeleteResult(aCB.ReqIndex, aCB.dev, idlError);
                aCB.action = IdiaNone;
            }
            break;
        case IdiaReplace:
            if (aCB.dev && aCB.dev->handle) {
                /*
                *  Replace a device
                */
               if (!IsProcessingDone()) {
                   /* simulate a busy condition (not done) */
                   idlError = IErr_IdiBusy;
               }
            } else {
                printf("%s: Malformed IdlDev", __FUNCTION__);
                idlError = IErr_Failure;
            }

            if (idlError != IErr_IdiBusy) {
                IdiFree(aCB.args);      // required
                IdlDevReplaceResult(aCB.ReqIndex, aCB.dev, idlError);
                aCB.action = IdiaNone;
            }
            break;
        case IdiaDpwrite:
            /*
             *  Do our write
             */
            if (IsProcessingDone()) {
                dpValue = aCB.value;
                printf(" Value written: %lf\n", dpValue);
                IdlDpWriteResult(aCB.ReqIndex, aCB.dev, aCB.dp, IErr_Success);
                aCB.action = IdiaNone;
            }
            else {
                /* simulate a busy condition (not done) */
                idlError = IErr_IdiBusy;
            }
            break;
        case IdiaDpread:
            /*
             *  Do our read
             */
            if (IsProcessingDone()) {
                aCB.action = IdiaNone;
                printf("%s Value read: %lf\n", 
                        (aCB.lastError == IErr_IdiBusy) ? ".done\n" : "",
                        dpValue);
                IdlDpReadResult(aCB.ReqIndex, aCB.dev, 
                    aCB.dp, aCB.context, IErr_Success, prio_array, dpValue);
            }
            else {
                /* simulate a busy condition (not done) */
                idlError = IErr_IdiBusy;
            }
            break;
        case IdiaProvision:
            /*
             *  Provision our device
             */
            if (IsProcessingDone()) {
                aCB.action = IdiaNone;
                IdlDevProvisionResult(aCB.ReqIndex, aCB.dev, IErr_Success);
            }
            else {
                /* simulate a busy condition (not done) */
                idlError = IErr_IdiBusy;
            }
            break;
        case IdiaDeprovision:
            /*
             *  Deprovision our device
             */
            if (IsProcessingDone()) {
                aCB.action = IdiaNone;
                IdlDevDeprovisionResult(aCB.ReqIndex, aCB.dev, IErr_Success);
            }
            else {
                /* simulate a busy condition (not done) */
                idlError = IErr_IdiBusy;
            }
            break;
        default:
            aCB.action = IdiaNone;     // unknown, clear it.
            break;
    }
    aCB.lastError = idlError;       // remember last processing error
    return idlError;
}

// This implementation is not ideal..
// An ideal custom driver should not be blocking the FSM while processing a long callback..
// The while loop below with its sleep function call should be avoided if possible.
int IdiBlockWhileBusy(IdiActiveCB& aCB) {
    int idlError = IErr_Success;

    bool newAction = true;
    if (aCB.action != IdiaNone) {
        time_t timeStart = time(NULL);
        while (aCB.action != IdiaNone) {
            IdiGenericResultFsm(aCB);
            // check again -
            if (aCB.action == IdiaNone) {
                break;
            }
            else {
                if (newAction) {
                    printf(" Random delay.");
                    newAction = false;
                }
                else {
                    printf(".");
                }
                if ((uint) (time(NULL) - timeStart) >= aCB.timeout) {
                    printf("\n%s: Timed out - UNID: %s, action: %d\n", __FUNCTION__, 
                        (aCB.dev->unid) ? aCB.dev->unid : "NULL", aCB.action);
                    idlError = IErr_Failure;
                    break;
                }
                usleep(10000); // 10 ms
            }
        }
    }
    return idlError;
}


