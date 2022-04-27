//
// common.h
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
// Custom driver common include file for example driver and ETI protocol
//

#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <cassert>
#include <unistd.h>
#include <string.h>
#include <cstring>
#include <mqueue.h>
#include <time.h>
#include <pthread.h>
#include <sys/resource.h> 


#include "IdlCommon.h"
#include "libidl.h"
#include "cJSON.h"

// Due to an EPR (Jira AP-9580) in Idl library , we have to continue registering the 
// unrecognized column callback routine via IdlDpUnrecColumnCallbackSet for the 
// list of unrecognized columns be reported in OnDpCreateCb.
#define AP_9580_WORKAROUND

#define SUCCESS                 IErr_Success
#define FAILURE                 IErr_Failure

#define BLOCKING_Q              1
#define NONBLOCKING_Q           0
#define MQ_HARD_LIM             10000
#define TASK_DEVACT_STACK_SIZE  (32 * 1024)

#define MAX_UNID_CHARS          132
#define Q_NAME_LENGTH           256


#define dbg_printf(args...)     /* printf(args) */
#define info_printf(args...)    printf(args)
#define err_printf(...)         fprintf(stderr, __VA_ARGS__)



// Local storage
typedef cJSON *T_DataPoint, **T_DpValPtr, **T_DpValVector; 

typedef struct _DrvInfo *T_DrvInfoPtr;

typedef struct {
    // pDpValue point to an entry in pDevDpValVector (via dp->address where one or more dp may  
    // use the same dp->address for feedback/loopback (one dp with R/W and the other R/O access)
    T_DataPoint *pDpValue;
    uint address;                       // used in eti example as register number/address 
    double testMultiplier;              // used in the example for showcasing XIF custom/unrecognized column
} T_DpSto, *T_DpStoVector;

typedef struct _DevSto {
    char devUid[MAX_UNID_CHARS+1];      // per device device id max 132 characters plus a null terminator
    uint devDpEntry;                    // per device current datapoint entry/count
    uint devDpCounts;                   // per device total datapoint count
    T_DpValVector pDevDpValVector;      // point to the begining of per device JSON dp values vector
    T_DpStoVector pDevDpVector;         // point to the begining of per device datapoint struct vector
    T_DrvInfoPtr  pDrvInfo;             // point back to the driver info structure
} T_DevSto, *T__DevStoPtr;

typedef struct _DevNodePtr {
    T__DevStoPtr pDevSto;
    _DevNodePtr *pPrevious;
    _DevNodePtr *pNext;
} T_DevNode, *T_DevNodePtr;

enum IdiStatus {
    IdiStop = 0,                        // final stop - terminated
    IdiRunning
};

typedef struct _DrvInfo {
    IdiStatus stat;
    uint deviceEntry;                   // current device entry/count - up to CDDEVLIMIT
    T_DevNodePtr pHeadDevNode;
    mqd_t idiDevActQueue;				// message Queue for sending pending device actions to vTaskIdiDevAct
#ifdef INCLUDE_ETI
	struct mosquitto *mosq;			    // mosquitto message queue for all devices
    mqd_t etiDevActQueue;				// message Queue for sending pending device actions to vTaskEtiDevAct
#endif
} T_DrvInfo;



extern int IdiCreateQueue(mqd_t *queueHndl, const char *name, int isBlocking, int queueSize, int msgSize);

#endif