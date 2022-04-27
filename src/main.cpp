//
// main.cpp
//
// Copyright (C) 2022 Dialog Semiconductor
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in 
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//
// Custom driver example main entry points
//



#include <pthread.h>

#include "libidl.h"
#include "example.h"

#ifndef CDNAME
#error Must define CDNAME when compiling this main.cpp file
#endif

#define WAIT_FOR_DEBUGGER_ATTACH

#ifdef WAIT_FOR_DEBUGGER_ATTACH
/* myDbgBreak: a utility function for causing a breakpoint in debugger */
static int myDbgFlag = 1;
void myDbgBreak(void) {
	if (myDbgFlag) {
		// Generate an interrupt
		raise(SIGINT);
	}
}
#endif


/* main: Main entry point for this custom driver example */
int main(int argc, char **argv)
{
    char conf_path[256] ="/var/apollo/data/" CDNAME "/" CDNAME "-idl.conf";
    pthread_t IdiProcessAsynchThread;

/* code section for inserting a countdown wait for the debugger to attach */
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

        pthread_create( &IdiProcessAsynchThread, NULL, ProcAsynThrdFunc, NULL);


		/* Initialize the IDL CDNAME Driver using the parameters defined */
		/* in CDNAME-idl.conf configuration file. IdlInit() never exits. */

        IdlInit(conf_path, idl);
    }
    return 0;
}

