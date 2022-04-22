#include "common.h"
#include "example.h"


#ifndef CDNAME
#error CDNAME must be defined in Makefile
#endif
#ifndef CDDEVLIMIT
#error CDDEVLIMIT must be defined in Makefile
#endif
#ifndef CDPROGRAMIDS
#error CDPROGRAMIDS must be defined in Makefile
#endif


extern Idl *idl;

// For this example there is only one activeCB.  In real-life driver this
// should be a linked list of activeCBs and the IdiProcAsynchThreadFunction
// will cycle through the list of activeCBs until no more available.
static IdiActiveCB activeCB;

static T_DrvInfo gDrvInfo = {};


// Dummy value and priority array for sake of this driver
// Would be read/writing to designated datapoint instead

static char *prio_array = NULL;


static int IdiBlockWhileBusy(IdiActiveCB& pActCb);
static int IdiGenericResultFsm(IdiActiveCB& pActCb);
static cJSON *GetDpValueForDpLocalStorage(IdiActiveCB& pActCb);
static cJSON *GenerateDpDefaultObject(IdlDatapoint *dp);


/* IdiStart: Custom driver startup function called from main.cpp  */

int IdiStart() 
{
	/* Your custom IDL driver can start up any other driver-specific  */
	/* actions here.  For example, you could open a connection to a   */
	/* serial port or a USB interface.  You should return a 0 here    */
	/* if your code started up your driver properly or else return 1. */

	printf("The " CDNAME " IDL driver is connected and ready...\n");

#ifdef INCLUDE_ETI
    EtiInit(&gDrvInfo);
#endif

    return 0;
}

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
                printf("%s: pDpStruct = %p, pDpStruct->pDpValue = %p\n", __FUNCTION__, pDpStruct, pDpStruct->pDpValue);
                // check if the pDevDpValVector[dp->address] entry has possibly been initialized by other 
                // datapoints with the same address (defined in device's XIF)
                if (!pDevEntry->pDevDpValVector[address]) {
                    cJSON *pDefaultJSON = GenerateDpDefaultObject(dp);
                    pDevEntry->pDevDpValVector[address] = pDefaultJSON;

                    char *jsonStr = cJSON_PrintUnformatted(*pDpStruct->pDpValue);
                    printf("%s: dp.name=%s, dp.address=%d, devDpEntry=%d, &pDevEntry->pDevDpValVector[dp->address]=%p, dpValue=%s\n",
                                __FUNCTION__, dp->name, address, pDevEntry->devDpEntry, 
                                &pDevEntry->pDevDpValVector[address], jsonStr);
                    IdlMemFree(jsonStr);
                }
                // set idiDpData to point to an entry in per device's DevDpStorage, record the address & increment the datapoint entry
                pDpStruct->address = address;
                dp->idiDpData = pDpStruct;
                pDevEntry->devDpEntry++;
            }
            else {
                printf("%s: devDpEntry for dp.name=%s exceeded devDpCounts(%d) - initialized in DevSetCustomIdiDevData\n", 
                        __FUNCTION__, dp->name, pDevEntry->devDpCounts);
                idlError = IErr_Failure;
            }
        }
        else {
            printf("%s: invalid idiDevData for dp.name=%s - initialized in DevSetCustomIdiDevData\n", __FUNCTION__, dp->name);
            idlError = IErr_Failure;
        }
    }
    else {
        printf("%s: idiDpData for dp.name=%s has already been initialized with value\n", __FUNCTION__, dp->name);
    }

    return idlError;
}


// checkSupportedProgramIds: returns IErr_TypeInvalid if driver doesn't support this device type.
static int checkSupportedProgramIds(IdlDev *dev)
{
    static const char *pSupportedProgramIDs[] = CDPROGRAMIDS;

    int idlError = IErr_TypeInvalid;
    for (auto *pid : pSupportedProgramIDs) {
        if (strcasecmp(dev->type, pid) == 0) {
            idlError = IErr_Success;
            break;
        }
    }

    return idlError;
}


static int DevReplaceUnid(IdlDev *dev) {
    int idlError = IErr_Failure;

    // point to the allocated storage
    T__DevStoPtr pLocDevStorageStruc = (T__DevStoPtr)dev->idiDevData;
    if (pLocDevStorageStruc) {
        strncpy(pLocDevStorageStruc->devUid, dev->unid, sizeof(pLocDevStorageStruc->devUid));
        pLocDevStorageStruc->devUid[MAX_UNID_CHARS] = '\0'; // forced string termination
        idlError = IErr_Success;
    }
    return idlError;
}


static int DevSetCustomIdiDevData(IdlDev *dev)
{
    uint dpCount = 0;

    int idlError = checkSupportedProgramIds(dev);
    if (idlError == IErr_Success) {
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
                    printf("%s: allocated per device DevStorage structure (%p)for local storage\n", __FUNCTION__, pLocDevStorageStruc);
                    // allocate per device device datapoint structure vector for local storage 
                    T_DpStoVector pDevDpVector = (T_DpStoVector)calloc(dpCount, sizeof(T_DpSto));
                    if (pDevDpVector) {
                        printf("%s: allocated per device datapoint structure vector (%p)for local storage\n", __FUNCTION__, pDevDpVector);
                        pLocDevStorageStruc->pDevDpVector = pDevDpVector;         // point to the allocated storage

                        // allocate per device device datapoint value vector for local storage 
                        T_DataPoint *pDevDpValVector = (T_DataPoint *)calloc(dpCount, sizeof(T_DataPoint));
                        if (pDevDpValVector) {
                            printf("%s: allocated per device datapoint value vector (%p)for local storage\n", __FUNCTION__, pDevDpValVector);
                            pLocDevStorageStruc->devDpCounts = dpCount;
                            pLocDevStorageStruc->pDevDpValVector = pDevDpValVector; // point to the allocated storage

                            T_DevNodePtr pNewNode = (T_DevNodePtr)calloc(1, sizeof(T_DevNode));
                            if (pNewNode) {
                                pNewNode->pDevSto = pLocDevStorageStruc;
                                // insert to the end of the list
                                if (gDrvInfo.pHeadDevNode == NULL) {
                                    pNewNode->pNext = pNewNode->pPrevious = pNewNode;
                                    gDrvInfo.pHeadDevNode = pNewNode;
                                }
                                else {
                                    pNewNode->pNext     = gDrvInfo.pHeadDevNode;
                                    pNewNode->pPrevious = gDrvInfo.pHeadDevNode->pPrevious;
                                    gDrvInfo.pHeadDevNode->pPrevious->pNext = pNewNode;
                                    gDrvInfo.pHeadDevNode->pPrevious = pNewNode;
                                }
                                pNewNode->pDevSto->pDrvInfo = &gDrvInfo;
                            }
                            // set idiDevData to the per device DevStorage Structure & increment device count
                            dev->idiDevData = (void *)pLocDevStorageStruc;          // point to the allocated storage
                            strncpy(pLocDevStorageStruc->devUid, dev->unid, sizeof(pLocDevStorageStruc->devUid));
                            pLocDevStorageStruc->devUid[MAX_UNID_CHARS] = '\0'; // forced string termination
                            gDrvInfo.deviceEntry++;
                            idlError = IErr_Success;
                        }
                        else {
                            printf("%s: failed to allocate per device datapoint value vector for local storage\n", __FUNCTION__);
                        }
                    }
                    else {
                        printf("%s: failed to allocate per device datapoint structure vector for local storage\n", __FUNCTION__);
                    }
                }
                else {
                        printf("%s: failed to allocate per device DevStorage structure for local storage\n", __FUNCTION__);
                }
            }
            else {
                printf("%s: Can't set custom_idiDevData, it has already been set!", __FUNCTION__);
            }
        }
        else {
            printf("%s: device count exceeded precompiled max number (%d) defined in device idl configuration\n", 
                        __FUNCTION__, CDDEVLIMIT);
        }
    }
    else {
        printf("%s: device type/program id %s is not supported by this " CDNAME " driver\n", __FUNCTION__, dev->type);
    }

    return idlError;
}


static int DevRemoveCustomIdiDevData(IdlDev *dev)
{
    int idlError = IErr_Failure;

    if (dev->idiDevData) {
        // clear the per datapoint idiDpData
        IdlInterfaceBlock *ifblock = dev->firstIfblock;
        while (ifblock) {
            IdlIapDatapoint *iapdp = ifblock->firstIapdp;
            while (iapdp) {
                iapdp->firstField->dp->idiDpData = NULL;
                iapdp = iapdp->next;
            }
            ifblock = ifblock->next;
        }

        // deallocate per device DeviceStorageStruc, dp storage vector, dp value storage vector & update gDrvInfo.pHeadDevNode list
        T_DevNodePtr pCurNode = gDrvInfo.pHeadDevNode;
        while (pCurNode) {
            if (pCurNode->pDevSto == (T__DevStoPtr)dev->idiDevData) {
                // we found the device storage node
                if (pCurNode == gDrvInfo.pHeadDevNode) {
                    // we are removing the head node
                    if (pCurNode->pNext == pCurNode->pPrevious) {
                        // we are removing the only node
                        gDrvInfo.pHeadDevNode = NULL;
                    }
                    else {
                        // we are moving the head to the next node
                        gDrvInfo.pHeadDevNode = pCurNode->pNext;
                        pCurNode->pPrevious->pNext = pCurNode->pNext;
                        pCurNode->pNext->pPrevious = pCurNode->pPrevious;
                    }
                }
                else {
                    pCurNode->pPrevious->pNext = pCurNode->pNext;
                    pCurNode->pNext->pPrevious = pCurNode->pPrevious;
                }

                printf("\n%s: deallocate per device datapoint structure vector  (%p)for local storage\n", __FUNCTION__, 
                            pCurNode->pDevSto->pDevDpVector);
                IdlMemFree( pCurNode->pDevSto->pDevDpVector );
                printf("\n%s: deallocate per device datapoint value vector (%p)for local storage\n", __FUNCTION__, 
                            pCurNode->pDevSto->pDevDpValVector );
                IdlMemFree( pCurNode->pDevSto->pDevDpValVector );
                printf("\n%s: deallocate per device DevStorage structure (%p)for local storage\n", __FUNCTION__, 
                            (dev->idiDevData));
                IdlMemFree( (dev->idiDevData));
                IdlMemFree(pCurNode);   // free the device storage node
                dev->idiDevData = NULL;
                gDrvInfo.deviceEntry--;

                idlError = IErr_Success;
                break;  // we are done
            }
            pCurNode = pCurNode->pNext;
            if (pCurNode == gDrvInfo.pHeadDevNode) {
                printf("\n%s: Unable to find the entry in gDrvInfo.pHeadDevNode list to clear custom_idiDevData!", __FUNCTION__);
                break;  // we have exhausted the search
            }
        }
    }
    else {
        printf("\n%s: Can't clear custom_idiDevData, it has not been set!", __FUNCTION__);
    }

    return idlError;
}


/* OnDpReadCb: Callback function registered with the IDL Library which triggers */
/* when a data point read occurs.  This implementation simulates a check if the */
/* custom driver (idi) is busy and if not it simulates a dp read.               */

int OnDpReadCb(int request_index, IdlDev *dev, IdlDatapoint *dp, void *context)
{
    int idlError = IErr_Success;
    printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    printf(" dev.info.product: %s\n"				// Print out selected fields to the console
        " dev.unid: %s\n"
        " dp.name: %s\n"
        " dp.addr: %d\n"
        ,dev->info.product,
        dev->unid,
        dp->name,
        dp->address
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

/* OnDpReadExCb: Callback function registered with the IDL Library which  */
/* triggers when a data point of type ascii or structured native read occurs. */
/* This implementation simulates a check if the custom driver (idi) is busy   */
/* and if not it simulates a dp read.                                         */
int OnDpReadExCb(int request_index, IdlDev *dev, IdlDatapoint *dp, void *context)
{

    int idlError = IErr_Success;
    printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    printf(" dev.info.product: %s\n"				// Print out selected fields to the console
        " dev.unid: %s\n"
        " dp.name: %s\n"
        " dp.addr: %d\n"
        ,dev->info.product,
        dev->unid,
        dp->name,
        dp->address
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

/* OnDpWriteCb: Callback function registered with the IDL Library which triggers */
/* when a data point write occurs.  This implementation simulates a check if the */
/* custom driver (idi) is busy and if not it simulates a dp write.               */
int OnDpWriteCb(int request_index, IdlDev *dev, IdlDatapoint *dp, int prio, int relinquish, double value)
{
    int idlError = IErr_Success;

    printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    printf(" dev.info.product: %s\n"				// Print out selected fields to the console
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

    if (activeCB.action == IdiaNone) {
        // Setup write
        activeCB = (IdiActiveCB){0};
        activeCB.action = IdiaDpwrite;
        activeCB.ReqIndex = request_index;
        activeCB.dev = dev;
        activeCB.dp = dp;
        activeCB.prio = prio;
        activeCB.relinquish = relinquish;
        activeCB.dValue = value;
        activeCB.timeout = IDI_ACTION_NORMAL_TIMEOUT;
    }
    else {
        // ideally this should not happen if we implement adaquate queue of activeCBs
        idlError = IErr_IdiBusy;
    }

    return idlError;
}

/* OnDpWriteExCb: Callback function registered with the IDL Library which triggers when a */
/* data point of type ascii or structured native write occurs.  This implementation simulates */
/* a check if the custom driver (idi) is busy and if not it simulates a dp write.             */
int OnDpWriteExCb(int request_index, IdlDev *dev, IdlDatapoint *dp, int prio, 
                      int relinquish, char *value)
{
    int idlError = IErr_Success;

    printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    printf(" dev.info.product: %s\n"				// Print out selected fields to the console
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

    if (activeCB.action == IdiaNone) {
        // Setup write
        activeCB = (IdiActiveCB){0};
        activeCB.action = IdiaDpwrite;
        activeCB.ReqIndex = request_index;
        activeCB.dev = dev;
        activeCB.dp = dp;
        activeCB.prio = prio;
        activeCB.relinquish = relinquish;
        activeCB.rawStringValue = value;
        activeCB.timeout = IDI_ACTION_NORMAL_TIMEOUT;
    }
    else {
        // ideally this should not happen if we implement adaquate queue of activeCBs
        idlError = IErr_IdiBusy;
    }

    return idlError;
}

#ifdef AP_9580_WORKAROUND
int OnUnrecColumnCb(int request_index, IdlDatapoint *dp, char *cpUnrecogCols)
{
    int idlError = IErr_Success;

    if (cpUnrecogCols) {
        cJSON *dpCustomColums = cJSON_Parse(cpUnrecogCols);
        if (dpCustomColums && cJSON_IsObject(dpCustomColums)) {
            cJSON *myTestMultiplier = cJSON_GetObjectItemCaseSensitive(dpCustomColums, "TestMultiplier");
            if (myTestMultiplier && cJSON_IsString(myTestMultiplier)) {
                printf("%s: got TestMultiplier column with value=%s\n", __FUNCTION__, myTestMultiplier->valuestring);
            }
            else {
                printf("%s: invalid or unable to find TestMultiplier in dpCustomColums for dp.name=%s\n", __FUNCTION__, dp->name);
                printf("%s: dpCustomColumns=%s\n", __FUNCTION__, cpUnrecogCols);
            }
            cJSON_free(myTestMultiplier);
        }
        else {
            printf("%s: invalid dpCustomColums(%p) %s for dp.name=%s\n", __FUNCTION__, dpCustomColums, cpUnrecogCols, dp->name);
        }
        cJSON_free(dpCustomColums);
    }

    return idlError;
}
#endif

int OnDpCreateCb(int  request_index, IdlDev *dev, IdlDatapoint *dp, char *cpUnrecogCols) 
{
    int idlError = IErr_Success;

    printf("\n%s:\n", __FUNCTION__);			// Print out the called function to the console
	
    printf(
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

    idlError = DpSetCustomIdiDpData(dev, dp);
    if (!idlError) {
        T_DpSto *pDpStruct = (T_DpSto *)dp->idiDpData;
        if (pDpStruct) {
            if (cpUnrecogCols) {
                cJSON *dpCustomColums = cJSON_Parse(cpUnrecogCols);
                if (dpCustomColums && cJSON_IsObject(dpCustomColums)) {
                    cJSON *myTestMultiplier = cJSON_GetObjectItemCaseSensitive(dpCustomColums, "TestMultiplier");
                    if (myTestMultiplier && cJSON_IsString(myTestMultiplier)) {
                        printf("%s: got TestMultiplier column with value=%s\n", __FUNCTION__, myTestMultiplier->valuestring);
                        char *pEnd;
                        pDpStruct->testMultiplier = strtod(myTestMultiplier->valuestring, &pEnd);
                        if (pDpStruct->testMultiplier == 0) {
                            // multiplier has to be non zero to prevent multiplying a value by 0
                            pDpStruct->testMultiplier = 1;
                        }
                        if (*pEnd) {
                            printf("%s: invalid TestMultiplier value=%s(%lf) in dpCustomColumns for dp.name=%s\n", 
                                        __FUNCTION__, myTestMultiplier->valuestring, pDpStruct->testMultiplier, dp->name);
                        }
                    }
                    else {
                        printf("%s: invalid or unable to find TestMultiplier in dpCustomColums for dp.name=%s\n", __FUNCTION__, dp->name);

                        printf("%s: dpCustomColumns=%s\n", __FUNCTION__, cpUnrecogCols);
                    }
                    cJSON_free(myTestMultiplier);
                }
                else {
                    printf("%s: invalid dpCustomColums(%p) %s for dp.name=%s\n", __FUNCTION__, dpCustomColums, cpUnrecogCols, dp->name);
                }
                cJSON_free(dpCustomColums);
            }
            else {
                // We expect at least a custom column TestMultiplier in the XIF, if not initialize testMultipier to a 1
                pDpStruct->testMultiplier = 1;  // to prevent multiplying a value by 0
                printf("Error %s: custom column testMultiplier is required for dp.name=%s\n", __FUNCTION__, dp->name);
            }
        }
        else {
            printf("%s: invalid idiDevData for dp.name=%s - initialized in DevSetCustomIdiDevData\n", __FUNCTION__, dp->name);
        }
    }
    
    return idlError;
}


/* OnDevCreateCb: Callback function registered with the IDL Library which triggers when a */
/* device of this protocol type is created.  This implementation simulates a check if the */
/* custom driver (idi) is busy and if not it simulates a device creation.                 */
int OnDevCreateCb(int request_index, IdlDev *dev, char *args, char *xif_dp_array)
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
	  
    idlError = DevSetCustomIdiDevData(dev);
    if (idlError == IErr_Success) {
        if (activeCB.action == IdiaNone) {
            if (idlError == IErr_Success) {
                // Setup provision
                activeCB = (IdiActiveCB){0};
                activeCB.action = IdiaCreate;
                activeCB.ReqIndex = request_index;
                activeCB.dev = dev;
                activeCB.args = args;
                activeCB.timeout = IDI_ACTION_LONG_TIMEOUT;
            }
        }
        else {
            // ideally this should not happen if we implement adaquate queue of activeCBs
            idlError = IErr_IdiBusy;
        }
    }
    else {
        IdlDevCreateResult(request_index, dev, idlError);
    }

    return idlError;
}

/* OnDevProvisionCb: Callback function registered with the IDL Library which triggers when a  */
/* request occurs to provision a device that uses this driver.  This implementation simulates */
/* a check if the custom driver (idi) is busy and if not it simulates a device provisioning.  */
int OnDevProvisionCb(int request_index, IdlDev *dev, char *args)
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

/* OnDevDeprovisionCb: Callback function registered with the IDL Library which triggers when a  */
/* request occurs to deprovision a device that uses this driver.  This implementation simulates */
/* a check if the custom driver (idi) is busy and if not it simulates a device deprovisioning.  */
int OnDevDeprovisionCb(int request_index, IdlDev *dev)
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

/* OnDevReplaceCb: Callback function registered with the IDL Library which triggers when a  */
/* request occurs to replace a device that uses this driver.  This implementation simulates */
/* a check if the custom driver (idi) is busy and if not it simulates a device replacement. */
int OnDevReplaceCb(int request_index, IdlDev *dev, char *args)
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

    idlError = DevSetCustomIdiDevData(dev);
    if (idlError == IErr_Success) {
        if (activeCB.action == IdiaNone) {
            // Setup replace

            /*
            *  Slot in the new device and deprovision the old device.
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
    }
    else {
        IdlDevReplaceResult(request_index, dev, idlError);
    }

    return idlError;
}

/* OnDevDeleteCb: Callback function registered with the IDL Library which triggers when a  */
/* request occurs to delete a device that uses this driver.  This implementation simulates */
/* a check if the custom driver (idi) is busy and if not it simulates a device deletion.   */
int OnDevDeleteCb(int request_index, IdlDev *dev)
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


// End of Custom Driver Callback functions



/* Returns an integer in the range [0, n] used for simulating percentage calculation.
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
bool IsSimulateProcessingDone() {
    uint digit =  randint(9);
    // printf("random digit: %u\n", digit);
    return (digit < 6) ? true : false;
}

/* ConvertDoubleValueToJson: a function taken directly from Idl library to convert double  */
/* value to cJSON object.  The conversion include conversion of enum to cJSON string       */
static cJSON *ConvertDoubleValueToJson(IdlDatapoint *dp, double value)
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

/* GenerateDpDefaultObject: a function taken directly from Idl library to return default */
/* value in cJSON object.  The conversion include conversion of enum to cJSON string     */
static cJSON *GenerateDpDefaultObject(IdlDatapoint *dp)
{
    cJSON *dfltJson = NULL;
    if (dp) {
        if (dp->info.dflt.isDefaultValid) {
            if (dp->info.isTypeAscii) {
                dfltJson = cJSON_CreateString(dp->info.dflt.stringValue);
            } else if (dp->info.isTypeNative) {
                dfltJson = IdlStringTocJSON(dp->info.dflt.nativeValue);
            } else {
                dfltJson = ConvertDoubleValueToJson(dp, dp->info.dflt.value);
            }
        } else {
            dfltJson = cJSON_CreateNumber(0);
        }
    }
    return dfltJson;
}


// GetDpValueForDpLocalStorage: a function to return datapoint's actual value to be written
// in cJSON object.
static cJSON *GetDpValueForDpLocalStorage(IdiActiveCB& aCB) 
{
    cJSON *actualValue = NULL;
    if (aCB.dp) {
        if (aCB.dp->info.isTypeAscii) {
            actualValue = cJSON_CreateString(aCB.rawStringValue);
        } else if (aCB.dp->info.isTypeNative) {
            actualValue = IdlStringTocJSON(aCB.rawStringValue);
        } else {
            actualValue = ConvertDoubleValueToJson(aCB.dp, aCB.dValue);
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
static int SetDpValueFromDpLocalStorage(IdiActiveCB& aCB, T_DataPoint pcJsonDpVal, double *dValue)
{
    int idlError = IErr_Success;

    if (aCB.dp->info.isTypeAscii) {
        char *pTemp = aCB.dp->info.actualStringValue;
        if (aCB.dp->info.actualStringValue) {
            // free the current actualStringValue
            printf("%s: isTypeAscii deallocate aCB.dp->info.actualStringValue (%p)\n", __FUNCTION__, (void *)aCB.dp->info.actualStringValue);
            IdlMemFree(aCB.dp->info.actualStringValue);
        }
        // let actualStringValue be freed by Idl library routine
        char *strValue = cJSON_PrintUnformatted(pcJsonDpVal);
        aCB.dp->info.actualStringValue = strValue;
        if (aCB.dp->info.rawStringValue && aCB.dp->info.rawStringValue != pTemp) {
            // free the current rawStringValue
            printf("%s: isTypeAscii deallocate aCB.dp->info.rawStringValue (%p)\n", __FUNCTION__, (void *)aCB.dp->info.rawStringValue);
            IdlMemFree(aCB.dp->info.rawStringValue);
        }
        else {
            // prevent double free or corruption (fasttop)
            if (aCB.dp->info.rawStringValue)
                printf("%s: WHY old aCB.dp->info.actualStringValue(%p) == aCB.dp->info.rawStringValue (%p)\n",
                    __FUNCTION__, (void *)pTemp, (void *)aCB.dp->info.rawStringValue);
        }
        // let rawStringValue be freed by Idl library routine
        aCB.dp->info.rawStringValue = strdup(strValue);
        // printf("SetDpValueFromDpLocalStorage: setting actualStringValue=%s\n", aCB.dp->info.actualStringValue);
    } else if (aCB.dp->info.isTypeNative) {
        if (aCB.dp->info.rawStringValue) {
            // free the current rawStringValue
            printf("%s: isTypeNative deallocate aCB.dp->info.rawStringValue (%p)\n", __FUNCTION__, (void *)aCB.dp->info.rawStringValue);
            IdlMemFree(aCB.dp->info.rawStringValue);
        }
        // let rawStringValue be freed by Idl library routine
        char *strValue = cJSON_PrintUnformatted(pcJsonDpVal);
        aCB.dp->info.rawStringValue = strValue;
        printf("%s: isTypeNative new aCB.dp->info.rawStringValue (%p) = %s\n", 
                __FUNCTION__, (void *)aCB.dp->info.rawStringValue, aCB.dp->info.rawStringValue);
    } else {
        idlError = JsonDpValueToDouble(pcJsonDpVal, aCB.dp, dValue);
        // aCB.dp->info.actualValue = *dValue;
        printf("%s: isTypeDouble setting dValue=%lf\n", __FUNCTION__, *dValue);
    }

    return idlError;
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

/* IdiGenericResultFsm: Example routine implementing a finite state machine */
/* to simulate asynchronous processing of any registered callback routines  */
/* and finally call the Action Callback Result to let Idl know we are done. */
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
               if (!IsSimulateProcessingDone()) {
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
               if (!IsSimulateProcessingDone()) {
                   /* simulate a busy condition (not done) */
                   idlError = IErr_IdiBusy;
               }
               else {
                   // free up per device's storage
                   // NOTE:
                   // we do it here and not in the actual callback function so we don't 
                   // have to potentially deal with removing affected CBs from list of 
                   // CBs since we only have one active CB in this example :)
                   DevRemoveCustomIdiDevData(aCB.dev);
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
                if (!IsSimulateProcessingDone()) {
                    /* simulate a busy condition (not done) */
                    idlError = IErr_IdiBusy;
                }
                else {
                    /*
                    *  Replace a device: may involve deprovision old device/unid
                    */
                   DevReplaceUnid(aCB.dev);
                   
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
            if (IsSimulateProcessingDone()) {
                T_DpSto *pDpStruc = (T_DpSto *)(aCB.dp->idiDpData);
                if (pDpStruc) {
                    if (*pDpStruc->pDpValue) {
                        // free existing Cjson object from the dp entry in per device's DevDpValue vector
                        IdlCjsonDelete(*pDpStruc->pDpValue);
                    }

                    // update dp entry in in per device's DevDpValue vector
                    *pDpStruc->pDpValue = GetDpValueForDpLocalStorage(aCB);
                    char *outStr = cJSON_PrintUnformatted(*pDpStruc->pDpValue);
                    printf(" Value written: %s at pDpStruc(%p)->pDpValue = %p\n", outStr, pDpStruc, pDpStruc->pDpValue);

#ifdef INCLUDE_ETI
                    uint reg = pDpStruc->address;
                    T__DevStoPtr pDevEntry = (T__DevStoPtr)(aCB.dev->idiDevData);
                    idlError = DevWritePublish(pDevEntry, reg, *pDpStruc->pDpValue);
#endif
                    IdlMemFree(outStr);
                }
                else {
                    idlError = IErr_Failure;
                    printf(" Write error: Unitialized idiDpData - device possibly has been deleted!\n");
                }
                IdlDpWriteResult(aCB.ReqIndex, aCB.dev, aCB.dp, idlError);
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
            if (IsSimulateProcessingDone()) {
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
                            printf("%s Value read: %s (before applying multiplier: %lf) at pDpStruc(%p)->pDpValue = %p\n", 
                                    (aCB.lastError == IErr_IdiBusy) ? ".done\n" : "", 
                                    jsonStr, pDpStruc->testMultiplier, pDpStruc, pDpStruc->pDpValue);
                         }
                        else {
                            printf(" Read error: Unable to read dp entry in localDpValuesVector\n");
                        }
                        IdlMemFree(jsonStr);
                    }
                    else {
                        idlError = IErr_Failure;
                        printf(" Read error: No value found for dp entry in localDpValuesVector\n");
                    }
                }
                else {
                    idlError = IErr_Failure;
                    printf(" Read error: Unitialized idiDpData - device possibly has been deleted!\n");
                }
                IdlDpReadResult(aCB.ReqIndex, aCB.dev, aCB.dp, aCB.context, idlError, prio_array, dpValue);
                aCB.action = IdiaNone;
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
            if (IsSimulateProcessingDone()) {
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
            if (IsSimulateProcessingDone()) {
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

// This implementation example is not ideal but works to show asynchronous callback processing..
// An ideal custom driver should not be blocking the FSM while processing a long callback.
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
                    printf(" Processing delay..");
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
                usleep(5000);
            }
        }
    }
    return idlError;
}


