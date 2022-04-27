//
// example.cpp
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
// Custom driver source file for example driver
//

#include "common.h"
#include "example.h"


#ifndef CDNAME
#error CDNAME must be defined in Makefile
#endif
#ifndef CDDEVLIMIT
#error CDDEVLIMIT must be defined in Makefile
#endif


extern Idl *idl;

static T_DrvInfo gDrvInfo = {};


// Dummy value and priority array for sake of this driver
// Would be read/writing to designated datapoint instead

static char *prio_array = NULL;


static int IdiBlockWhileBusy(IdiActionCB& pActCb);
static int IdiGenericResultFsm(IdiActionCB& pActCb);
static cJSON *GetDpValForLocStorUpdate(IdiActionCB& pActCb);
static cJSON *GenerateDpDefVal(IdlDatapoint *dp);



/* IdiStart: Custom driver startup function called from main.cpp  */
int IdiStart() 
{
	/* Your custom IDL driver can start up any other driver-specific  */
	/* actions here.  For example, you could open a connection to a   */
	/* serial port or a USB interface.  You should return a 0 here    */
	/* if your code started up your driver properly or else return 1. */

    info_printf("INFO %s: The " CDNAME " IDL driver is connected and ready...\n", 
                    __FUNCTION__);

#ifdef INCLUDE_ETI
    EtiInit(&gDrvInfo);
#endif

    return 0;
}


/* IdiCreateQueue: a utility function to create a message queue for further  */
/* processing of device actions by an asynch action processing thread        */
int IdiCreateQueue(mqd_t *queueHndl, const char *name, int isBlocking, int queueSize, int msgSize)
{
    long hardLimit = 0;
    int ret = FAILURE;
    int qFlags = 0;
    mode_t qPerms = {0};
    struct mq_attr qAttr = {0};
    struct rlimit mqLimit = {0};

    if (getrlimit(RLIMIT_MSGQUEUE, &mqLimit) == 0) {
        hardLimit = mqLimit.rlim_max;
        dbg_printf("%s Old mqLlimit -> soft limit= %ld, hard limit= %ld\n", __FUNCTION__, 
                        mqLimit.rlim_cur, mqLimit.rlim_max); 
        if (hardLimit <= 0xfffffff) {
            hardLimit = 0xfffffff;
            mqLimit.rlim_max = hardLimit;
            mqLimit.rlim_cur = hardLimit;
            if (setrlimit(RLIMIT_MSGQUEUE, &mqLimit) == 0) {
                dbg_printf("%s successfully set limits\n", __FUNCTION__);
                dbg_printf("%s new hardLimit = %ld\n", __FUNCTION__, hardLimit);
            } else {
                dbg_printf("%s error in setting mqlimit, errno = %d\n", __FUNCTION__, errno);
            }
        }
    }
    qAttr.mq_maxmsg = queueSize;
    qPerms = S_IRUSR | S_IWUSR;
    qAttr.mq_msgsize = msgSize;
    qFlags = O_CREAT | O_EXCL | O_RDWR;
    if (isBlocking == 0) {
        qFlags = qFlags | O_NONBLOCK;
    }

    if (name && queueSize && msgSize) {
        *queueHndl = mq_open(name, qFlags, qPerms, &qAttr);
        if (*queueHndl == (mqd_t) -1)
        {
            dbg_printf("%s Unable to create mqueue %s, errno = %d\r\n", __FUNCTION__, name, errno);
            if (errno == 17)                                                                          
            {   
                /* Delete existing queue and try again */
                mq_unlink(name);
                dbg_printf("%s Opening queue again after unlink\n", __FUNCTION__);
                *queueHndl = mq_open(name, qFlags, qPerms, &qAttr);                           
                if (*queueHndl == (mqd_t) -1)
                {   
                    err_printf("ERROR: %s- Not able to create mqueue %s"
                            "after unlinking, errno = %d\n", 
                            name, __FUNCTION__, errno);
                } else {
                    ret = SUCCESS;
                }
            }
        } else {
            ret = SUCCESS;
        }
    }
    return ret;
}


/* IerrToStr: a utility function to convert IdiError to string */
static const char *IErrToStr(int errCode) 
{
    const char *cpErr;

    switch (errCode) {
    case IErr_Success: 
        cpErr = "IErr_Success"; 
        break;
    case IErr_Failure: 
        cpErr = "IErr_Failure"; 
        break;
    case IErr_IdiBusy: 
        cpErr = "IErr_IdiBusy"; 
        break;
    case IErr_TypeInvalid: 
        cpErr = "IErr_TypeInvalid"; 
        break;
    case IErr_UnidInvalid: 
        cpErr = "IErr_UnidInvalid"; 
        break;
    case IErr_UnidExists: 
        cpErr  = "IErr_UnidExists"; 
        break;
    case IErr_DevCommFail: 
        cpErr = "IErr_DevCommFail"; 
        break;
    case IErr_Nack: 
        cpErr = "IErr_Nack"; 
        break;
    case IErr_Max: 
        cpErr = "IErr_Max"; 
        break;
    default: 
        cpErr = "IErr_Unknown"; 
        break;
    }

    return cpErr;
}


/* DpSetCustomIdiDpData: a utility function to setup datapoint specific pointers/offsets in */
/* device's storage area.                                                                   */
static int DpSetCustomIdiDpData(IdlDev *dev, IdlDatapoint *dp)
{
    int idlError = IErr_Success;

    if (!dp->idiDpData) {
        if (dev->idiDevData) {
            uint address = dp->address;
            T__DevStoPtr pDevEntry = (T__DevStoPtr)(dev->idiDevData);
            if (pDevEntry->devDpEntry < pDevEntry->devDpCounts &&
                address < pDevEntry->devDpCounts) {
                // initialize local datapoint storage and idiDpData to point to this entry in storage vector
                T_DpSto *pDpStruct = &pDevEntry->pDevDpVector[pDevEntry->devDpEntry];
                // set pDpValue to point to the pDevDpValVector[dp->address] entry
                pDpStruct->pDpValue = &(pDevEntry->pDevDpValVector[address]);
                dbg_printf(" %s: pDpStruct = %p, pDpStruct->pDpValue = %p\n", __FUNCTION__, pDpStruct, pDpStruct->pDpValue);
                // check if the pDevDpValVector[dp->address] entry has possibly been initialized by other 
                // datapoints with the same address (defined in device's XIF)
                if (!pDevEntry->pDevDpValVector[address]) {
                    cJSON *pDefaultJSON = GenerateDpDefVal(dp);
                    pDevEntry->pDevDpValVector[address] = pDefaultJSON;

                    char *jsonStr = cJSON_PrintUnformatted(*pDpStruct->pDpValue);
                    dbg_printf(" %s: dp.name=%s, dp.address=%d, devDpEntry=%d, &pDevEntry->pDevDpValVector[dp->address]=%p, dpValue=%s\n",
                                __FUNCTION__, dp->name, address, pDevEntry->devDpEntry, 
                                &pDevEntry->pDevDpValVector[address], jsonStr);
                    IdlMemFree(jsonStr);
                }
                // set idiDpData to point to an entry in per device's DevDpStorage, record the address & increment the datapoint entry
                pDpStruct->address = address;
                dp->idiDpData = pDpStruct;
                pDevEntry->devDpEntry++;
            } else {
                err_printf("ERROR: %s- devDpEntry for dp.name=%s exceeded devDpCounts(%d) - initialized in DevSetCustomIdiDevData\n", 
                        __FUNCTION__, dp->name, pDevEntry->devDpCounts);
                idlError = IErr_Failure;
            }
        } else {
            err_printf("ERROR: %s- invalid idiDevData for dp.name=%s - initialized in DevSetCustomIdiDevData\n", __FUNCTION__, dp->name);
            idlError = IErr_Failure;
        }
    } else {
        err_printf("ERROR: %s- idiDpData for dp.name=%s has already been initialized with value\n", __FUNCTION__, dp->name);
    }

    return idlError;
}


/* DevReplaceUnid: a utility function to replace device's unid */
static int DevReplaceUnid(IdlDev *dev) {
    int idlError = IErr_Failure;

    // point to the allocated storage
    T__DevStoPtr pLocDevStorageStruc = (T__DevStoPtr)dev->idiDevData;
    if (pLocDevStorageStruc) {
        if (dev->unid) {
            strncpy(pLocDevStorageStruc->devUid, dev->unid, sizeof(pLocDevStorageStruc->devUid));
            pLocDevStorageStruc->devUid[MAX_UNID_CHARS] = '\0'; // forced string termination
        }
        idlError = IErr_Success;
    }
    return idlError;
}


/* DevSetCustomIdiDevData: a utility function to add device specific allocated storage */
/* and set the per device's idiDevData to NULL.                                        */
static int DevSetCustomIdiDevData(IdlDev *dev)
{
    uint dpCount = 0;

    int idlError = IErr_Success;
    if (gDrvInfo.deviceEntry < CDDEVLIMIT) {
        if (!dev->idiDevData) {
            // get the device's total datapoint count
            IdlInterfaceBlock *ifblock = dev->firstIfblock;
            while (ifblock) {
                IdlIapDatapoint *iapdp = ifblock->firstIapdp;
                while (iapdp) {
                    dpCount += iapdp->dpCnt;
                    iapdp = iapdp->next;
                }
                ifblock = ifblock->next;
            }

            // allocate per device DevStorage structure
            T_DevSto *pLocDevStorageStruc = (T_DevSto *)calloc(1, sizeof(T_DevSto));
            if (pLocDevStorageStruc) {
                // dbg_printf("%s: allocated per device DevStorage structure (%p)for local storage\n", __FUNCTION__, pLocDevStorageStruc);
                // allocate per device device datapoint structure vector for local storage 
                T_DpStoVector pDevDpVector = (T_DpStoVector)calloc(dpCount, sizeof(T_DpSto));
                if (pDevDpVector) {
                    // dbg_printf("%s: allocated per device datapoint structure vector (%p)for local storage\n", __FUNCTION__, pDevDpVector);
                    pLocDevStorageStruc->pDevDpVector = pDevDpVector;         // point to the allocated storage

                    // allocate per device device datapoint value vector for local storage 
                    T_DataPoint *pDevDpValVector = (T_DataPoint *)calloc(dpCount, sizeof(T_DataPoint));
                    if (pDevDpValVector) {
                        // dbg_printf("%s: allocated per device datapoint value vector (%p)for local storage\n", __FUNCTION__, pDevDpValVector);
                        pLocDevStorageStruc->devDpCounts = dpCount;
                        pLocDevStorageStruc->pDevDpValVector = pDevDpValVector; // point to the allocated storage

                        T_DevNodePtr pNewNode = (T_DevNodePtr)calloc(1, sizeof(T_DevNode));
                        if (pNewNode) {
                            pNewNode->pDevSto = pLocDevStorageStruc;
                            // insert to the end of the list
                            if (gDrvInfo.pHeadDevNode == NULL) {
                                pNewNode->pNext = pNewNode->pPrevious = pNewNode;
                                gDrvInfo.pHeadDevNode = pNewNode;
                            } else {
                                pNewNode->pNext     = gDrvInfo.pHeadDevNode;
                                pNewNode->pPrevious = gDrvInfo.pHeadDevNode->pPrevious;
                                gDrvInfo.pHeadDevNode->pPrevious->pNext = pNewNode;
                                gDrvInfo.pHeadDevNode->pPrevious = pNewNode;
                            }
                            pNewNode->pDevSto->pDrvInfo = &gDrvInfo;
                        }
                        // set idiDevData to the per device DevStorage Structure & increment device count
                        dev->idiDevData = (void *)pLocDevStorageStruc;          // point to the allocated storage
                        if (dev->unid) {
                            strncpy(pLocDevStorageStruc->devUid, dev->unid, sizeof(pLocDevStorageStruc->devUid));
                            pLocDevStorageStruc->devUid[MAX_UNID_CHARS] = '\0'; // forced string termination
                        }
                        gDrvInfo.deviceEntry++;
                        idlError = IErr_Success;
                    } else {
                        err_printf("ERROR: %s- failed to allocate per device's dp value vector for local storage\n", __FUNCTION__);
                    }
                } else {
                    err_printf("ERROR: %s- failed to allocate per device's dp structure vector for local storage\n", __FUNCTION__);
                }
            } else {
                    err_printf("ERROR: %s- failed to allocate per device DevStorage structure for local storage\n", __FUNCTION__);
            }
        } else {
            err_printf("ERROR: %s- Can't set custom_idiDevData, it has already been set!", __FUNCTION__);
        }
    } else {
        err_printf("ERROR: %s- device count exceeded precompiled max number (%d) defined in device idl configuration\n", 
                    __FUNCTION__, CDDEVLIMIT);
    }

    return idlError;
}


/* DevRemoveCustomIdiDevData: a utility function to remove device specific allocated storage */
/* and set the per device's idiDevData to NULL.                                              */
static int DevRemoveCustomIdiDevData(IdlDev *dev)
{
    int idlError = IErr_Failure;

    if (dev->idiDevData) {
        // deallocate per device DeviceStorageStruc, dp storage vector, dp value storage vector & 
        // update gDrvInfo.pHeadDevNode list
        // we assume this routine is called within the fsm in threadsafe manner
        T_DevNodePtr pCurNode = gDrvInfo.pHeadDevNode;
        dbg_printf("\n%s: pCurNode = pHeadDevNode (%p)for local storage\n", __FUNCTION__, pCurNode);
        while (pCurNode) {
            dbg_printf("\n%s: processing pCurNode(%p) with name=%s\n", __FUNCTION__, pCurNode, pCurNode->pDevSto->devUid);
            if (pCurNode->pDevSto == (T__DevStoPtr)dev->idiDevData) {
                // we found the device storage node
                dbg_printf("\n%s: Dev->name:%s pCurNode(%p) pCurNode->pDevSto(%p) == dev->idiDevData\n", __FUNCTION__, 
                                 dev->info.name, pCurNode, pCurNode->pDevSto);
                if (pCurNode == gDrvInfo.pHeadDevNode) {
                    // we are removing the head node
                    dbg_printf("\n%s: pCurNode == pHeadDevNode(%p) => removing from head\n", __FUNCTION__, gDrvInfo.pHeadDevNode);
                    if (pCurNode->pNext == pCurNode) {
                        // we are removing the only node
                        gDrvInfo.pHeadDevNode = NULL;
                        dbg_printf("\n%s: pCurNode->pNext == pCurNode(%p) => removing the only node\n", __FUNCTION__, pCurNode->pNext);
                    } else {
                        // we are moving the head to the next node
                        gDrvInfo.pHeadDevNode = pCurNode->pNext;
                        dbg_printf("\n%s: => moving  head to next node; gDrvInfo.pHeadDevNode = pCurNode->pNext(%p)\n", 
                                    __FUNCTION__, pCurNode->pNext);
                        pCurNode->pPrevious->pNext = pCurNode->pNext;
                        dbg_printf("\n%s: pCurNode->pPrevious->pNext = pCurNode->pNext;(%p)\n", __FUNCTION__, pCurNode->pNext);
                        pCurNode->pNext->pPrevious = pCurNode->pPrevious;
                        dbg_printf("\n%s: pCurNode->pNext->pPrevious = pCurNode->pPrevious;(%p)\n", __FUNCTION__, pCurNode->pPrevious);
                    }
                } else {
                    dbg_printf("\n %s: Dev->name:%s pCurNode(%p) pCurNode != pHeadDevNode(%p) => removing from somewhere in list\n", 
                                __FUNCTION__, dev->info.name, pCurNode, gDrvInfo.pHeadDevNode);
                    pCurNode->pPrevious->pNext = pCurNode->pNext;
                    dbg_printf("\n %s: pCurNode->pPrevious->pNext = pCurNode->pNext;(%p)\n", __FUNCTION__, pCurNode->pNext);
                    pCurNode->pNext->pPrevious = pCurNode->pPrevious;
                    dbg_printf("\n %s: pCurNode->pNext->pPrevious = pCurNode->pPrevious;(%p)\n", __FUNCTION__, pCurNode->pPrevious);
                }

                dbg_printf("\n %s: deallocate per device datapoint structure vector  (%p)for local storage\n", __FUNCTION__, 
                            pCurNode->pDevSto->pDevDpVector);
                IdlMemFree( pCurNode->pDevSto->pDevDpVector );
                dbg_printf("\n %s: deallocate per device datapoint value vector (%p)for local storage\n", __FUNCTION__, 
                            pCurNode->pDevSto->pDevDpValVector );
                IdlMemFree( pCurNode->pDevSto->pDevDpValVector );
                dbg_printf("\n %s: deallocate per device DevStorage structure (%p)for local storage\n", __FUNCTION__, 
                            (dev->idiDevData));
                IdlMemFree( (dev->idiDevData));
                dbg_printf("\n %s: Freeing pCurNode(%p)\n", __FUNCTION__, pCurNode);
                IdlMemFree(pCurNode);   // free the device storage node
                dev->idiDevData = NULL;
                gDrvInfo.deviceEntry--;

                idlError = IErr_Success;
                break;  // we are done
            }
            pCurNode = pCurNode->pNext;
            dbg_printf("\n %s: pCurNode = pCurNode->pNext;(%p)\n", __FUNCTION__, pCurNode);
            if (pCurNode == gDrvInfo.pHeadDevNode) {
                dbg_printf("\n %s: Done - Unable to find the entry in gDrvInfo.pHeadDevNode list to clear custom_idiDevData!", 
                            __FUNCTION__);
                break;  // we have exhausted the search
            }
        }
        dbg_printf("\n %s: done. idlError=%d\n", __FUNCTION__, idlError);
    } else {
        err_printf("ERROR: %s- Can't clear custom_idiDevData, it has not been set!", __FUNCTION__);
    }

    return idlError;
}


/* OnDpReadCb: Callback function registered with the IDL Library which triggers */
/* when a regular (of type native double) data point read occurs.               */
int OnDpReadCb(int request_index, IdlDev *dev, IdlDatapoint *dp, void *context)
{
    int idlError = IErr_Success;
#ifdef DEBUGGING
    dbg_printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    dbg_printf(" dev.info.product: %s\n"				// Print out selected fields to the console
        " dev.unid: %s\n"
        " dp.name: %s\n"
        " dp.addr: %d\n"
        ,dev->info.product,
        dev->unid,
        dp->name,
        dp->address
    );
#endif

    // Setup read
    IdiActionCB aCB = (IdiActionCB){0};
    aCB.action = IdiaDpread;
    aCB.ReqIndex = request_index;
    aCB.dev = dev;
    aCB.dp = dp;
    aCB.timeout = IDI_ACTION_NORMAL_TIMEOUT;
    aCB.context = context;
    if (mq_send(gDrvInfo.idiDevActQueue, (const char *)&aCB, sizeof(IdiActionCB), 1) != 0) {    
        err_printf("WARN: %s- Failed to send IdiaDpread to idiDevActQueue, err=%d\n", __FUNCTION__, errno);
        idlError = IErr_IdiBusy;
    }

    return idlError;
}


/* OnDpReadExCb: Callback function registered with the IDL Library which     */
/* triggers when a data point of type ascii or structured native read occurs. */
int OnDpReadExCb(int request_index, IdlDev *dev, IdlDatapoint *dp, void *context)
{

    int idlError = IErr_Success;
#ifdef DEBUGGING
    dbg_printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    dbg_printf(" dev.info.product: %s\n"				// Print out selected fields to the console
        " dev.unid: %s\n"
        " dp.name: %s\n"
        " dp.addr: %d\n"
        ,dev->info.product,
        dev->unid,
        dp->name,
        dp->address
    );
#endif

    // Setup read
    IdiActionCB aCB = (IdiActionCB){0};
    aCB.action = IdiaDpread;
    aCB.ReqIndex = request_index;
    aCB.dev = dev;
    aCB.dp = dp;
    aCB.timeout = IDI_ACTION_NORMAL_TIMEOUT;
    aCB.context = context;
    if (mq_send(gDrvInfo.idiDevActQueue, (const char *)&aCB, sizeof(IdiActionCB), 1) != 0) {    
        err_printf("WARN: %s- Failed to send IdiaDpread to idiDevActQueue, err=%d\n", __FUNCTION__, errno);
        idlError = IErr_IdiBusy;
    }

    return idlError;
}


/* OnDpWriteCb: Callback function registered with the IDL Library which triggers */
/* when a regular (of type native double) data point write occurs.               */
int OnDpWriteCb(int request_index, IdlDev *dev, IdlDatapoint *dp, int prio, int relinquish, double value)
{
    int idlError = IErr_Success;

    dbg_printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    dbg_printf(" dev.info.product: %s\n"				// Print out selected fields to the console
        " dev.unid: %s\n"
        " dp.name: %s\n"
        " dp.addr: %d\n"
        " dp.value: %lf\n"
        ,dev->info.product,
        dev->unid,
        dp->name,
        dp->address,
        value
    );

    // Setup write
    IdiActionCB aCB = (IdiActionCB){0};
    aCB.action = IdiaDpwrite;
    aCB.ReqIndex = request_index;
    aCB.dev = dev;
    aCB.dp = dp;
    aCB.prio = prio;
    aCB.relinquish = relinquish;
    aCB.dValue = value;
    aCB.timeout = IDI_ACTION_NORMAL_TIMEOUT;
    if (mq_send(gDrvInfo.idiDevActQueue, (const char *)&aCB, sizeof(IdiActionCB), 1) != 0) {    
        err_printf("WARN: %s- Failed to send IdiaDpwrite to idiDevActQueue, err=%d\n", __FUNCTION__, errno);
        idlError = IErr_IdiBusy;
    }

    return idlError;
}


/* OnDpWriteExCb: Callback function registered with the IDL Library which triggers when a */
/* data point of type ascii or structured native write occurs.                            */
int OnDpWriteExCb(int request_index, IdlDev *dev, IdlDatapoint *dp, int prio, 
                      int relinquish, char *value)
{
    int idlError = IErr_Success;

    dbg_printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    dbg_printf(" dev.info.product: %s\n"			// Print out selected fields to the console
        " dev.unid: %s\n"
        " dp.name: %s\n"
        " dp.addr: %d\n"
        " dp.value: %s\n"
        ,dev->info.product,
        dev->unid,
        dp->name,
        dp->address,
        value
    );

    // Setup write
    IdiActionCB aCB = (IdiActionCB){0};
    aCB.action = IdiaDpwrite;
    aCB.ReqIndex = request_index;
    aCB.dev = dev;
    aCB.dp = dp;
    aCB.prio = prio;
    aCB.relinquish = relinquish;
    aCB.rawStringValue = value;
    aCB.timeout = IDI_ACTION_NORMAL_TIMEOUT;
    if (mq_send(gDrvInfo.idiDevActQueue, (const char *)&aCB, sizeof(IdiActionCB), 1) != 0) {    
        err_printf("WARN: %s- Failed to send IdiaDpwrite to idiDevActQueue, err=%d\n", __FUNCTION__, errno);
        idlError = IErr_IdiBusy;
    }

    dbg_printf(" idlError: %s\n", IErrToStr(idlError));

    return idlError;
}

#ifdef AP_9580_WORKAROUND
/* OnUnrecColumnCb: An older callback function registered with the IDL Library which triggers  */
/* when a device datapoint is created by Idl. Typical usage of this callback is to process XIF */
/* custom/unrecognized column and setting up datapoint specific storage area.                  */
/* NOTE:                                                                                       */
/* Due to an EPR (Jira AP-9580) in Idl library , we have to continue registering this          */
/* unrecognized column callback routine via IdlDpUnrecColumnCallbackSet for the list of        */
/* unrecognized columns be reported in OnDpCreateCb callback routine.                          */
int OnUnrecColumnCb(int request_index, IdlDatapoint *dp, char *cpUnrecogCols)
{
    int idlError = IErr_Success;

    if (cpUnrecogCols) {
        cJSON *dpCustomColums = cJSON_Parse(cpUnrecogCols);
        if (dpCustomColums && cJSON_IsObject(dpCustomColums)) {
            cJSON *testMultCjson = cJSON_GetObjectItemCaseSensitive(dpCustomColums, "TestMultiplier");
            if (testMultCjson && cJSON_IsString(testMultCjson)) {
                dbg_printf("%s: got TestMultiplier column with value=%s\n", __FUNCTION__, 
                            testMultCjson->valuestring);
            } else {
                dbg_printf("%s: invalid or unable to find TestMultiplier in dpCustomColums for dp.name=%s\n", 
                            __FUNCTION__, dp->name);
                dbg_printf("%s: dpCustomColumns=%s\n", __FUNCTION__, cpUnrecogCols);
            }
            cJSON_free(testMultCjson);
        } else {
            err_printf("ERROR: %s- invalid dpCustomColums(%p) %s for dp.name=%s\n", __FUNCTION__, 
                        dpCustomColums, cpUnrecogCols, dp->name);
        }
        cJSON_free(dpCustomColums);
    }

    return idlError;
}
#endif


/* OnDpCreateCb: Callback function registered with the IDL Library which triggers when */
/* a device datapoint is created by Idl. Typical usage of this callback is to process  */
/* XIF custom/unrecognized column and setting up datapoint specific storage area.      */
int OnDpCreateCb(int  request_index, IdlDev *dev, IdlDatapoint *dp, char *cpUnrecogCols) 
{
    int idlError = IErr_Success;

    dbg_printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    dbg_printf(
        " dev.info.product: %s\n"				// Print out selected fields to the console
        " dev.unid: %s\n"
        " dp.name: %s\n"
        " dp.addr: %d\n"
        " dp.unrecognizedColumns: %s\n"
        ,dev->info.product,
        dev->unid,
        dp->name,
        dp->address,
        cpUnrecogCols
    );

    IdiActionCB aCB = (IdiActionCB){0};
    aCB.action = IdiaDpCreate;
    aCB.ReqIndex = request_index;
    aCB.dev = dev;
    aCB.dp = dp;
    aCB.cpUnrecogCols = cpUnrecogCols;
    aCB.timeout = IDI_ACTION_NORMAL_TIMEOUT;
    if (mq_send(gDrvInfo.idiDevActQueue, (const char *)&aCB, sizeof(IdiActionCB), 1) != 0) {    
        err_printf("WARN: %s- Failed to send IdiaDpCreate to idiDevActQueue, err=%d\n", __FUNCTION__, errno);
        idlError = IErr_IdiBusy;
    }

    dbg_printf(" idlError: %s\n", IErrToStr(idlError));
    
    return idlError;
}


/* OnDevCreateCb: Callback function registered with the IDL Library which triggers when */
/* a device of this protocol type is created.                                           */
int OnDevCreateCb(int request_index, IdlDev *dev, char *args, char *xif_dp_array)
{
    int idlError = IErr_Success;

    dbg_printf("\n%s:\n", __FUNCTION__);		// Print out the called function to the console
	
    dbg_printf(" dev.state: %d\n"				// Print out selected fields to the console
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
	  
    // Setup create action
    IdiActionCB aCB = (IdiActionCB){0};
    aCB.action = IdiaCreate;
    aCB.ReqIndex = request_index;
    aCB.dev = dev;
    aCB.args = args;
    aCB.timeout = IDI_ACTION_LONG_TIMEOUT;
    if (mq_send(gDrvInfo.idiDevActQueue, (const char *)&aCB, sizeof(IdiActionCB), 1) != 0) {    
        err_printf("WARN: %s- Failed to send IdiaCreate to idiDevActQueue, err=%d\n", __FUNCTION__, errno);
        idlError = IErr_IdiBusy;
    }

    dbg_printf(" idlError: %s\n", IErrToStr(idlError));

    return idlError;
}


/* OnDevProvisionCb: Callback function registered with the IDL Library which triggers when */
/* a request occurs to provision a device that uses this driver.                           */
int OnDevProvisionCb(int request_index, IdlDev *dev, char *args)
{
    int idlError = IErr_Success;

    dbg_printf("\n%s:\n", __FUNCTION__);		// Print out the called function to the console
	
    dbg_printf(" dev.state: %d\n"				// Print out selected fields to the console
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

    // Setup provision
    IdiActionCB aCB = (IdiActionCB){0};
    aCB.action = IdiaProvision;
    aCB.ReqIndex = request_index;
    aCB.dev = dev;
    aCB.args = args;
    aCB.timeout = IDI_ACTION_LONG_TIMEOUT;
    if (mq_send(gDrvInfo.idiDevActQueue, (const char *)&aCB, sizeof(IdiActionCB), 1) != 0) {    
        err_printf("WARN: %s- Failed to send IdiaProvision to idiDevActQueue, err=%d\n", __FUNCTION__, errno);
        idlError = IErr_IdiBusy;
    }

    dbg_printf(" idlError: %s\n", IErrToStr(idlError));

    return idlError;
}


/* OnDevDeprovisionCb: Callback function registered with the IDL Library which triggers when  */
/* a request occurs to deprovision a device that uses this driver.                            */
int OnDevDeprovisionCb(int request_index, IdlDev *dev)
{
    int idlError = IErr_Success;

    dbg_printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    dbg_printf(" dev.state: %d\n"				// Print out selected fields to the console
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

    /*
    *  Deprovision this device
    */
    IdiActionCB aCB = (IdiActionCB){0};
    aCB.action = IdiaDeprovision;
    aCB.ReqIndex = request_index;
    aCB.dev = dev;
    aCB.timeout = IDI_ACTION_LONG_TIMEOUT;
    if (mq_send(gDrvInfo.idiDevActQueue, (const char *)&aCB, sizeof(IdiActionCB), 1) != 0) {    
        err_printf("WARN: %s- Failed to send IdiaDeprovision to idiDevActQueue, err=%d\n", __FUNCTION__, errno);
        idlError = IErr_IdiBusy;
    }

    dbg_printf(" idlError: %s\n", IErrToStr(idlError));

    return idlError;
}


/* OnDevReplaceCb: Callback function registered with the IDL Library which triggers when */
/* a request occurs to replace a device that uses this driver.                           */
int OnDevReplaceCb(int request_index, IdlDev *dev, char *args)
{
    int idlError = IErr_Success;
		
    dbg_printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    dbg_printf(" dev.state: %d\n"				// Print out selected fields to the console
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

    // Setup replace
    IdiActionCB aCB = (IdiActionCB){0};
    aCB.action = IdiaReplace;
    aCB.ReqIndex = request_index;
    aCB.dev = dev;
    aCB.args = args;
    aCB.timeout = IDI_ACTION_LONG_TIMEOUT;
    if (mq_send(gDrvInfo.idiDevActQueue, (const char *)&aCB, sizeof(IdiActionCB), 1) != 0) {    
        err_printf("WARN: %s- Failed to send IdiaReplace to idiDevActQueue, err=%d\n", __FUNCTION__, errno);
        idlError = IErr_IdiBusy;
    }

    dbg_printf(" idlError: %s\n", IErrToStr(idlError));

    return idlError;
}


/* OnDevDeleteCb: Callback function registered with the IDL Library which triggers when  */
/* a request occurs to delete a device that uses this driver.                            */
int OnDevDeleteCb(int request_index, IdlDev *dev)
{
    int idlError = IErr_Success;
	
    dbg_printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    dbg_printf(" dev.state: %d\n"				// Print out selected fields to the console
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

    // Setup delete
    IdiActionCB aCB = (IdiActionCB){0};
    aCB.action = IdiaDelete;
    aCB.ReqIndex = request_index;
    aCB.dev = dev;
    aCB.timeout = IDI_ACTION_LONG_TIMEOUT;
    if (mq_send(gDrvInfo.idiDevActQueue, (const char *)&aCB, sizeof(IdiActionCB), 1) != 0) {    
        err_printf("WARN: %s- Failed to send IdiaDelete to idiDevActQueue, err=%d\n", __FUNCTION__, errno);
        idlError = IErr_IdiBusy;
    }

    dbg_printf(" idlError: %s\n", IErrToStr(idlError));

    return idlError;
}


//
// End of Custom Driver Callback functions
//


/* RandomInteger returns an integer in the range [0, n] used for simulating percentage
 *
 * Uses rand(), and so is affected-by/affects the same seed.
 */
uint RandomInteger(uint n) {
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


/* ConvertDoubleToJson: a function taken directly from Idl library to convert double  */
/* value to cJSON object.  The conversion include conversion of enum to cJSON string  */
static cJSON *ConvertDoubleToJson(IdlDatapoint *dp, double value)
{
    int i = 0;
    cJSON *dpValue = NULL;
    if (dp) {
        if (dp->info.iapEnum.enumMap) {
            for (i = 0; i < dp->info.iapEnum.count; i++) {
                if (value == dp->info.iapEnum.enumMap[i].value) {
                    dpValue = cJSON_CreateString(dp->info.iapEnum.enumMap[i].enumStr);
                    break;
                }
            }
            if (i == dp->info.iapEnum.count) {
                dpValue = cJSON_CreateNull();
            }
        } else {
            dpValue = cJSON_CreateNumber(value);
        }
    }
    return dpValue;
}


/* GenerateDpDefVal: a function taken directly from Idl library to return default    */
/* value in cJSON object.  The conversion include conversion of enum to cJSON string */
static cJSON *GenerateDpDefVal(IdlDatapoint *dp)
{
    cJSON *dfltJson = NULL;
    if (dp) {
        if (dp->info.dflt.isDefaultValid) {
            if (dp->info.isTypeAscii) {
                dfltJson = cJSON_CreateString(dp->info.dflt.stringValue);
            } else if (dp->info.isTypeNative) {
                dfltJson = IdlStringTocJSON(dp->info.dflt.nativeValue);
            } else {
                dfltJson = ConvertDoubleToJson(dp, dp->info.dflt.value);
            }
        } else {
            dfltJson = cJSON_CreateNumber(0);
        }
    }
    return dfltJson;
}


// GetDpValForLocStorUpdate: a function to return datapoint's actual value to be written
// in cJSON object.
static cJSON *GetDpValForLocStorUpdate(IdiActionCB& aCB) 
{
    cJSON *actualValue = NULL;
    if (aCB.dp) {
        if (aCB.dp->info.isTypeAscii) {
            actualValue = cJSON_CreateString(aCB.rawStringValue);
        } else if (aCB.dp->info.isTypeNative) {
            actualValue = IdlStringTocJSON(aCB.rawStringValue);
        } else {
            actualValue = ConvertDoubleToJson(aCB.dp, aCB.dValue);
        }
    }
    return actualValue;
}


#define TYPE_INVALID   1
#define TYPE_BELOW_MIN 2
#define TYPE_ABOVE_MAX 3

/* DpValueRangeCheck: a function taken directly from Idl library to check datapoint's  */
/* value range (valid rangeMin & rangeMax).  If passing a null for the exceptionType   */
/* or (isRangeMinValid or isRangeMaxValid is true) and value is not out of range, this */
/* function will return IErr_Success. Otherwise, it will return IErr_Failure           */
static int DpValueRangeCheck(IdlDatapoint *dp, double value, int *exceptionType)
{
    int idlError = IErr_Success;
    if (dp && exceptionType) {
        if  (dp->info.scale.isRangeMinValid && (value < (double)dp->info.scale.rangeMin)) {
            *exceptionType = TYPE_BELOW_MIN;
            idlError = IErr_Failure;
        } else if (dp->info.scale.isRangeMaxValid && (value > (double)dp->info.scale.rangeMax)) {
            *exceptionType = TYPE_ABOVE_MAX;
            idlError = IErr_Failure;
        }
    }
    return idlError;
}


// JsonDpValueToDouble: a function to convert datapoint Json value to a double.  
// The conversion include conversion of enum string to double */
static int JsonDpValueToDouble(cJSON *value, IdlDatapoint *dp, double *dValue)
{
    int i = 0;
    int idlError = IErr_Success;
    if (dp) {
        if (cJSON_IsString(value)) {
            if (strcmp(value->valuestring, INVALID_STR) == 0 && dp->info.scale.isInvalidPresent) {
                *dValue = dp->info.actualValue;
                dp->info.writeInvalidToDp = 1;
            } else if (dp->info.iapEnum.enumMap) {
                for (i = 0; i < dp->info.iapEnum.count; i++) {
                    if (strcasecmp(value->valuestring, dp->info.iapEnum.enumMap[i].enumStr) == 0) {
                        *dValue = dp->info.iapEnum.enumMap[i].value;
                        break;
                    }
                }
                if (i == dp->info.iapEnum.count) {
                    *dValue = dp->info.dflt.value; //will be 0 if default is not defined
                }
            } else {
                idlError = IErr_Failure;
            }
        } else if (cJSON_IsNumber(value) && DpValueRangeCheck(dp, value->valuedouble, NULL) == IDL_SUCCESS) {
            *dValue = value->valuedouble;
        } else {
            idlError = IErr_Failure;
        }
    } else {
        idlError = IErr_Failure;
    }
    return idlError;
}


// SetDpValueFromDpLocalStorage: a function to set datapoint actual value (actualStringValue,
// actualNativeValue, or double)
static int SetDpValueFromDpLocalStorage(IdiActionCB& aCB, T_DataPoint pcJsonDpVal, double *dValue)
{
    int idlError = IErr_Success;

    if (aCB.dp->info.isTypeAscii) {
        char *pTemp = aCB.dp->info.actualStringValue;
        if (aCB.dp->info.actualStringValue) {
            // free the current actualStringValue
            // dbg_printf("%s: isTypeAscii deallocate aCB.dp->info.actualStringValue (%p)\n", 
            //                  __FUNCTION__, (void *)aCB.dp->info.actualStringValue);
            IdlMemFree(aCB.dp->info.actualStringValue);
        }
        // let actualStringValue be freed by Idl library routine
        char *strValue = cJSON_PrintUnformatted(pcJsonDpVal);
        aCB.dp->info.actualStringValue = strValue;
        if (aCB.dp->info.rawStringValue && aCB.dp->info.rawStringValue != pTemp) {
            // free the current rawStringValue
            // dbg_printf("%s: isTypeAscii deallocate aCB.dp->info.rawStringValue (%p)\n", 
            //                 __FUNCTION__, (void *)aCB.dp->info.rawStringValue);
            IdlMemFree(aCB.dp->info.rawStringValue);
        } else {
            // prevent double free or corruption (fasttop)
            if (aCB.dp->info.rawStringValue)
                err_printf("WARN: %s- WHY old aCB.dp->info.actualStringValue(%p) == aCB.dp->info.rawStringValue (%p)\n",
                    __FUNCTION__, (void *)pTemp, (void *)aCB.dp->info.rawStringValue);
        }
        // let rawStringValue be freed by Idl library routine
        aCB.dp->info.rawStringValue = strdup(strValue);
        // dbg_printf("SetDpValueFromDpLocalStorage: setting actualStringValue=%s\n", aCB.dp->info.actualStringValue);
    } else if (aCB.dp->info.isTypeNative) {
        if (aCB.dp->info.rawStringValue) {
            // free the current rawStringValue
            // dbg_printf("%s: isTypeNative deallocate aCB.dp->info.rawStringValue (%p)\n", 
            //                  __FUNCTION__, (void *)aCB.dp->info.rawStringValue);
            IdlMemFree(aCB.dp->info.rawStringValue);
        }
        // let rawStringValue be freed by Idl library routine
        char *strValue = cJSON_PrintUnformatted(pcJsonDpVal);
        aCB.dp->info.rawStringValue = strValue;
        // dbg_printf("%s: isTypeNative new aCB.dp->info.rawStringValue (%p) = %s\n", 
        //         __FUNCTION__, (void *)aCB.dp->info.rawStringValue, aCB.dp->info.rawStringValue);
    } else {
        idlError = JsonDpValueToDouble(pcJsonDpVal, aCB.dp, dValue);
        // aCB.dp->info.actualValue = *dValue;
        // dbg_printf("%s: isTypeDouble setting dValue=%lf\n", __FUNCTION__, *dValue);
    }

    return idlError;
}


static int IdiDpProcessCustomColum(IdiActionCB& aCB)
{
    int idlError = IErr_Success;

    idlError = DpSetCustomIdiDpData(aCB.dev, aCB.dp);
    if (!idlError) {
        idlError = IErr_Failure;
        T_DpSto *pDpStruct = (T_DpSto *)aCB.dp->idiDpData;
        if (pDpStruct) {
            if (aCB.cpUnrecogCols) {
                cJSON *dpCustomColums = cJSON_Parse(aCB.cpUnrecogCols);
                if (dpCustomColums && cJSON_IsObject(dpCustomColums)) {
                    cJSON *testMultCjson = cJSON_GetObjectItemCaseSensitive(dpCustomColums, "TestMultiplier");
                    if (testMultCjson && cJSON_IsString(testMultCjson)) {
                        dbg_printf("%s: got TestMultiplier column with value=%s\n", __FUNCTION__, testMultCjson->valuestring);
                        char *pEnd;
                        pDpStruct->testMultiplier = strtod(testMultCjson->valuestring, &pEnd);
                        if (pDpStruct->testMultiplier == 0) {
                            // multiplier has to be non zero to prevent multiplying a value by 0
                            pDpStruct->testMultiplier = 1;
                        }
                        if (*pEnd) {
                            dbg_printf("%s: invalid TestMultiplier value=%s(%lf) in dpCustomColumns for dp.name=%s\n", 
                                        __FUNCTION__, testMultCjson->valuestring, pDpStruct->testMultiplier, aCB.dp->name);
                        }
                        idlError = IErr_Success;
                    } else {
                        err_printf("ERROR: %s- invalid or unable to find TestMultiplier in dpCustomColums for dp.name=%s\n", 
                                    __FUNCTION__, aCB.dp->name);

                        err_printf("ERROR: %s- dpCustomColumns=%s\n", __FUNCTION__, aCB.cpUnrecogCols);
                    }
                    cJSON_free(testMultCjson);
                } else {
                    err_printf("ERROR: %s- invalid dpCustomColums(%p) %s for dp.name=%s\n", __FUNCTION__, 
                                dpCustomColums, aCB.cpUnrecogCols, aCB.dp->name);
                }
                cJSON_free(dpCustomColums);
            } else {
                // We expect at least a custom column TestMultiplier in the XIF, if not initialize testMultipier to a 1
                pDpStruct->testMultiplier = 1;  // to prevent multiplying a value by 0
                err_printf("ERROR: %s- custom column testMultiplier is required for dp.name=%s\n", __FUNCTION__, aCB.dp->name);
            }
        } else {
            err_printf("ERROR: %s- invalid idiDevData for dp.name=%s - initialized in DevSetCustomIdiDevData\n", __FUNCTION__, aCB.dp->name);
        }
    }

    return idlError;
}


/* ProcAsynThrdFunc: Example thread called from main.cpp to allow    */
/*  typical custom driver to receive and process async incoming I/O  */
/*  data. In this example, this routine is used to drive an FSM code */
/*  to simulate asynchronous processing of any callback routines.    */
void *ProcAsynThrdFunc(void* pvArg)
{
	int retVal = SUCCESS;
    char qName[Q_NAME_LENGTH];
    IdiActionCB aCB = {0};
    
    pthread_setname_np(pthread_self(), __FUNCTION__);       // <= 16 chars
    info_printf("INFO: The " CDNAME " IDI Process Asynchronous Requests driver thread started...\r\n");

    srand(time(NULL));   // Initialization, should only be called once.
    sprintf(qName, IDI_ACT_Q, CDNAME);
	retVal = IdiCreateQueue(&gDrvInfo.idiDevActQueue, qName, BLOCKING_Q, MQ_HARD_LIM, sizeof(IdiActionCB));
	if (retVal != SUCCESS) {
        err_printf("ERROR %s: IdiCreateQueue %s failed\n", __FUNCTION__, qName);
        exit (EXIT_FAILURE);
	} else {
		info_printf("INFO: IdiCreateQueue %s successful\n", qName);

        // service the device action queue here until being told to stop
        gDrvInfo.stat = IdiRunning;
        while (gDrvInfo.stat != IdiStop) {
            retVal = mq_receive(gDrvInfo.idiDevActQueue, (char*)&aCB, sizeof(IdiActionCB), NULL);
            if(retVal == -1) {
                continue;
            }

            IdiBlockWhileBusy(aCB);

            /* clean-up */
            aCB = {0};
        }


    }

    return NULL;
}

/* IsFsmProcessingDone: a function returning true when processing is done */
/*   This routine typically implement a state machine for processing aCBs */
bool IsFsmProcessingDone(IdiActionCB& aCB) {
#ifdef ENABLE_SIMULATION
    // returning true 60% of a time to simulate a busy condition
    uint digit =  RandomInteger(9);
    // dbg_printf("random digit: %u\n", digit);
    return (digit < 6) ? true : false;
#else
    return true;
#endif
}

/* IdiGenericResultFsm: Example routine implementing a finite state machine */
/* to simulate asynchronous processing of any registered callback routines  */
/* and finally call the Action Callback Result to let Idl know we are done. */
int IdiGenericResultFsm(IdiActionCB& aCB)
{
    int idlError = IErr_Success;

    switch(aCB.action) {
    case IdiaNone:
        break;
    case IdiaDpCreate:
        if (aCB.dev && aCB.dp) {
            /*
            *  Process a device datapoint creation
            */
            if (!IsFsmProcessingDone(aCB)) {
                idlError = IErr_IdiBusy;
            } else {
                // processing dp custom column
                idlError = IdiDpProcessCustomColum(aCB);
            }
        } else {
            err_printf("ERROR: %s- Malformed IdlDev", __FUNCTION__);
            idlError = IErr_Failure;
        }
        // there is no IdlDpCreateResult to call..
        break;
    case IdiaCreate:
        if (aCB.dev && aCB.dev->handle) {
            /*
            *  Create a device
            */
            if (!IsFsmProcessingDone(aCB)) {
                idlError = IErr_IdiBusy;
            } else {
                // allocate per device's storage
                idlError = DevSetCustomIdiDevData(aCB.dev);
            }
        } else {
            err_printf("ERROR: %s- Malformed IdlDev", __FUNCTION__);
            idlError = IErr_Failure;
        }

        if (idlError != IErr_IdiBusy) {
            if (aCB.args) {
                IdiFree(aCB.args);
            }
            IdlDevCreateResult(aCB.ReqIndex, aCB.dev, idlError);
        }
        break;
    case IdiaDelete:
        if (aCB.dev && aCB.dev->handle) {
            /*
            *  Delete a device
            */
            if (!IsFsmProcessingDone(aCB)) {
                idlError = IErr_IdiBusy;
            } else {
                // free up per device's storage
                idlError = DevRemoveCustomIdiDevData(aCB.dev);
            }
        } else {
            err_printf("ERROR: %s- Malformed IdlDev", __FUNCTION__);
            idlError = IErr_Failure;
        }

        if (idlError != IErr_IdiBusy) {
            if (aCB.args) {
                IdiFree(aCB.args);
            }
            IdlDevDeleteResult(aCB.ReqIndex, aCB.dev, idlError);
        }
        break;
    case IdiaReplace:
        if (aCB.dev && aCB.dev->handle) {
            if (!IsFsmProcessingDone(aCB)) {
                idlError = IErr_IdiBusy;
            } else {
                /*
                *  Replace a device: may involve deprovision old device/unid
                */
                DevReplaceUnid(aCB.dev);
            }
        } else {
            err_printf("ERROR: %s- Malformed IdlDev", __FUNCTION__);
            idlError = IErr_Failure;
        }

        if (idlError != IErr_IdiBusy) {
            if (aCB.args) {
                IdiFree(aCB.args);
            }
            IdlDevReplaceResult(aCB.ReqIndex, aCB.dev, idlError);
        }
        break;
    case IdiaDpwrite:
        /*
        *  Do our write
        */
        if (IsFsmProcessingDone(aCB)) {
            T_DpSto *pDpStruc = (T_DpSto *)(aCB.dp->idiDpData);
            if (pDpStruc) {
                if (*pDpStruc->pDpValue) {
                    // free existing Cjson object from the dp entry in per device's DevDpValue vector
                    IdlCjsonDelete(*pDpStruc->pDpValue);
                }

                // update dp entry in in per device's DevDpValue vector
                *pDpStruc->pDpValue = GetDpValForLocStorUpdate(aCB);
                char *outStr = cJSON_PrintUnformatted(*pDpStruc->pDpValue);
                dbg_printf(" Value written: %s at pDpStruc(%p)->pDpValue = %p\n", 
                            outStr, pDpStruc, pDpStruc->pDpValue);

#ifdef INCLUDE_ETI
                uint reg = pDpStruc->address;
                T__DevStoPtr pDevEntry = (T__DevStoPtr)(aCB.dev->idiDevData);
                idlError = DevWritePublish(pDevEntry, reg, *pDpStruc->pDpValue);
#endif
                IdlMemFree(outStr);
            } else {
                idlError = IErr_Failure;
                err_printf("ERROR: %s- Write error on unitialized idiDpData\n", __FUNCTION__);
            }
            if (aCB.args) {
                IdiFree(aCB.args);
            }
            IdlDpWriteResult(aCB.ReqIndex, aCB.dev, aCB.dp, idlError);
        } else {
            idlError = IErr_IdiBusy;
        }
        break;
    case IdiaDpread:
        /*
        *  Do our read
        */
        if (IsFsmProcessingDone(aCB)) {
            T_DpSto *pDpStruc = (T_DpSto *)(aCB.dp->idiDpData);
            double dpValue = 0;
            if (pDpStruc) {
#ifdef INCLUDE_ETI
                uint reg = pDpStruc->address;
                T__DevStoPtr pDevEntry = (T__DevStoPtr)(aCB.dev->idiDevData);
                DevReadPublish(pDevEntry, reg);
#endif
                if (*pDpStruc->pDpValue) {
                    char *jsonStr = cJSON_PrintUnformatted(*pDpStruc->pDpValue);
                    idlError = SetDpValueFromDpLocalStorage(aCB, *pDpStruc->pDpValue, &dpValue);
                    if (pDpStruc->testMultiplier != 0)
                        dpValue *= pDpStruc->testMultiplier;
                    if (idlError == IErr_Success) {
                        /* 
                        *    dbg_printf("%s Value read: %s (before applying multiplier: %lf) at pDpStruc(%p)->pDpValue = %p\n", 
                        *         (aCB.lastError == IErr_IdiBusy) ? ".done\n" : "", 
                        *         jsonStr, pDpStruc->testMultiplier, pDpStruc, pDpStruc->pDpValue);
                        */
                    } else {
                        err_printf("ERROR: %s- Unable to read dp entry in localDpValuesVector\n", __FUNCTION__);
                    }
                    IdlMemFree(jsonStr);
                } else {
                    idlError = IErr_Failure;
                    err_printf("ERROR: %s- No value found for dp entry in localDpValuesVector\n", __FUNCTION__);
                }
            } else {
                idlError = IErr_Failure;
                err_printf("ERROR: %s- Unitialized idiDpData - device possibly has been deleted!\n", __FUNCTION__);
            }
            if (aCB.args) {
                IdiFree(aCB.args);
            }
            IdlDpReadResult(aCB.ReqIndex, aCB.dev, aCB.dp, aCB.context, idlError, prio_array, dpValue);
        } else {
            idlError = IErr_IdiBusy;
        }
        break;
    case IdiaProvision:
        /*
            *  Provision our device
            */
        if (IsFsmProcessingDone(aCB)) {
            if (aCB.args) {
                IdiFree(aCB.args);
            }
            IdlDevProvisionResult(aCB.ReqIndex, aCB.dev, IErr_Success);
        } else {
            idlError = IErr_IdiBusy;
        }
        break;
    case IdiaDeprovision:
        /*
            *  Deprovision our device
            */
        if (IsFsmProcessingDone(aCB)) {
            if (aCB.args) {
                IdiFree(aCB.args);
            }
            IdlDevDeprovisionResult(aCB.ReqIndex, aCB.dev, IErr_Success);
        } else {
            idlError = IErr_IdiBusy;
        }
        break;
    default:
        break;
    }

    aCB.lastError = idlError;       // remember last processing error
    return idlError;
}


int IdiBlockWhileBusy(IdiActionCB& aCB) {
    int idlError = IErr_Success;

    if (aCB.action != IdiaNone) {
        time_t timeStart = time(NULL);
        while (aCB.action != IdiaNone) {
            idlError = IdiGenericResultFsm(aCB);
            if (idlError != IErr_IdiBusy) {
                aCB.action = IdiaNone;     // done, clear the action for a new callback action.
                break;
            } else {
                if ((uint) (time(NULL) - timeStart) >= aCB.timeout) {
                    idlError = IErr_Failure;
                    err_printf("\nERROR %s- Timed out on UNID: %s, action: %d, idlError: %s\n", __FUNCTION__, 
                                    (aCB.dev->unid) ? aCB.dev->unid : "NULL", aCB.action, IErrToStr(idlError));
                    break;
                }
            }
        }
    }
    return idlError;
}


