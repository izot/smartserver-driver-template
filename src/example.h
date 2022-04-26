#ifndef EXAMPLE_H
#define EXAMPLE_H

#include "common.h"

#define IdiFree(x) {if (x) { free(x); x = NULL;}}
#define IDI_ACTION_NORMAL_TIMEOUT   30
#define IDI_ACTION_LONG_TIMEOUT     50

typedef enum {
    IdiaNone = 0,
    IdiaCreate,
    IdiaProvision,
    IdiaDeprovision,
    IdiaReplace,
    IdiaDpread,
    IdiaDpwrite,
    IdiaDelete,
    Ida_last
} IdiAction;

// Various IDL 'result' bits
typedef struct _IdiActiveCB {
    double dValue;
    char *rawStringValue;
    char *args;
    int prio;
    int relinquish;
    IdiAction action;
    int ReqIndex;
    IdlDev* dev;
    IdlDatapoint* dp;
    uint timeout;          // time out in milliseconds
    int lastError;
    void* context;
} IdiActiveCB;

extern int IdiStart();

extern void *ProcAsynThrdFunc(void* argA);

/* CALLBACKS */
extern int OnDpReadCb(int request_index, IdlDev *dev, IdlDatapoint *dp, void *context);
extern int OnDpReadExCb(int request_index, IdlDev *dev, IdlDatapoint *dp, void *context);
extern int OnDpWriteCb(int request_index, IdlDev *dev, IdlDatapoint *dp, int prio, int relinquish, double value);
extern int OnDpWriteExCb(int request_index, IdlDev *dev, IdlDatapoint *dp, int prio, int relinquish, char *value);
extern int OnDpCreateCb(int  request_index, IdlDev *dev, IdlDatapoint *dp, char *cpUnrecognizedColumns);
extern int OnDevCreateCb(int request_index, IdlDev *dev, char *args, char *xif_dp_array);
extern int OnDevProvisionCb(int request_index, IdlDev *dev, char *args);
extern int OnDevDeprovisionCb(int request_index, IdlDev *dev);
extern int OnDevReplaceCb(int request_index, IdlDev *dev, char *args);
extern int OnDevDeleteCb(int request_index, IdlDev *dev);
#ifdef AP_9580_WORKAROUND
extern int OnUnrecColumnCb(int request_index, IdlDatapoint *dp, char *cpUnrecogCols);
#endif

#ifdef INCLUDE_ETI
#include "eti.h"
#endif

#endif
