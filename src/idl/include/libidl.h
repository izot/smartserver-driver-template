/**
 *******************************************************************************
 * @file    libidl.h
 * @author  Rashmi Vaidya
 *
 * @brief   This files exposes all the structures, APIs and callback prototypes needed
 *          to integrate a driver with IAP/MQ. An IAP Driver Instance (IDI) must include
 *          this file in their source codes to use the APIs provided by the IDL.
 *          1. The main structures are Idl, IdlAdvOpts, IdlDev, IdlDatapoint and IdlDiscoveryCallback.
 *          2. The important APIs exposed are IdlNew(), IdlSetAdvancedOptions(), IdlInit(), APIs for
 *             registering device action callbacks, APIs for registering DP read and write callbacks,
 *             result APIs, IdlOnDpEvent() and IdlDeviceDiscovered().
 *          3. This file also provides the error codes enum (IdlErrorCodes) that the IDI must
 *             use when sending success/error responses to the IDL.
 *
 *******************************************************************************
 * @attention
 *   Copyright 2019 - 2022 Dialog Semiconductor.  All Rights Reserved.
 *******************************************************************************
 **/

#ifndef IDL_H
#define IDL_H

#include <stdint.h>
#include <uv.h>

#define NORMAL_PRIO 17

typedef unsigned int DpAddress;
typedef unsigned int DpLevel;
typedef unsigned int ModbusFunctionCode;
typedef unsigned int ModbusDatatype;
typedef double ModbusDiscoveryMarkerValue;

typedef enum {
    IErr_Success = 0,
    IErr_Failure,
    IErr_IdiBusy,
    IErr_TypeInvalid,
    IErr_UnidInvalid,
    IErr_UnidExists,
    IErr_DevCommFail,
    IErr_Nack,
    IErr_Unknown,
    IErr_Max
} IdlErrorCodes;

typedef enum {
    M_BIT = 0,
    M_UINT8,
    M_UINT16,
    M_SINT16,
    M_UINT32,
    M_SINT32,
    M_FLOAT,
    M_SINT64,
    M_MOD10,
    M_MOD10_2,
    M_MOD10_3,
    M_MOD10_4,
    M_CHAR8,
    M_CHAR16,
    M_CHAR8_2,
    M_MODB_DATATYPES_MAX
} ModbusDatatypes;

typedef enum {
    I_EMPTY_ENDIAN = 0,
    I_BIG_ENDIAN,
    I_LITTLE_ENDIAN
} I_Endianness;

typedef enum {
    M_NONE = 0,
    M_TTYMXC2,
    M_TTYMXC4
} M_RS485Port;

typedef enum {
    M_PARITY_NONE = 0,
    M_ODD_PARITY,
    M_EVEN_PARITY
} M_PARITY;

typedef enum {
    Act_Create = 0,
    Act_Prov,
    Act_Deprov,
    Act_Delete,
    Act_Load,
    Act_Join,
    Act_Msg,
    Act_Repair,
    Act_Replace,
    Act_Reset,
    Act_Test,
    Act_Wink,
    Act_Max
} I_DevAction;

typedef enum {
    ConnAct_Create = 0,
    ConnAct_Prov,
    ConnAct_Deprov,
    ConnAct_Update,
    ConnAct_Delete,
    ConnAct_Max
} I_ConnAction;

typedef enum {
    DEV_STATE_UNKNOWN = 0,
    DEV_STATE_UNLICENSED,
    DEV_STATE_UNPROVISIONED,
    DEV_STATE_PROVISIONED,
    DEV_STATE_DELETED,
    DEV_STATE_MAX			/* Add new states before this */
} I_DevState;

typedef enum {
    I_HEALTH_NORMAL = 0,
    I_HEALTH_SUSPECT,
    I_HEALTH_DOWN,
    I_HEALTH_NASCENT,
    I_HEALTH_MAX
} I_Health;

typedef enum {
    CAT_DATA = 0,
    CAT_INFO,
    CAT_ERR,
    CAT_IN,
    CAT_OUT,
    CAT_MAX
} I_DataCategory;

typedef enum {
    REP_ANY = 0,
    REP_CHANGE,
    REP_THR_ANY,
    REP_THR_ALL,
    REP_MAX
} I_MonReport;

typedef enum {
    P_NONE = 0,
    P_CREATE,
    P_PROV,
    P_DEPROV,
    P_DEL,
    P_CREATE_PLUS_PROV,
    P_DEPROV_PLUS_DEL,
    P_IAP_TYPES
} I_PendingAct;

typedef enum {
    NORMAL_READ_REQ = 1,
    WRITE_REQ,
    GET_REQ,   //get request
    READ_BACK_REQ,
    PROV_REQ,  //provision action for the first time
    TEST_REQ,   //test action
    HEALTH_REQ,
    COV_REQ, //to check if value has changed for source conn or event = true
    RCV_TIMEOUT_REQ
} pollRequestType;

typedef enum _DiscoveryCallbackType {
    DISCOVERY_START_CALLBACK, 
    DISCOVERY_STEP_CALLBACK, 
    DISCOVERY_STOP_CALLBACK
} DiscoveryCallbackType;

typedef enum {
    CONN_STATE_UNKNOWN = 0,
    CONN_STATE_CREATED,
    CONN_STATE_PROVISIONED,
    CONN_STATE_UNPROVISIONED,
    CONN_STATE_UPDATED,
    CONN_STATE_DELETED,
    CONN_STATE_MAX			/* Add new states before this */
} I_ConnState;

typedef enum {
    S_None = 0,
    S_NativeScaling,
    S_Abc,
    S_MultOff,
    S_AbcFromTypeDef
} ScalingType;

/* IDI multi-channel support details */
typedef struct _ProtoChannel {
    unsigned int channelId;
    char *channelName;
    char *channelTtyPath;
} ProtoChannel;

typedef struct _PriorityArray {
    //unsigned int isValid:1;
    double value;
    char *stringValue;
    char *nativeValue;
} PriorityArray;

typedef struct _EventMonitorObject {
    unsigned int enabled:1;
    unsigned int eventMonitoringByIdl:1;
    unsigned int retryCount;
    unsigned int callbackSuccessCount;
    long long lastRenewedTimeMs;
    uv_timer_t eventCallbackTimer;
} EventMonitorObject;

typedef struct _MonitorObject {
    uv_timer_t receiveTimeoutTimer;
    uv_timer_t onDemandPublishTimer;
    I_Health health;
    EventMonitorObject event;
    unsigned int receiveTimeoutMs;
    float rate;
    float throttle;
    float heartbeat;
    float threshold;
    I_DataCategory cat;
    I_MonReport report;
    char *focus;
    long long lastPubTimeMs;
} MonitorObject;

typedef struct _ScalingObject {
    unsigned int isRangeMinValid:1;
    unsigned int isRangeMaxValid:1;
    unsigned int isPrecisionValid:1;
    unsigned int isInvalidPresent:1;
    int precision;
    ScalingType type;
    double nativeValue1;
    double nativeValue2;
    double scaledValue1;
    double scaledValue2;
    float multiplier;
    float offset;
    float A;
    float B;
    float C;
    double rangeMin;
    double rangeMax;
    double invalidValue;
} ScalingObject;

typedef struct _DefaultObject {
    unsigned int isDefaultValid:1;
    double value;
    char *stringValue;
    char *nativeValue;
} DefaultObject;

typedef struct _OnDemandResponse {
    char *topic;
    char *corr;
} OnDemandResponse;

typedef struct _DpReadContext {
    pollRequestType type;
} DpReadContext;

typedef struct _IapDpReadContext {
    pollRequestType type;
    OnDemandResponse response;
} IapDpReadContext;

typedef struct _IdlConnectionTopics {
    char *stsTopic;
    char *implTopic;
} IdlConnectionTopics;

typedef struct _IdlConnection
{   
    //pthread_mutex_t dcmlock;
    I_ConnState state;
    int sourceCount;
    int destCount;
    char *handle;
    IdlConnectionTopics topic;
    char **sourceList;
    char **destList;
    char *monitorIn;
    char *monitorOut;
    struct _IdlConnection *prev;
    struct _IdlConnection *next;
} IdlConnection;

typedef struct _IdlConnectionSrcList {
    IdlConnection *conn;
    struct _IdlConnectionSrcList *prev;
    struct _IdlConnectionSrcList *next;
} IdlConnectionSrcList;

typedef struct _IapTypeEnumMap {
    int value;
    char *enumStr;
} IapTypeEnumMap;

typedef struct _IapTypeEnumObj {
    int count;
    IapTypeEnumMap *enumMap;
} IapTypeEnumObj;

typedef struct _IdlIapDatapoint IdlIapDatapoint;

typedef struct _IdlDpInfo {
    unsigned int isInput:1;
    unsigned int isReadOnly:1; //This is defined in IAP interface
    unsigned int isFirstDp:1;
    unsigned int firstWriteDone:1;
    unsigned int firstReadDone:1; //This will tell if the actual value was actually read from the device or not. Useful right after start-up
    unsigned int pollInProgress:1;
    unsigned int isWritePending:1;
    unsigned int isPrioArrayWritable:1;
    unsigned int isNormalWritable:1;
    unsigned int writeInvalidToDp:1; //This flag is set when an "INVALID" value is to be written to the DP and cleared after the write
    uv_timer_t pollTimer;
    uv_timer_t dpCallbackTimer;
    unsigned int onDemandEventFlag:1;
    unsigned int isTypeAscii:1;
    unsigned int isTypeNative:1;
    unsigned int asciiLength;
    unsigned int asciiStringLength;
    ScalingObject scale;
    DefaultObject dflt;
    double actualValue;
    double rawValue;
    double lastPubValue; /* Last value that was pub-ed to
                            ev/data or monitoring service */
    char *actualStringValue;
    char *actualNativeValue;
    char *rawStringValue;
    char *unrecColumnData;
    PriorityArray pArr[NORMAL_PRIO];
    long long lastReadTimeMs;
    IapTypeEnumObj iapEnum;
    DpReadContext *context;
    IdlIapDatapoint *parentIapdp;
} IdlDpInfo;

typedef struct _DevMgmt {
    unsigned int isRestore:1;
    unsigned int firstProvDone:1;
    unsigned int isDeviceModbusRTU:1;
    unsigned int isDevMarginal:1;
    unsigned int isDevLicensed:1;
    unsigned int isDevStateProvision:1;
    unsigned int offlineCount;
    unsigned long blockCount;         // is used to manage interface restore
    unsigned long xifBlockCount;      // number of blocks available in XIF file
    unsigned long deletedBlockCount;  // number of deleted blocks in /if channel
    unsigned long oldBlockCount;      // number of blocks in fb channel that are still part of the current xif
    I_PendingAct pendingAction;
    uv_timer_t actionCallbackTimer;
    uv_timer_t interfaceBlockTimer;
    uv_timer_t nackTimer;
    long long lastPollAckTimeMs;
    char *oldUnid;
} DevMgmt;

typedef struct _DevInfo {
    char *name;
    char *desc;
    char *manufacturer;
    char *product;
    char *ver;
} DevInfo;

typedef struct _DevLocation {
    float latitude;
    float longitude;
    float elevation;
} DevLocation;

typedef struct _DevTopic {
    char *request; /* glp/0/SID/rq/dev/P/H */
    char *feedback; /* glp/0/SID/fb/dev/P/H */
    char *requestInterface; /* glp/0/SID/rq/dev/P/H/if */
    char *feedbackInterface; /* glp/0/SID/fb/dev/P/H/if */
    char *engine; /* glp/0/./=engine/P */
} DevTopic;

typedef struct _ModbusPortInfo {
    M_RS485Port port;
    M_PARITY parity;
    unsigned int baudrate;
    unsigned int stopbits:2;
} ModbusPortInfo;

typedef struct _ModbusDpInfo {
    ModbusFunctionCode functionCode;
    ModbusDatatype datatype;
} ModbusDpInfo;

typedef struct _ModbusMarkerInfo {
    I_Endianness wordOrder;
    I_Endianness byteOrder;
    I_Endianness bitOrder;
    ModbusDpInfo modbusDpInfo;
    DpAddress address;
    ModbusDiscoveryMarkerValue rawMarkerValue;
    struct _ModbusMarkerInfo *next;
} ModbusMarkerInfo;

typedef struct _IdlXifInfo {
    char *xin;
    char *pid;
    ModbusMarkerInfo *firstMarker; //singly linked list of ModbusMarkerInfo
    struct _IdlXifInfo *next;
} IdlXifInfo;

typedef struct _IdlDiscoveryTimeouts {
    unsigned int startDiscoveryTimeoutMs;
    unsigned int stepDiscoveryTimeoutMs;
    unsigned int stopDiscoveryTimeoutMs;
} IdlDiscoveryTimeouts;

typedef struct _IdlDiscoveryData {
    IdlDiscoveryTimeouts timeouts;
    IdlXifInfo *firstXif; //singly linked list of IdlXifInfo
} IdlDiscoveryData;

typedef struct _IdlInterfaceBlock IdlInterfaceBlock;

typedef struct _IdlIapDpInfo {
    unsigned int isProperty:1;
    unsigned int writeInitial:1;
    int isInput;
    int isReadOnly; //This is defined in IAP interface
    unsigned int pollRateMs; //based on monitor.rate, monitor.event, heartbeat and dcm source
    DpLevel level; /* The active priority level */
    char *iapType; //The extended IAP type with SNVT_ etc prefix/suffix
    char *xifIapType; //As mentioned in the XIF. May or may not have SNVT_ etc prefixes/suffix
    MonitorObject monitor;
    unsigned int isValid[NORMAL_PRIO];
    char *presets;
    char *localization;
    IapDpReadContext *context;
    IdlConnectionSrcList *firstSrcConn;
    IdlInterfaceBlock *parentIfblock;
} IdlIapDpInfo;

/* IDL datapoint structure */
typedef struct _IdlDatapoint {
    I_Endianness wordOrder;
    I_Endianness byteOrder;
    I_Endianness bitOrder;
    DpAddress address;
    char *bacnetDpAddress;
    char *name; //IAP DP name copied from IdlIapDatapoint so that compiling IDI which uses dp->name does not break
    void *idiDpData; // Protocol specific datapoint data
    IdlDpInfo info;
    ModbusDpInfo modbusDpInfo;
    struct _IdlDatapoint *next;
} IdlDatapoint;

typedef struct _IapTypeValueFields {
    char *fieldName;
    char *enumTypeName;
    char *iapType;
    char *typeString;
    IdlDatapoint *dp;
    ScalingObject *typeScaling;
    struct _IapTypeValueFields *next;
    struct _IapTypeValueFields *child;
} IapTypeValueFields;

/* IDL datapoint structure */
typedef struct _IdlIapDatapoint {
    int dpCnt;
    char *name; //IAP DP name. The DP is identified in IAP/MQ with this name, eg in topics, connections, get requests, etc.
    char *dpName; //This name appears in "name" field in /if blocks. This is the DP name that appears on the CMS.
    IdlIapDpInfo info;
    IapTypeValueFields *firstField;
    struct _IdlIapDatapoint *next; //next IAP DP in the ifblock
} IdlIapDatapoint;

/* IDL interface block structure */
typedef struct _IdlInterfaceBlock {
    unsigned int firstPubDone:1; /* is used to manage interface restore */
    int index; /* block index for IAP interface topics */
    char *name;
    IdlIapDatapoint *firstIapdp;
    struct _IdlInterfaceBlock *next;
} IdlInterfaceBlock;

/* IDL device structure */
typedef struct _IdlDev {
    I_DevState state;
    I_Health health;
    DevLocation location;
    ProtoChannel channel;
    DevMgmt mgmt;
    DevInfo info;
    DevTopic topic;
    char *unid;
    char *handle;
    char *type;
    void *idiDevData; //Protocol specific device data
    IdlInterfaceBlock *firstIfblock;
    struct _IdlDev *prev;
    struct _IdlDev *next;
    unsigned long totalBlocks;
} IdlDev;

typedef int (*DeviceDiscoveryCallbackFPtr) (void *driverInstanceContext, 
        IdlDiscoveryData *data, int *done, unsigned int *msTimeout);

typedef struct _IdlDiscoveryCallback {
    void *context;
    DeviceDiscoveryCallbackFPtr cb;
    struct _IdlDiscoveryCallback *next;
} IdlDiscoveryCallback;

typedef struct _IdlAdvOpts {
    unsigned int renewEventOnRestart:1;
    unsigned int receiveTimeoutSupported:1;
    unsigned int objectInstancesAreNumbers:1;
    unsigned int uidsAreNumbers:1;
} IdlAdvOpts;

/* Common structure that consists of all callbacks */
typedef struct _Idl {
    /* callbacks */
    int (*OnDevCreate)(int, IdlDev *, char *, char *);
    int (*OnDevProvision)(int, IdlDev *, char *);
    int (*OnDevDeprovision)(int, IdlDev *);
    int (*OnDevReplace)(int, IdlDev *, char *);
    int (*OnDevDelete)(int, IdlDev *);
    int (*DpRead)(int, IdlDev *, IdlDatapoint *, void *);
    int (*DpWrite)(int, IdlDev *, IdlDatapoint *, int, int, double);
    int (*DpAsciiRead)(int, IdlDev *, IdlDatapoint *, void *);
    int (*DpAsciiWrite)(int, IdlDev *, IdlDatapoint *, int, int, char *);
    int (*OnUnrecColumn)(int , IdlDatapoint *, char *);
    int (*OnDpCreate)(int, IdlDev *, IdlDatapoint *, char *);
    IdlErrorCodes (*DpEnableEvent)(int, IdlDev *, IdlDatapoint *);
    IdlErrorCodes (*DpDisableEvent)(int, IdlDev *, IdlDatapoint *);
    int (*ModbusPortFn)(int, ModbusPortInfo);
    IdlDiscoveryCallback *startDiscovery;
    IdlDiscoveryCallback *firstStep;
    IdlDiscoveryCallback *stopDiscovery;
} Idl;

Idl *IdlNew(void);
void IdlSetAdvancedOptions(IdlAdvOpts opts); //should be called after IdlNew() but before IdlInit()
int IdlInit(char *confPath, Idl *idl);

/* Set callback APIs */
void IdlDevCreateCallbackSet(Idl *idl, int (*OnDevCreate)(int, IdlDev *, char *, char *));
void IdlDevProvisionCallbackSet(Idl *idl, int (*OnDevProvision)(int, IdlDev *, char *));
void IdlDevDeprovisionCallbackSet(Idl *idl, int (*OnDevDeprovision)(int, IdlDev *));
void IdlDevReplaceCallbackSet(Idl *idl, int (*OnDevReplace)(int, IdlDev *, char *));
void IdlDevDeleteCallbackSet(Idl *idl, int (*OnDevDelete)(int, IdlDev *));
void IdlDpReadCallbackSet(Idl *idl, int (*DpRead)(int, IdlDev *, IdlDatapoint *, void *));
void IdlDpWriteCallbackSet(Idl *idl, int (*DpWrite)(int, IdlDev *, IdlDatapoint *, int, int, double));
void IdlDpAsciiReadCallbackSet(Idl *idl, int (*DpAsciiRead)(int, IdlDev *, IdlDatapoint *, void *));
void IdlDpAsciiWriteCallbackSet(Idl *idl, int (*DpAsciiWrite)(int, IdlDev *, IdlDatapoint *, int, int, char *));
void IdlDpEnableEventCallbackSet(Idl *idl, IdlErrorCodes (*DpEnableEvent)(int, IdlDev *, IdlDatapoint *));
void IdlDpDisableEventCallbackSet(Idl *idl, IdlErrorCodes (*DpDisableEvent)(int, IdlDev *, IdlDatapoint *));
int IdlDevDiscoveryCallbackSet(Idl *idl, void *driverInstanceContext, 
        DiscoveryCallbackType dcbFunctionType, DeviceDiscoveryCallbackFPtr dcbFunction);
void IdlModbusPortCallbackSet(Idl *idl, int (*ModbusPortFn)(int, ModbusPortInfo));
void IdlDpUnrecColumnCallbackSet(Idl *idl,int (*OnUnrecColumn)(int , IdlDatapoint *, char *));
void IdlDpCreateCallbackSet(Idl *idl,int (*OnDpCreate)(int , IdlDev *, IdlDatapoint *, char *));

/* Result APIs */
void IdlDevCreateResult(int callbackIndex, IdlDev *dev, int idlError);
void IdlDevProvisionResult(int callbackIndex, IdlDev *dev, int idlError);
void IdlDevDeprovisionResult(int callbackIndex, IdlDev *dev, int idlError);
void IdlDevReplaceResult(int callbackIndex, IdlDev *dev, int idlError);
void IdlDevDeleteResult(int callbackIndex, IdlDev *dev, int idlError);
void IdlDpReadResult(int callbackIndex, IdlDev *dev, IdlDatapoint *dp, 
        void *idlContext, int idlError, char *prioArray, double value);
void IdlDpWriteResult(int callbackIndex, IdlDev *dev, IdlDatapoint *dp, int idlError);
void IdlDpEnableEventResult(int callbackIndex, IdlDev *dev, IdlDatapoint *dp, IdlErrorCodes idlError);
void IdlDpDisableEventResult(int callbackIndex, IdlDev *dev, IdlDatapoint *dp, IdlErrorCodes idlError);

/* Device Discovered API - frName is the Friendly name and may be NULL */
int IdlDeviceDiscovered(char *unid, char *xin, char *frName);

/* Event based monitoring */
void IdlOnDpEvent(IdlDev *dev, IdlDatapoint *dp, char *prioArray, double value);

/* API to get device pointer from UNID */
IdlDev *IdlGetDeviceByUnid(char *unid);

/* API for BACnet to get datapoint pointer from IAP DP name which is of the format <object type>:<object instance> */
IdlDatapoint *IdlGetDatapointByName(IdlDev *dev, char *dpName, char *blockName, int *blockIndex);

/* API to publish custom error message strings to logger */
void IdlPublishCustomErrorMessage(IdlDev *dev, IdlDatapoint *dp, char *errorMsg);

/* API wrapper over printf() that can print or not print the logs depending on debug mode */
void dbg_print(const char *fmt, ...);

#endif
