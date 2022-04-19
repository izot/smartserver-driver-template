/**
 *******************************************************************************
 * @file    IdlCommon.h
 * @author  Rashmi Vaidya
 *
 * @brief   This file contains all structures, macros and API declarations
 *          commonly used in the IDL source files.
 *******************************************************************************
 * @attention
 *   Copyright 2019 - 2022 Dialog Semiconductor.  All Rights Reserved.
 *******************************************************************************
 **/

#ifndef IDL_COMMON_H
#define IDL_COMMON_H

#include <stdint.h>
#include "cJSON.h"
#include "csvin.h"
#include "libidl.h"

#define IDL_VERSION             "1.04.001" //On-demand behaviour

#define DEV_HNDL_INDEX          7
#define DEV_TOPIC_ID_INDEX      8
#define BLK_NAME_INDEX          9
#define BLK_INDEX_INDEX         10
#define DATAPOINT_INDEX         11

#define DVC_DEV_HNDL_INDEX      10
#define DVC_BLK_NAME_INDEX      12
#define DVC_BLK_INDEX_INDEX     13
#define DVC_DATAPOINT_INDEX     14

#define MONITOR_HNDL_INDEX      6
#define CONN_HNDL_INDEX         7
#define CONN_TOPIC_ID_INDEX     8

#define GET_DEV_HNDL_INDEX      10
#define GET_BLK_NAME_INDEX      12
#define GET_BLK_INDEX_INDEX     13
#define GET_DATAPOINT_INDEX     14

#define DEFAULT_CREATE_TMOUT    120000 //120sec
#define DEFAULT_PROV_TMOUT      60000 //60sec
#define DEFAULT_DEPROV_TMOUT    10000 //10sec
#define DEFAULT_DEL_TMOUT       60000 //60sec
#define DEFAULT_REPLACE_TMOUT   60000 //60sec
#define DEFAULT_TEST_TMOUT      60000 //60sec
#define DEFAULT_READ_TMOUT      1000 //1sec
#define DEFAULT_WRITE_TMOUT     1000 //1sec
#define DEFAULT_ENABLE_EVENT_TMOUT 3000 //3sec
#define DEFAULT_EVENT_RENEWAL_TMOUT 120000 //120sec
#define DEFAULT_HEALTH_TMOUT    1000 //1sec
#define DEFAULT_HOLD_TMOUT      120000 //120sec
#define DEFAULT_DISCOV_START_TMOUT 15000 //15sec
#define DEFAULT_DISCOV_STEP_TMOUT  30000 //30sec
#define DEFAULT_DISCOV_STOP_TMOUT  15000 //15sec
#define DEFAULT_EVENT_POLL_RATE    500 //0.5sec

#define AMD_METHOD              1
#define DISCOVERY_METHOD        2
#define IAP_TYPES_METHOD        3

#define WRITE_TO_ALL_DEV        1
#define WRITE_TO_RTU_DEV        2

#define MQTT_TOPIC_SIZE         1024
#define IDL_SUCCESS             0
#define IDL_FAILURE             -1

#define MQTT_DEFAULT_KEEP_ALIVE_TIME  60
#define MQTT_BROKER_PORT        1883
#define MQTT_BROKER_ADDR        "127.0.0.1"

#define RUNLEVEL_STARTED_VAL    100

#define DP_ENABLE_EVENT_RETRY_COUNT 3

#define IAP_RQ_Q                "/iap_rq_q"
#define POLL_MSG_Q              "/poll_msg_q"
#define DEV_ACT_Q               "/dev_act_q"

#define APOLLO_SID_TOPIC        "glp/0/././sid"
#define APOLLO_READINESS_TOPIC  "glp/0/././runlevel"
#define APOLLO_CFG_TOPIC        "glp/0/%s/fb/cfg"
#define APOLLO_MONITOR_TOPIC    "glp/0/./=monitoring/service"
#define APOLLO_LOGGER_TOPIC     "glp/0/./=logger/event"
#define ENGINE_TOPIC            "glp/0/./=engine"
#define DP_GET_TOPIC            "glp/0/./=engine/%s/rq/poll/dev/%s"
#define AMD_TOPIC               "glp/0/%s/fb/res/application/%s/%s"
#define RES_TYPE_TOPIC          "glp/0/%s/fb/res/type/%s"
#define DEVICE_DISCOVERY_RQ_TOPIC "glp/0/./=engine/%s/rq/discoverDevices"
#define DISCOVERY_EVENT_TOPIC   "glp/0/%s/ev/discovery/device"
#define CONNECTION_TOPIC        "glp/0/./=engine/%s/con"
#define MONITOR_INPUT_TOPIC     "glp/0/./=dcm/con/%s/in"
#define MONITOR_OUTPUT_TOPIC    "glp/0/./=dcm/con/%s/out"
#define MODBUS_PORT_TOPIC       "glp/0/%s/%s/dev/modbus/sys/if/modbus/0"
#define DEBUG_MODE_TOPIC        "idl/%s/%s/debugMode"
#define RESOURCE_PUBLISHER_RQ_TOPIC "glp/0/./=resource-publisher/request"
#define RESOURCE_PUBLISHER_RSP_TOPIC "idl/apollo-rp-launcher/response"

#define APOLLO_DATA_ENVVAR      "APOLLO_DATA"
#define APOLLO_CONFD_ENVVAR     "APOLLO_CONFD"
#define APOLLO_DATA_PATH        "/var/apollo/data"
#define APOLLO_CONFD_PATH       "/var/apollo/conf.d"
#define RS485_CONFD_FILE        "rs-485.conf"
#define TTY_PORT1_STR           "/dev/ttymxc2"
#define TTY_PORT2_STR           "/dev/ttymxc4"
#define RS485_PID               "8FFFF00500000000-4"
#define RS485_BAUD_TYPE         "UCPTbaudrate"
#define RS485_PARITY_TYPE       "UCPTparity"
#define RS485_STOPBITS_TYPE     "UCPTstopbits"
#define RS485_PROFILE_TYPE      "UFPTmodbusRtuMgmt"

#define BACNET_PROTOCOL_ID      "bacnet"
#define MODBUS_PROTOCOL_ID      "modbus"
#define DEFAULT_PROTOCOL_ID     "custom"
#define PROTO_ID_STR            "Protocol identifier"
#define MQTT_DETAILS_STR        "MQTT specific details"
#define CLIENTID_STR            "MQTT client ID"
#define CLEAN_SESSION_STR       "cleanSession"
#define XIF_PATH_STR            "xif_dir_absolute_path"
#define XIF_FT_SUF_STR          "_xif"
#define DISCOV_CONF_STR         "isDiscoverySupported"
#define DO_STR                  "do"
#define STS_STR                 "sts"
#define CFG_STR                 "cfg"
#define LIC_STR                 "lic"
#define IF_STR                  "if"
#define IMPL_STR                "impl"
#define ACTION_STR              "action"
#define ARGS_STR                "args"
#define TYPE_STR                "type"
#define DEVTYPE_STR             "devtype"
#define UNID_STR                "unid"
#define PROVISION_STR           "provision"
#define STATE_STR               "state"
#define HEALTH_STR              "health"
#define ERROR_STR               "error"
#define CAT_STR                 "cat"
#define INTERFACE_STR           "interface"
#define USAGE_STR               "usage"
#define VERSION_STR             "version"
#define PRODUCT_STR             "product"
#define XIF_STR                 "xif"
#define BODY_STR                "body"
#define MFG_STR                 "manufacturer"
#define MRU_STR                 "mru"
#define UTC_STR                 "utc"
#define TIMESTAMP_STR           "ts"
#define STATIC_STR              "static"
#define CONTROLER_STR           "controller"
#define NAME_STR                "name"
#define DESC_STR                "desc"
#define ZONE_STR                "motion_zone"
#define RAD_STR                 "motion_radius"
#define MTIMEOUT_STR            "motion_timeout"
#define LAT_STR                 "lat"
#define LONG_STR                "lng"
#define ELE_STR                 "ele"
#define LOC_STR                 "loc"
#define VALUE_STR               "value"
#define PRIO_STR                "prio"
#define NONE_STR                "none"
#define MONITOR_STR             "monitor"
#define RATE_STR                "rate"
#define DEFAULT_BLOCK_NAME      "block"
#define RATE_STR                "rate"
#define EVENT_STR               "event"
#define THROTTLE_STR            "throttle"
#define THRESHOLD_STR           "threshold"
#define REPORT_STR              "report"
#define FOCUS_STR               "focus"
#define HEARTBEAT_STR           "heartbeat"
#define RECVTIMEOUT_STR         "receiveTimeout"
#define READONLY_STR            "read-only"
#define PROPERTY_STR            "property"
#define PRESETS_STR             "presets"
#define LOCALIZE_STR            "localization"
#define LEVELS_STR              "levels"
#define LEVEL_STR               "level"
#define VALUES_STR              "values"
#define DEFAULT_STR             "default"
#define INITIA_VAL_STR          "Initial Value"
#define WRITE_INITIAL_STR       "Write Initial"
#define FAST_STR                "fast"
#define SLOW_STR                "slow"
#define NORMAL_STR              "normal"
#define AUTO_STR                "auto"
#define LANGUAGE_STR            "language"
#define EN_STR                  "en"
#define MESSAGE_STR             "message"
#define DATA_STR                "data"
#define DATAPOINT_STR           "datapoint"
#define SOURCE_STR              "source"
#define SOURCES_STR             "sources"
#define DESTS_STR               "destinations"
#define INPUT_STR               "input"
#define OUTPUT_STR              "output"
#define TOPIC_STR               "topic"
#define BIG_STR                 "big"
#define LITTLE_STR              "little"
#define INVALID_STR             "invalid"
#define ABOVE_MAX_STR           "above-max"
#define BELOW_MIN_STR           "below-min"
#define CORR_STR                "corr"
#define RESPOND_STR             "respond"
#define MAXAGE_STR              "maxAge"
#define MODE_STR                "mode"
#define LICENSE_STR             "license"
#define LICENSED_STR            "licensed"
#define COMPONENT_STR           "components"
#define PROTOCOL_STR            "protocol"
#define PROTOCOLS_STR           "protocols"
#define DEVICES_STR             "devices"
#define DISCOV_DEVICES_STR      "discoverDevices"
#define LIMITN_STR              "limitations"
#define CONF_ABOUT_STR          "about object details"
#define DEV_LIMIT_STR           "device max count"
#define ENGINE_MFG              "Dialog Semiconductor, A Renesas Company"
#define COPYRIGHT_STR           "Copyright 2019 - 2022 Dialog Semiconductor.  All Rights Reserved."
#define COPYRIGHTS_STR          "copyright"
#define FILETYPE                "filetype"
#define EXTENSION               "extension"
#define CLASS_STR               "class"
#define META_STR                "meta"
#define APPL_STR                "application"
#define TIMEOUT_STR             "timeouts"
#define CREATE_TM_STR           "Device create timeout ms"
#define PROV_TM_STR             "Device provision timeout ms"
#define DEPROV_TM_STR           "Device deprovision timeout ms"
#define DEL_TM_STR              "Device delete timeout ms"
#define REPLACE_TM_STR          "Device replace timeout ms"
#define TEST_TM_STR             "Device test timeout ms"
#define READ_TM_STR             "Datapoint read timeout ms"
#define WRITE_TM_STR            "Datapoint write timeout ms"
#define EVENT_TM_STR            "Datapoint enable event timeout ms"
#define EVENT_RENEW_STR         "Event subscription renewal timeout ms"
#define HEALTH_TM_STR           "Device health timeout ms"
#define HOLD_TIMER_TM_STR       "Feedback hold timer timeout ms"
#define DSTART_TIMER_TM_STR     "Discovery start callback timeout ms"
#define DSTEP_TIMER_TM_STR      "Discovery step callback timeout ms"
#define DSTOP_TIMER_TM_STR      "Discovery stop callback timeout ms"
#define RS485_PORT1_STR         "RS-485 1"
#define RS485_PORT2_STR         "RS-485 2"
#define PURPOSE_STR             "purpose"
#define MODBUS_PURPOSE_STR      "Modbus"
#define CONFIG_STR              "config"
#define BAUDRATE_STR            "baudrate"
#define PARITY_STR              "parity"
#define STOPBITS_STR            "stopbits"
#define ID_STR                  "id"
#define PORT_STR                "port"
#define COMMENT_STR             "comment"
#define UNIT_STR                "unit"
#define BASE_STR                "base"
#define REGEX_STR               "regex"
#define MAND_STR                "mandatory"
#define APPLIES_STR             "applies"
#define PROFILE_STR             "profile"
#define FAILURE_CODE_STR        "failureCode"
#define FAILURE_RSN_STR         "failureReason"
#define SID_STR                 "sid"
#define COMMAND_STR             "command"
#define FIELD_STR               "field"
#define EXCEPTION_STR           "exception"
#define BLOCKS_STR              "blocks"
//#define _STR ""

#define PID_LEN                 16
#define FILETYPE_METADATA       "filetype"
#define PID_METADATA            "program_ID"
#define DESC_METADATA           "description"
#define MFG_METADATA            "manufacturer"
#define VER_METADATA            "version"

#define MQTT_SUB_QOS            1
#define MQTT_PUB_QOS            1
#define RETAIN_TRUE             1
#define RETAIN_FALSE            0

#define FAST_MONITOR_RATE_SEC   5
#define NORMAL_MONITOR_RATE_SEC 30
#define SLOW_MONITOR_RATE_SEC   60
#define AUTO_MONITOR_RATE_SEC   30

typedef enum {
    NORMAL_MODE = 0,
    RESTORE_MODE
} operationMode;

typedef enum {
    RQ_CHANNEL = 0,
    FB_CHANNEL
} iapChannel;

typedef enum {
    DEVICE_TOPIC_TYPE,
    GET_TOPIC_TYPE,
    CONN_TOPIC_TYPE,
    MONITOR_TOPIC_TYPE
} topicType;

typedef enum {
    POLL_TIMER = 0,
    CALLBACK_TIMER,
    EVENT_CALLBACK_TIMER, //timer to check if the IDI has responded to event enable callback in time, else start 0.5sec polling
    RECEIVE_TIMEOUT_TIMER,
    ON_DEMAND_PUBLISH_TIMER
} timerType;

typedef enum {
    Err_NoError = 0,
    Err_ActionFail,
    Err_ActionNotSupp,
    Err_ActionNotFound,
    Err_UnlicensedDev,
    Err_TypeNotFound,
    Err_TypeInvalid,
    Err_TypeUnsupported,
    Err_DiscovActFail,
    Err_DevNotFound,
    Err_BlockNotFound,
    Err_DpNotFound,
    Err_UnidMissing,
    Err_UnidInvalid,
    Err_UnidExists,
    Err_ModbusTwoPorts,
    Err_IdiBusy,
    Err_ValueInvalid,
    Err_ValOutOfRange,
    Err_ValNegative,
    Err_DpReadOnly,
    Err_DevCommFail,
    Err_DiscovWriteFail,
    Err_AsciiWriteFail,
    Err_AsciiCallbackFunctionsFail,
    Err_Max
} ERR_STR_INDEX;

typedef enum {
    MODE_ONNET = 0,
    MODE_MAINT,
    MODE_OFFNET,
    MODE_MAX
} APOLLO_MODE;

typedef enum {
    XW_FieldIgnored,
    XE_InvalidField,
    XE_IAPTypeNotFound,
    XE_TypeNotSupported,
    XE_MissingField,
    XE_MissingColumn,
    XE_SameNativeScaledValue,
    XE_DuplicateField
} XifError;

typedef struct _SegmentInfo {
    APOLLO_MODE mode;
    char *sid;
    char evDataTopic[128];
    char evErrorTopic[128];
    char fbCfgTopic[128];
    char evDiscoveryTopic[128];
} SegmentInfo;

typedef struct _MosqMsg {
    unsigned int msgId; /**< message identification */
    char *topic; /**< message topic */
    char *payload; /**< message payload content */
    unsigned int payloadLen; /**< message pay load length */
    unsigned int qos; /**< message qos flag */
    unsigned int retain:1; /**< message retain flag */
} MosqMsg;

typedef struct _IdlPollData {
    int cb_index;
    pollRequestType type;
    int prio;
    int relinquish;
    double writeValue;
    char *writeStringValue;
    IdlDatapoint *dp;
    IdlIapDatapoint *iapdp;
    IdlInterfaceBlock *ifblock;
    IdlDev *dev;
} IdlPollData;

typedef struct _IdlDevActData {
    char *topic;
    char *payload;
    iapChannel channel;
} IdlDevActData;

typedef struct _IdlPendItemsData {
    char *topic;
    char *payload;
    I_PendingAct pending;
} IdlPendItemsData;

typedef struct _IdiMqttInfo {
    char *clientId;
    unsigned int cleanSession:1;
} IdiMqttInfo;

typedef struct _MultiChannel {
    unsigned int channelCount;
    ProtoChannel *channelArray;
} MultiChannel;

typedef struct _IdiDiscoveryInfo {
    unsigned int isDiscoverySupported:1;
    unsigned int discoveryType;
} IdiDiscoveryInfo;

typedef struct _IdiAboutInfo {
    int deviceLimitations;
    char *name;
    char *desc;
    char *filetype;
    char *fileExtension;
    char *licensed;
    char *manufacturer;
    char *copyright;
    char *version;
} IdiAboutInfo;

typedef struct _Timeouts {
    unsigned int devCreateTimeoutMs;
    unsigned int devProvisionTimeoutMs;
    unsigned int devDeprovisionTimeoutMs;
    unsigned int devDeleteTimeoutMs;
    unsigned int devTestTimeoutMs;
    unsigned int devReplaceTimeoutMs;
    unsigned int dpReadTimeoutMs;
    unsigned int dpWriteTimeoutMs;
    unsigned int dpEventTimeoutMs;
    unsigned int dpEventRenewalTimeoutMs;
    unsigned int healthCheckerTimeoutMs;
    unsigned int fbHoldTimerTimeoutMs;
    unsigned int startDiscoveryTimeoutMs;
    unsigned int stepDiscoveryTimeoutMs;
    unsigned int stopDiscoveryTimeoutMs;
} Timeouts;

typedef struct _RetryCounts {
    unsigned int dpRead;
    unsigned int dpWrite;
} RetryCounts;

typedef struct _QuantPollrate {
    float fastRateSec;
    float normalRateSec;
    float slowRateSec;
    float autoRateSec;
} QuantPollrate;

typedef struct _IdiConfig {
    unsigned int isProtocolModbus:1;
    unsigned int isEventSupported:1;
    unsigned int renewEventOnRestart:1;
    unsigned int receiveTimeoutSupported:1;
    unsigned int objectInstancesAreNumbers:1;
    unsigned int uidsAreNumbers:1;
    char *protocolIdentifier;
    char *xifDirAbsolutePath;
    IdiMqttInfo mqtt;
    MultiChannel multiChannelConfig;
    IdiDiscoveryInfo discov;
    IdiAboutInfo aboutDetails;
    Timeouts timeouts;
    RetryCounts retryCounts;
    QuantPollrate quantitativePollRate;
} IdiConfig;

/* ENGINE RELATED FUNTIONS */
int IdlUpdateModbusPortInfoStruct(ModbusPortInfo portInfo);
int IdlEngineInit(char *conf_path, Idl *idl);
int IdlCallbackIndexGenerator(void);

/* CONFIG FILE RELATED FUNCTIONS */
void IdiConfInit(void);
void IdlPublishModbusPortConfig(void);
int IdlParseModbusPortConfig(cJSON *config, ModbusPortInfo *portInfo);
int IdlConfFileParser(char *conf_path);

/* XIF FILE RELATED FUNCTIONS */
int IdlRemoveSeparatorsFromPid(char pidWithoutSeparators[], char *inputPid);
int IdlNormalisePid(char parsedPid[], char *inputPid);
int IdlGetPidFromXif(char *pid, Csv *c);
int IdlGetXif(char *xifDirPath, char *pid, char *xin, char *xifAbslPath);
int IdlCreateDeviceFromXif(char *topic, IdlDev *dev, char *xifPath, char **xifDpArray);
void IdlPublishTypeToResourcePublisherWithTypePrefix(char *pid, char *type);
void IdlScanResDirectory(char *xifDirPath, int methodType);
int IdlParseAndCompareStrings(char *string1, char *string2);

/* TIMER RELATED FUNCTIONS */
void IdlIdleTimerExpiry(uv_timer_t *handle);
void IdlInitHealthTimer(void);
void IdlHealthTimerStart(IdlDev *dev);
void IdlHealthTimerStop(void);
void IdlInitInterfaceBlocksTimer(IdlDev *dev);
void IdlInterfaceBlocksTimerStart(IdlDev *dev);
int IdlInterfaceBlocksTimerStop(IdlDev *dev);
void IdlInitNackTimer(IdlDev *dev);
void IdlNackTimerStart(IdlDev *dev);
int IdlInitAllDpTimers(IdlDev *dev);
int IdlStartDpTimer(IdlIapDatapoint *iapdp, IdlDatapoint *dp, int startTimeMs, timerType type);
void IdlStartDpPollTimersInField(IdlIapDatapoint *iapdp, IapTypeValueFields *field);
int IdlStartAllDpPollTimers(IdlDev *dev);
int IdlStopDpTimer(IdlIapDatapoint *iapdp, IdlDatapoint *dp, timerType type);
void IdlStopDpTimersInField(IdlIapDatapoint *iapdp, IapTypeValueFields *field);
int IdlStopAllDpTimers(IdlDev *dev);
int IdlStopAllDeviceTimers(IdlDev *dev);
void IdlAsyncInit(void);

/* IAP TYPE RELATED FUNCTIONS */
void IdlPublishTypeToResourcePublisher(char *type);
void IdlPublishAllTypesToResourcePublisher(void);
void IdlSaveAllTypeDefinitions(void);
void IdlSeparatePidAndType(char pid[], char type[], char *inputType);
bool IdlCheckForTypePrefixAndSuffix(char *type);
int IdlCheckEnumTypeDefinitionForDp(IapTypeValueFields *field, IdlDatapoint *dp);
int IdlCheckIfBaseTypesAndSaveToDp(IdlIapDatapoint *iapdp, char *inputType);
int IdlSaveTypeDefinitionToIapdp(char *topic, IdlIapDatapoint *iapdp, char *type, char extendedType[], int nativeTypeFlag);
void IdlIapTypeDefinitionHndl(char *topic, char *msg);

/* DEVICE RELATED FUNCTIONS */
IdlDev *IdlGetFirstDevice(void);
int IdlDeleteDeviceNode(IdlDev *dev);
IdlDev *IdlGetDevFromTopic(char *topic, topicType type);
int IdlFreeDevMem(IdlDev *dev);
IdlDev *IdlGetNextDeviceFromList(IdlDev *curNode);
int IdlIssuePendingDpWritesForAllDev(int devType);
int IdlIsUnidUnique(char *unid);
int IdlPublishDevStatusObject(IdlDev *dev, char *idempotentDevHandle);
int IdlPublishDevCfgObject(IdlDev *dev, char *idempotentDevHandle);
int IdlDevRevertBackOldUnid(IdlDev *dev);
void IdlSendPendingProvisionRequest(IdlDev *dev);
int IdlDevRqHndl(char *topic, char *msg);
int IdlDevFbHndl(char *topic, char *msg);

/* DATAPOINTS RELATED FUNCTIONS */
int IdlDeepTopicParser(char *deepTopic, char *msgData, char *normalTopic, cJSON **normalisedJson);
int IdlReverseTransformationEngine(IdlDatapoint *dp, double iapValue, double *raw);
cJSON *IdlGenerateIapdpValueObject(IdlIapDatapoint *iapdp, int prio, int ifDefault, int ifActual);
int IdlUpdateIapdpPollRate(IdlDev *dev, IdlIapDatapoint *iapdp);
void IdlCallEnableEventCallbackForIapdp(IdlDev *dev, IdlIapDatapoint *iapdp);
int IdlIssueDpReadRequest(IdlDev *dev, IdlInterfaceBlock *ifblock, IdlDatapoint *dp);
int IdlIssueDpWriteRequest(IdlDev *dev, IdlDatapoint *dp, int prio, int relinquish, double value, char *stringValue);
int IdlIssuePendingDpWrites(IdlDev *dev, int initialWrite);
int IdlInterfaceHndl(char *topic, char *msg, int iapChannel);
int IdlPublishAllInterfaceBlocks(IdlDev *dev);
int IdlPublishEvData(IdlDev *dev, IdlInterfaceBlock *ifblock, IdlIapDatapoint *iapdp);
int IdlFastPath(IdlDev *dev, IdlInterfaceBlock *ifblock, IdlIapDatapoint *iapdp, IdlDatapoint *dp);
int IdlAllocateIapDpContext(IdlDatapoint *dp, IdlIapDatapoint *iapdp, pollRequestType type, char *onDemandResponseTopic,
        char *onDemandCorr);
int IdlAllocateDpContext(IdlDatapoint *dp, pollRequestType type);
int IdlClearDpContext(IdlDatapoint *dp);
int IdlClearIapDpContext(IdlIapDatapoint *iapdp);
int IdlAllocateAndEnqueuePollRequest(IdlDev *dev, IdlInterfaceBlock *ifblock, IdlIapDatapoint *iapdp, IdlDatapoint *dp, int prio, int relinquish, double writeValue, char *writeStringValue, pollRequestType type, char *onDemandResponseTopic, char *onDemandCorr);
void IdlPollIapdpOnReceiveTimeout(IdlDev *dev, IdlInterfaceBlock *ifblock, IdlIapDatapoint *iapdp);
int IdlOnDemandReadHndl(char *topic, char *msg);
void IdlNackResponse(IdlDev *dev);
int IdlDpReadContextResult(IdlDev *dev, IdlInterfaceBlock *ifblock, IdlDatapoint *dp, 
        IdlErrorCodes error, char *prioArray, double value, char *stringValue);
void IdlProcessDpPrioArray(IdlDev *dev, IdlInterfaceBlock *ifblock, IdlDatapoint *dp, char *prioArray);
int IdlUpdateDpValue(IdlDev *dev, IdlInterfaceBlock *ifblock, IdlDatapoint *dp,
        char *prioArray, double value, char *stringValue);
int IdlPollFirstDp(IdlDev *dev, pollRequestType type);
int IdlPublishOnDemandReadValue(IdlDev *dev, IdlInterfaceBlock *ifblock, IdlIapDatapoint *iapdp);
void IdlEventDrivenMonitoringLoop(void);
IdlIapDatapoint *IdlGetIapDatapointByName(IdlDev *dev, char *dpName, char *blockName, int *blockIndex);
int IdlSendCustomDriverDpData(IdlDev *dev);

/* DEVICE DISCOVERY RELATED FUNTIONS */
int IdlCreateDiscoveryData(void);
int IdlDeviceDiscoveryHndl(char *msg);

/* DATAPOINT CONNECTIONS RELATED FUNCTIONS */
int IdlSendValueToConnectionDestinations(IdlIapDatapoint *iapdp);
int IdlConnectionMonitorHndl(char *topic, char *msg);
int IdlConnectionHndl(char *topic, char *msg);

/* COMMON UTILITY FUNCTIONS */
long log_time(void);
int IdlMosquittoSubscribe(char *topic, int subQos);
int IdlMosquittoUnsubscribe(char *topic);
int IdlCreateThread(pthread_t threadHndl, const char *name, int stackSize, 
        void *(*startRoutine) (void *), void *arg);
void IdlPtName(const char *ptName);
int IdlMemFree(void *mem);
int IdlCjsonDelete(cJSON *obj);
int IdlGetTopicField(char *topic, int key, char *field);
int IdlGetDevHandleFromTopic(char *topic, char *handle, topicType type);
int IdlGetBlockNameFromTopic(char *topic, char *blockname, topicType type);
int IdlGetBlockIndexFromTopic(char *topic, char *blockindex, topicType type);
int IdlGetDpNameFromTopic(char *topic, char *dpName, topicType type);
cJSON *IdlStringTocJSON(char *msg_data);
long long IdlGetTimeOfTheDayMs(void);
cJSON *IdlGenerateMruObject(void);
double MruObjectDiff(cJSON * jMru);
void IdlPublishError(IdlDev *dev, char *topic, const char *msgStr, int errorCode);
void IdlPublishXifError(char *topic, char *errorMsg);
int IdlDevIncrementOfflineCount(IdlDev *dev);
int IdlDevClearOfflineCount(IdlDev *dev);
int IdlCheckCallbackReturn(IdlDev *dev, int error, char *topic);
void IdlRemoveLeadZerosFromString(char * strdata);
int IdlCustomUnrecColFromXif(IdlDatapoint *dp, Csv *c, int col);

#endif
