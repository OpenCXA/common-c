/**
 * @file
 * @copyright 2015 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MQTT_CLIENT_H_
#define CXA_MQTT_CLIENT_H_


// ******** includes ********
#include <cxa_array.h>
#include <cxa_ioStream.h>
#include <cxa_logger_header.h>
#include <cxa_mqtt_protocolParser.h>
#include <cxa_stateMachine.h>
#include <cxa_timeDiff.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_MQTT_CLIENT_MAXNUM_LISTENERS
	#define CXA_MQTT_CLIENT_MAXNUM_LISTENERS			1
#endif

#ifndef CXA_MQTT_CLIENT_MAXNUM_SUBSCRIPTIONS
	#define CXA_MQTT_CLIENT_MAXNUM_SUBSCRIPTIONS		4
#endif


// ******** global type definitions *********
typedef struct cxa_mqtt_client cxa_mqtt_client_t;


typedef void (*cxa_mqtt_client_cb_onConnect_t)(cxa_mqtt_client_t *const clientIn, void* userVarIn);
typedef void (*cxa_mqtt_client_cb_onDisconnect_t)(cxa_mqtt_client_t *const clientIn, void* userVarIn);

typedef void (*cxa_mqtt_client_cb_onPublish_t)(cxa_mqtt_client_t *const clientIn, char* topicNameIn, void* payloadIn, size_t payloadLen_bytesIn, void* userVarIn);


/**
 * @private
 */
typedef struct
{
	cxa_mqtt_client_cb_onConnect_t cb_onConnect;
	cxa_mqtt_client_cb_onDisconnect_t cb_onDisconnect;

	void* userVar;
}cxa_mqtt_client_listenerEntry_t;


typedef enum
{
	CXA_MQTT_CLIENT_SUBSCRIPTION_STATE_UNACKNOWLEDGED,
	CXA_MQTT_CLIENT_SUBSCRIPTION_STATE_ACKNOWLEDGED,
	CXA_MQTT_CLIENT_SUBSCRIPTION_STATE_REFUSED
}cxa_mqtt_client_subscriptionState_t;


/**
 * @private
 */
typedef struct
{
	uint16_t packetId;
	cxa_mqtt_client_subscriptionState_t state;

	char* topicFilter;
	cxa_mqtt_protocolParser_qosLevel_t qos;
	cxa_mqtt_client_cb_onPublish_t cb_onPublish;

	void* userVar;
}cxa_mqtt_client_subscriptionEntry_t;


/**
 * @private
 */
struct cxa_mqtt_client
{
	cxa_mqtt_protocolParser_t mpp;

	cxa_array_t listeners;
	cxa_mqtt_client_listenerEntry_t listeners_raw[CXA_MQTT_CLIENT_MAXNUM_LISTENERS];

	cxa_array_t subscriptions;
	cxa_mqtt_client_subscriptionEntry_t subscriptions_raw[CXA_MQTT_CLIENT_MAXNUM_SUBSCRIPTIONS];

	cxa_stateMachine_t stateMachine;
	cxa_timeBase_t *timeBase;
	cxa_timeDiff_t td_timeout;
	cxa_timeDiff_t td_sendKeepAlive;
	cxa_timeDiff_t td_receiveKeepAlive;

	cxa_logger_t logger;

	char* clientId;
	bool hasSentConnectPacket;
	uint16_t currPacketId;
};


// ******** global function prototypes ********
void cxa_mqtt_client_init(cxa_mqtt_client_t *const clientIn, cxa_ioStream_t *const iosIn, cxa_timeBase_t *const timeBaseIn, char *const clientIdIn);

void cxa_mqtt_client_addListener(cxa_mqtt_client_t *const clientIn,
								 cxa_mqtt_client_cb_onConnect_t cb_onConnectIn,
								 cxa_mqtt_client_cb_onDisconnect_t cb_onDisconnectIn,
								 void *const userVarIn);

bool cxa_mqtt_client_connect(cxa_mqtt_client_t *const clientIn, char *const usernameIn, char *const passwordIn);
bool cxa_mqtt_client_isConnected(cxa_mqtt_client_t *const clientIn);
void cxa_mqtt_client_disconnect(cxa_mqtt_client_t *const clientIn);

bool cxa_mqtt_client_publish(cxa_mqtt_client_t *const clientIn, cxa_mqtt_protocolParser_qosLevel_t qosIn, bool retainIn,
							 char* topicNameIn, void *const payloadIn, size_t payloadLen_bytesIn);
void cxa_mqtt_client_subscribe(cxa_mqtt_client_t *const clientIn, char *topicFilterIn, cxa_mqtt_protocolParser_qosLevel_t qosIn, cxa_mqtt_client_cb_onPublish_t cb_onPublishIn, void* userVarIn);

void cxa_mqtt_client_update(cxa_mqtt_client_t *const clientIn);


#endif // CXA_MQTT_CLIENT_H_
