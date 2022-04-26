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

#include "IdlCommon.h"
#include "libidl.h"
#include "cJSON.h"

// Due to an EPR (Jira AP-9580) in Idl library , we have to continue registering the 
// unrecognized column callback routine via IdlDpUnrecColumnCallbackSet for the 
// list of unrecognized columns be reported in OnDpCreateCb.
#define AP_9580_WORKAROUND

#define MAX_UNID_CHARS 132


#define dbg_printf(args...)     printf(args)
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

enum EtiStatus {
    EtiStop = 0,                        // final stop - terminated
    EtiRunning
};

typedef struct _DrvInfo {
    EtiStatus stat;
    uint deviceEntry;                   // current device entry/count - up to CDDEVLIMIT
    T_DevNodePtr pHeadDevNode;
#ifdef INCLUDE_ETI
	struct mosquitto *mosq;			    // mosquitto message queue for all devices
    mqd_t devActQueue;				    // message Queue for sending pending device actions to vTaskDevAct
#endif
    pthread_mutex_t mutex;
} T_DrvInfo;



#endif