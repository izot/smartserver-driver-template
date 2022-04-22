#include <pthread.h>

#include "libidl.h"
#include "example.h"

#ifndef CDNAME
#error Must define CDNAME when compiling this main.cpp file
#endif

#define WAIT_FOR_DEBUGGER_ATTACH

#ifdef WAIT_FOR_DEBUGGER_ATTACH
static int myDbgFlag = 1;
void myDbgBreak(void) {
	if (myDbgFlag) {
		// Generate an interrupt
		raise(SIGINT);
	}
}
#endif

int main(int argc, char **argv)
{
    char conf_path[256] ="/var/apollo/data/" CDNAME "/" CDNAME "-idl.conf";
    pthread_t IdiProcessAsynchThread;

#ifdef WAIT_FOR_DEBUGGER_ATTACH
	// argv[1] - simulation instance number
    bool bDebuging = false;
	if (argc > 1) {
		if (strcasecmp(argv[1], "-d") == 0);
            bDebuging = true;
	}

    unsigned long countdown = 30;
	if (bDebuging) {
        if (argc > 2) {
            countdown = (uint)strtol(argv[2], NULL, 10);
            if (countdown < 1)
                countdown = 1;
        }
        
        printf("Wait %ld seconds for debugger to attach ", countdown);

        while(--countdown > 0) {
            sleep(1);
            printf(".");
            fflush(stdout);
        }
        printf(" continue!\n");
        myDbgBreak();
    }
    

#endif

    Idl *idl = IdlNew();		// Create a new driver instance

    /* Register the CDNAME Driver Callback Routines with the IAP Driver Library (IDL) */
	
    IdlDevCreateCallbackSet(idl, OnDevCreateCb);
    IdlDevProvisionCallbackSet(idl, OnDevProvisionCb);
    IdlDevDeprovisionCallbackSet(idl, OnDevDeprovisionCb);
    IdlDevReplaceCallbackSet(idl, OnDevReplaceCb);
    IdlDevDeleteCallbackSet(idl, OnDevDeleteCb);
    IdlDpReadCallbackSet(idl, OnDpReadCb);
    IdlDpWriteCallbackSet(idl, OnDpWriteCb);
    IdlDpAsciiReadCallbackSet(idl, OnDpReadExCb);
    IdlDpAsciiWriteCallbackSet(idl, OnDpWriteExCb);
    IdlDpCreateCallbackSet(idl, OnDpCreateCb);
    #ifdef AP_9580_WORKAROUND
    // Due to an EPR (Jira AP-9580) in Idl library , we have to continue registering the 
    // unrecognized column callback routine via IdlDpUnrecColumnCallbackSet for the 
    // list of unrecognized columns be reported in OnDpCreateCb.
    IdlDpUnrecColumnCallbackSet(idl, OnUnrecColumnCb);
    #endif

    if (IdiStart() == 0) {
        printf("The " CDNAME " IDL Driver started up...\r\n");
		
		/* Create any POSIX threads your driver might need before calling  */
		/* IdlInit() as IdlInit() never returns.  For example, this driver */
		/* creates a thread that could be used to process asynchronous     */
		/* communications packets the driver needs to capture and process. */

        pthread_create( &IdiProcessAsynchThread, NULL, IdiProcAsynchThreadFunction, NULL);


		/* Initialize the IDL CDNAME Driver using the parameters defined */
		/* in CDNAME-idl.conf configuration file. IdlInit() never exits. */

        IdlInit(conf_path, idl);
    }
    return 0;
}

