#include "common.h"

#ifdef INCLUDE_ETI

#include "eti.h"

/* Field key numbers start from 1, ie topic will be key-1/key-2/key-3/... */
static int GetTopicField(char *topic, int key, char *field)
{
    if (topic == NULL) {
        return (FAILURE);
    }
    char *token = NULL;
    char *rest = topic;
    int i = 0;
    for (i = 0; i < key; i++) {
        token = strtok_r(rest, "/", &rest);
    }
    if (token) {
        memcpy(field, token, 64);
        return (SUCCESS);
    } else {
        return (FAILURE);
    }
}


int DevReadPublish(T__DevStoPtr pDev, uint reg)
{
    char topicStr[MQTT_TOPIC_SIZE] = {0};
    int retVal = FAILURE;

	// ETI_CAT_DEV_REG_ID_TOPIC_FMT is not a retained topic
	// "eti/0/wr/dev/%s/reg/%d" {"value":"%s"}

	if (reg < regMaxOf(pDev)) {
		sprintf(topicStr, ETI_CAT_DEV_REG_ID_TOPIC_FMT, ETI_CAT_RD_STR, unidOf(pDev), reg);

        int pubQoS = MQTT_PUB_QOS;
        bool bRetain = RETAIN_FALSE;
		dbg_printf("Publishing:\nTopic:%s\nData:null - with QOS=%d, retain=%s\n", 
                    topicStr, pubQoS, (bRetain) ? "true" : "false");
		retVal = mosquitto_publish(mosqOf(pDev), NULL, topicStr, 0, NULL, pubQoS, bRetain);
		if (retVal != MOSQ_ERR_SUCCESS) {
			err_printf("ERROR %s: mosquitto_publish failed on topic=%s msg=null\n",
						__FUNCTION__, topicStr);
		}
	}
	else {
        err_printf("ERROR %s: reg[%d] is not a valid register\n", __FUNCTION__, reg);
	}
			
    return retVal;
}


int DevWritePublish(T__DevStoPtr pDev, uint reg, cJSON *pJsonNewVal)
{
    char topicStr[MQTT_TOPIC_SIZE] = {0};
    char *outStr = NULL;
    int retVal = FAILURE;

	// ETI_CAT_DEV_REG_ID_TOPIC_FMT is not a retained topic
	// "eti/0/wr/dev/%s/reg/%d" {"value":"%s"}

	if (regValVectorOf(pDev) && reg < regMaxOf(pDev)) {
		sprintf(topicStr, ETI_CAT_DEV_REG_ID_TOPIC_FMT, ETI_CAT_WR_STR, unidOf(pDev), reg);

		outStr = cJSON_PrintUnformatted(pJsonNewVal);
        int pubQoS = MQTT_PUB_QOS;
        bool bRetain = RETAIN_FALSE;
		dbg_printf("Publishing:\nTopic:%s\nData:%s - with QOS=%d, retain=%s\n", 
                    topicStr, outStr, pubQoS, (bRetain) ? "true" : "false");
		retVal = mosquitto_publish(mosqOf(pDev), NULL, topicStr, strlen(outStr), outStr, pubQoS, bRetain);
		if (retVal != MOSQ_ERR_SUCCESS) {
			err_printf("ERROR %s: mosquitto_publish failed on topic=%s msg=%s\n",
						__FUNCTION__, topicStr, outStr);
		}
		/* clean-up */
		free(outStr);
	}
	else {
		if (regValVectorOf(pDev))
			err_printf("ERROR %s: reg[%d] is not a valid register\n", __FUNCTION__, reg);
		else
			err_printf("ERROR %s:pDev->regValuesVector is not initialized\n", __FUNCTION__);
	}
			
    return retVal;
}


static int DevRegEvHndl(T__DevStoPtr pDev, char *pUnid, char *topic, char *msg) 
{
    int retVal = FAILURE;
    char tempStr[TEMP_STR_LENGTH] = {0};
    char regId[KEY_LENGTH] = {0};

    strcpy(tempStr, topic);
    if (GetTopicField(tempStr, ETI_REG_VAL_INDEX, regId) == SUCCESS) {
        // get the register id
        uint reg = strtol(regId, NULL, 10);
        info_printf("INFO %s: processing reg(id=%s/%d) unid=%s in the pDevStor(%p) list\n", 
                    __FUNCTION__, regId, reg, unidOf(pDev), pDev);
        if (reg < regMaxOf(pDev)) {
            T_DpValPtr pRegValEntry = &regValOf(pDev, reg);
            if (msg && strlen(msg) > 0) {
                info_printf("INFO %s: regId=%s, msg=%s\n", __FUNCTION__, regId,  msg);
                cJSON *pNewValJson = IdlStringTocJSON(msg);
                if (pNewValJson) {
                    char *pNewVal = cJSON_PrintUnformatted(pNewValJson);
                    info_printf("INFO %s: set pNewValJson=%s into reg[%d]\n", __FUNCTION__, pNewVal, reg);
                    if (*pRegValEntry) {
                        if (cJSON_Compare(*pRegValEntry, pNewValJson, 0)) {
                            // same value, no need to replace but free new value
                            cJSON_free(pNewValJson);
                        }
                        else {
                            // different value, free old and replace with new
                            cJSON_free(*pRegValEntry);
                            *pRegValEntry = pNewValJson;
                            info_printf("INFO %s: save pNewValJson=%s to reg[%s]\n", __FUNCTION__, pNewVal, regId);
                        }
                    }
                    else {
                        // no existing value, save new value
                        *pRegValEntry = pNewValJson;
                        info_printf("INFO %s: set pNewValJson=%s to reg[%s]\n", __FUNCTION__, pNewVal, regId);
                    }
                    cJSON_free(pNewVal);
                }
                else {
                    info_printf("INFO %s: pNewValJson is null for reg[%d]\n", __FUNCTION__, reg);
                }
            }
            else {
                cJSON_free(*pRegValEntry);
                *pRegValEntry = NULL;
                info_printf("INFO %s: no new value (null msg) for reg[%d]\n", __FUNCTION__, reg);
            }
        }
        else {
            err_printf("ERROR %s: reg[%u] is not a valid register in the ev topic=%s\n",
                        __FUNCTION__, reg, topic);
        }
    } else {
        err_printf("ERROR %s: no register id found in the ev topic=%s\n",
                    __FUNCTION__, topic);
    }

    return retVal;
}


static int DevEvHndl(T_DrvInfoPtr pDrvInfo, char *topic, char *msg)
{
    int retVal = FAILURE;
    char tempStr[TEMP_STR_LENGTH] = {0};
    char topicId[FIELD_LENGTH] = {0};
	char unid[UNID_LENGTH] = {0};

    // eti/0/%s/dev
    int len = sprintf(tempStr, ETI_CAT_DEV_KEY_TOPIC_FMT, ETI_CAT_EV_STR);
    if (topic && len > 0) {
        if (strncmp(topic, tempStr, strlen(tempStr)) == 0) {
            // matched eti/0/ev/dev 
            strcpy(tempStr, topic);
            if (GetTopicField(tempStr, ETI_UNID_INDEX, unid) == SUCCESS) {
                strcpy(tempStr, topic);
                if ((GetTopicField(tempStr, ETI_REG_KEY_INDEX, topicId) == SUCCESS) &&
                    (strcmp(topicId, ETI_REG_KEY) == 0)) {
                    bool bFound = false;

                    // We linearly walk through the list of nodes to find the proper node with
                    // matching unid.. It's not efficient but does the job for this eti example
                    T_DevNodePtr pDevStorNode = pDrvInfo->pHeadDevNode;
                    info_printf("INFO %s: searching for device with unid=%s in the pHeadDevNode(%p) list\n",
                                    __FUNCTION__, unid, pDrvInfo->pHeadDevNode);
                    while (pDevStorNode) {
                        info_printf("INFO %s: processing device with unid=%s in the pDevStorNode(%p) list\n", 
                                    __FUNCTION__, unidOfNode(pDevStorNode), pDevOfNode(pDevStorNode));
                        if (strcmp(unidOfNode(pDevStorNode), unid) == 0) {
                            // found the device node ptr
                            retVal = DevRegEvHndl(pDevOfNode(pDevStorNode), unidOfNode(pDevStorNode), topic, msg);
                            bFound = true;
                            break;
                        }
                        pDevStorNode = pNextOfNode(pDevStorNode);
                        // it's a doubly linked list
                        if (pDevStorNode == pDrvInfo->pHeadDevNode) {
                            info_printf("INFO %s: done processing (end of list)\n", __FUNCTION__);
                            break; // we are done
                        }
                    }
                    if (!bFound) {
                        err_printf("ERROR %s: unable to find device with unid=%s in the pHeadDevNode list\n", 
                                    __FUNCTION__, unid);
                    }
                }
                else {
                    err_printf("ERROR %s: no reg key found in the ev topic\n", __FUNCTION__);
                    err_printf("ERROR %s: ev topic; %s\n", __FUNCTION__, topic);
                }
            }
        }
        else {
            err_printf("ERROR %s: no matching of dev key found in the ev topic\n", __FUNCTION__);
            err_printf("ERROR %s: ev topic; %s\n", __FUNCTION__, topic);
        }
    }

    return retVal;
}


/* This thread handles all device related operations */
static void *vTaskDevAct(void *pvArg)
{
    int retVal = 0;
    EtiDevActData msg = {0};
	T_DrvInfoPtr pDrvInfo = (T_DrvInfoPtr) pvArg;
    info_printf("INFO %s: setting pDrvInfo(%p) from pvArg\n", __FUNCTION__, pDrvInfo);

    pthread_setname_np(pthread_self(), __FUNCTION__);       // <= 16 chars

    pDrvInfo->stat = EtiRunning;
    while (pDrvInfo->stat != EtiStop) {
        retVal = mq_receive(pDrvInfo->devActQueue, (char*)&msg, sizeof(EtiDevActData), NULL);
        if(retVal == -1) {
            continue;
        }
        if (msg.category == EV_CATEGORY) {
            retVal = DevEvHndl(pDrvInfo, msg.topic, msg.payload);
        }
        /* clean-up */
        free(msg.topic);
        free(msg.payload);
        msg.topic = NULL;
        msg.payload = NULL;
    }

    // We are done -- tidy up
    mosquitto_disconnect(pDrvInfo->mosq);
    mosquitto_destroy(pDrvInfo->mosq);
    mosquitto_lib_cleanup();

	return NULL;
}


static const char *GetCategoryStr(MsgCategory cat) {
	if (cat == RD_CATEGORY)
		return ETI_CAT_RD_STR;
	else if (cat == WR_CATEGORY)
		return ETI_CAT_WR_STR;
	else if (cat == EV_CATEGORY)
		return ETI_CAT_EV_STR;

	return ETI_CAT_IV_STR;
}


static void EtiMessageCb(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	EtiDevActData dmsg = {0};
	char devTopic[MQTT_TOPIC_SIZE] = {0};
	T_DrvInfoPtr pDrvInfo = (T_DrvInfoPtr)obj;
    info_printf("INFO %s: setting pDrvInfo(%p) from pvArg\n", __FUNCTION__, pDrvInfo);

	snprintf(devTopic, sizeof(devTopic), ETI_CAT_DEV_KEY_TOPIC_FMT, ETI_CAT_EV_STR);
	if (message->topic) {
		dmsg.topic = strdup(message->topic);
	} else {
		return;
	}   
	if (strncmp(message->topic, devTopic, strlen(devTopic)) == 0) {
		dmsg.category = EV_CATEGORY;
	}
	else {
		// no other category topic of interrest
		return;
	}

	if (message->payload) {
		dmsg.payload = (char *)calloc(message->payloadlen + 1, sizeof(char));
		if (dmsg.payload == NULL) {
			err_printf("ERROR %s: calloc failed\n", __FUNCTION__);
			free(dmsg.topic);
			return;
		}
		strncpy(dmsg.payload, (const char *)(message->payload), message->payloadlen);
		dmsg.payload[message->payloadlen] = 0;	// terminate the payload string
	} else {   
		dmsg.payload = NULL;
	}   

	dbg_printf("%s Topic :    %s\n", __FUNCTION__, dmsg.topic);
	dbg_printf("%s Data :     %s\n", __FUNCTION__, (char *)(dmsg.payload));
	dbg_printf("%s Category : %s\n", __FUNCTION__, GetCategoryStr(dmsg.category));

	if (mq_send(pDrvInfo->devActQueue, (const char*)&dmsg, sizeof(dmsg), 1) != 0) {    
		err_printf("WARN: %s- Failed to send data to devActQueue, err=%d\n",
				__FUNCTION__, errno);
	} else {
		dbg_printf("Sent msg on devActQueue\n");
	}
}



/* Sub to device category topic for single of wildcard device(s) */
static int SubToDeviceCatTopic(T_DrvInfoPtr pDrvInfo, const char *cat, const char *unid)
{
    char devTopic[128] = {0};
    /* We need to subscribe to device ev/rd/wr topics. 
     * All subscriptions can be done using a single topic with wildcards:
     *    eti/0/%s/dev/%s/reg/#
     */
	snprintf(devTopic, sizeof(devTopic), ETI_CAT_DEV_SUBSC_TOPIC_FMT, cat, unid);
	mosquitto_subscribe(pDrvInfo->mosq, NULL, devTopic, MQTT_SUB_QOS);
    return SUCCESS;
}


static int CreateQueue(mqd_t *queueHndl, const char *name, int isBlocking, int queueSize, int msgSize)
{
    long hardLimit = 0;
    int ret = FAILURE;
    int qFlags = 0;
    mode_t qPerms = {0};
    struct mq_attr qAttr = {0};
    struct rlimit mqLimit = {0};

    if (getrlimit(RLIMIT_MSGQUEUE, &mqLimit) == 0) {
        hardLimit = mqLimit.rlim_max;
        dbg_printf("Old mqLlimit -> soft limit= %ld, hard limit= %ld\n", mqLimit.rlim_cur, mqLimit.rlim_max); 
        if (hardLimit <= 0xfffffff) {
            hardLimit = 0xfffffff;
            mqLimit.rlim_max = hardLimit;
            mqLimit.rlim_cur = hardLimit;
            if (setrlimit(RLIMIT_MSGQUEUE, &mqLimit) == 0) {
                dbg_printf("successfully set limits\n");
                dbg_printf("new hardLimit = %ld\n", hardLimit);
            } else {
                dbg_printf("Error: could not set mqlimit, errno = %d\n", errno);
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
            dbg_printf("Unable to create mqueue %s, errno = %d\r\n", name, errno);
            if (errno == 17)                                                                          
            {   
                /* Delete existing queue and try again */
                mq_unlink(name);
                dbg_printf("Opening queue again after unlink\n");
                *queueHndl = mq_open(name, qFlags, qPerms, &qAttr);                           
                if (*queueHndl == (mqd_t) -1)
                {   
                    printf("ERROR: %s- Not able to create mqueue %s"
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


static int CreateThread(pthread_t *pThreadHndl, const char *name, int stackSize, 
        void *(*startRoutine) (void *), void *arg)
{
    int retVal = FAILURE;
    pthread_attr_t attr;

    retVal = pthread_attr_init(&attr);
    if (retVal != 0) 
    {
        err_printf("ERROR: %s- Failed to init pthread attr(%d)\n", 
                __FUNCTION__, retVal);
        return FAILURE;
    }
    /* create MQTT thread */
    if (pthread_attr_setstacksize(&attr, stackSize) != 0) {
        err_printf("ERROR: %s- Failed to set stackSize for thread %s (errmo: %d)\n", 
                __FUNCTION__, name, errno);
    } else if (pthread_create(pThreadHndl, &attr, startRoutine, arg) != 0) {
        err_printf("ERROR: %s- Failed to create pthread %s (errno: %d)\n", 
                __FUNCTION__, name, errno);
    } else {
        retVal = SUCCESS;
        dbg_printf("%s thread created successfully\n", name);
    }

    return retVal;
}


pthread_t* EtiInit(T_DrvInfoPtr pDrvInfo)
{
    static pthread_t threadDevAct = {0};  // static pthread object
	int rc = SUCCESS;

	rc = CreateQueue(&pDrvInfo->devActQueue, DEV_ACT_Q, BLOCKING_Q, MQ_HARD_LIM, sizeof(EtiDevActData));
	if (rc != SUCCESS) {
        info_printf("INFO: CreateQueue %s failed\n", DEV_ACT_Q);
	}
	else {
		info_printf("INFO: CreateQueue %s successful\n", DEV_ACT_Q);

        mosquitto_lib_init();
        info_printf("INFO %s: passing pDrvInfo(%p) to mosquitto_new for EtiMessageCb callback\n", 
                        __FUNCTION__, pDrvInfo);
        pDrvInfo->mosq = mosquitto_new(MOSQ_CLIENT_ID, true, pDrvInfo);
        
        //mosquitto_threaded_set(simDevInfo.mosq, true);
            
        /* Set up a callback function for receiving ETI MQTT    */
        /* messages for topics we subscribe to, e.g. rq and set */
        
        mosquitto_message_callback_set(pDrvInfo->mosq, EtiMessageCb);		// Set up a callback function 
            
        rc = mosquitto_connect(pDrvInfo->mosq, MQTT_BROKER_ADDR, MQTT_BROKER_PORT, 
                MQTT_DEFAULT_KEEP_ALIVE_TIME);
        if (rc) {
            err_printf("ERROR: %s- Can't connect to Mosquitto server. "
                    "mosquitto_connect failed with rc = %d\n", 
                    __FUNCTION__, rc);
            if (rc == MOSQ_ERR_ERRNO) {
                err_printf("rc = MOSQ_ERR_ERRNO. A system call returned errno %d\n", errno);
            }
        } else {
            info_printf("INFO: mosquitto_connect successful\n");

            /* Subscribe to the Example Test I/O (ETI) MQTT ev for wildcard dev topic */
            SubToDeviceCatTopic(pDrvInfo, ETI_CAT_EV_STR, WILDCARD_SUB_PLUS);
            
            if (CreateThread(&threadDevAct, "vTaskDevAct", TASK_DEVACT_STACK_SIZE, 
                                            vTaskDevAct, pDrvInfo) == SUCCESS) {
                if (mosquitto_loop_start(pDrvInfo->mosq) != 0) {
                    err_printf("ERROR: %s- mosquitto_loop_start failed\n", __FUNCTION__);
                    rc = FAILURE;
                }
            }
            else {
                rc = FAILURE;
                err_printf("ERROR: %s- create vTaskDevAct thread failed\n", __FUNCTION__);
            }

        }
	}

    if (rc) {
        // Tidy up
        mosquitto_disconnect(pDrvInfo->mosq);
        mosquitto_destroy(pDrvInfo->mosq);
        mosquitto_lib_cleanup();
        exit (EXIT_FAILURE);
    }

	return &threadDevAct;   // static pthread object for main to join
}


#endif