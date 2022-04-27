//
// example.h
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
// Custom driver include file for example driver
//

#ifndef EXAMPLE_H
#define EXAMPLE_H

#include "common.h"

#define IdiFree(x) {if (x) { free(x); x = NULL;}}
#define IDI_ACTION_NORMAL_TIMEOUT   30
#define IDI_ACTION_LONG_TIMEOUT     50
#define IDI_ACT_Q                   "/dev_act_q_idi_%s"


typedef enum {
    IdiaNone = 0,
    IdiaCreate,
    IdiaDpCreate,
    IdiaProvision,
    IdiaDeprovision,
    IdiaReplace,
    IdiaDpread,
    IdiaDpwrite,
    IdiaDelete,
    Ida_last
} IdiAction;

// Various IDL 'result' bits
typedef struct _IdiActionCB {
    double dValue;
    char *rawStringValue;
    char *args;
    char *cpUnrecogCols;
    int prio;
    int relinquish;
    IdiAction action;
    int ReqIndex;
    IdlDev* dev;
    IdlDatapoint* dp;
    uint timeout;          // time out in milliseconds
    int lastError;
    void* context;
} IdiActionCB;

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
