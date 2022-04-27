#ifndef ETI_H
#define ETI_H

#include "mosquitto.h"

#define KEY_LENGTH              128
#define FIELD_LENGTH            128
#define UNID_LENGTH             128
#define TEMP_STR_LENGTH         1024

#define ETI_ACT_Q               "/dev_act_q_eti_%s"
#define ETI_MOSQ_CLIENT_ID      "eti_client_%s"

#define MQTT_SUB_QOS            1
#define MQTT_PUB_QOS            1
#define RETAIN_TRUE             1
#define RETAIN_FALSE            0

#define ETI_CAT_RD_STR          "rd"    /* read    */
#define ETI_CAT_WR_STR          "wr"    /* write   */
#define ETI_CAT_EV_STR          "ev"    /* event   */
#define ETI_CAT_IV_STR          "iv"    /* invalid */

#define WILDCARD_SUB_PLUS       "+"
#define WILDCARD_SUB_HASH       "#"

typedef enum {
    IV_CATEGORY = -1,   // category: invalid
    RD_CATEGORY,        // category: read
    WR_CATEGORY,        // category: write
    EV_CATEGORY         // category: event
} MsgCategory;


#define ETI_CAT_INDEX           3   // index 3 of ETI TOPIC {"rd", "wr", or "ev"}
#define ETI_DEV_KEY_INDEX       4   // index 4 of ETI Topic "dev"
#define ETI_UNID_INDEX          5   // index 5 of ETI topic $unid
#define ETI_REG_KEY_INDEX       6   // index 6 of ETI topic "reg"
#define ETI_REG_VAL_INDEX       7   // index 7 of ETI topic "$reg"

#define ETI_DEV_KEY             "dev"
#define ETI_REG_KEY             "reg"

#define ETI_CAT_TOPIC_FMT               "eti/" CDNAME "/%s"
#define ETI_CAT_DEV_KEY_TOPIC_FMT       "eti/" CDNAME "/%s/dev"
#define ETI_CAT_DEV_ID_TOPIC_FMT        "eti/" CDNAME "/%s/dev/%s"
#define ETI_CAT_DEV_REG_KEY_TOPIC_FMT   "eti/" CDNAME "/%s/dev/%s/reg"
#define ETI_CAT_DEV_REG_ID_TOPIC_FMT    "eti/" CDNAME "/%s/dev/%s/reg/%d"
#define ETI_CAT_DEV_SUBSC_TOPIC_FMT     "eti/" CDNAME "/%s/dev/%s/reg/#"


#define pDevOfNode(pNode)       (pNode->pDevSto)
#define unidOfNode(pNode)       (pNode->pDevSto->devUid)
#define pNextOfNode(pNode)      (pNode->pNext)
#define unidOf(pDev)            (pDev->devUid)
#define regMaxOf(pDev)          (pDev->devDpCounts)
#define regValVectorOf(pDev)    (pDev->pDevDpValVector)
#define regValOf(pDev, reg)     (pDev->pDevDpValVector[reg])
#define mosqOf(pDev)            (pDev->pDrvInfo->mosq)


typedef struct _DevActData {
    char *topic;
    char *payload;
    MsgCategory category;
} EtiDevActData;


extern pthread_t* EtiInit(T_DrvInfoPtr pDrvInfo);
extern int DevReadPublish(T__DevStoPtr pDev, uint reg);
extern int DevWritePublish(T__DevStoPtr pDev, uint reg, cJSON *pJsonNewVal);


#endif