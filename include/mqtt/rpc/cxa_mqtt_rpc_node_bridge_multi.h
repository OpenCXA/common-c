/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_MQTT_RPC_NODE_BRIDGE_MULTI_H_
#define CXA_MQTT_RPC_NODE_BRIDGE_MULTI_H_


// ******** includes ********
#include <cxa_mqtt_rpc_node_bridge.h>
#include <cxa_array.h>
#include <cxa_ioStream.h>
#include <cxa_logger_header.h>
#include <cxa_mqtt_rpc_node.h>
#include <cxa_protocolParser_mqtt.h>
#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_MQTT_RPC_NODE_BRIDGE_MAXNUM_REMOTE_NODES
	#define CXA_MQTT_RPC_NODE_BRIDGE_MAXNUM_REMOTE_NODES			4
#endif


// ******** global type definitions *********
typedef struct cxa_mqtt_rpc_node_bridge_multi cxa_mqtt_rpc_node_bridge_multi_t;

typedef cxa_mqtt_rpc_node_bridge_authorization_t (*cxa_mqtt_rpc_node_bridge_multi_cb_authenticateClient_t)(char *const clientIdIn, size_t clientIdLen_bytes,
																											char *const usernameIn, size_t usernameLen_bytesIn,
																											uint8_t *const passwordIn, size_t passwordLen_bytesIn,
																											char* provisionedNameOut, size_t maxProvisionedNameLen_bytesIn,
																											void *userVarIn);

/**
 * @private
 */
typedef struct
{
	char clientId[CXA_MQTT_RPC_NODE_BRIDGE_CLIENTID_MAXLEN_BYTES];
	char mappedName[CXA_MQTT_RPC_NODE_BRIDGE_MAPPEDNAME_MAXLEN_BYTES];
}cxa_mqtt_rpc_node_bridge_multi_remoteNodeEntry_t;


/**
 * @private
 */
struct cxa_mqtt_rpc_node_bridge_multi
{
	cxa_mqtt_rpc_node_bridge_t super;

	cxa_array_t remoteNodes;
	cxa_mqtt_rpc_node_bridge_multi_remoteNodeEntry_t remoteNodes_raw[CXA_MQTT_RPC_NODE_BRIDGE_MAXNUM_REMOTE_NODES];

	cxa_mqtt_rpc_node_bridge_multi_cb_authenticateClient_t cb_localAuth;
	void* localAuthUserVar;
};


// ******** global function prototypes ********
void cxa_mqtt_rpc_node_bridge_multi_init(cxa_mqtt_rpc_node_bridge_multi_t *const nodeIn, cxa_mqtt_rpc_node_t *const parentNodeIn,
										 cxa_protocolParser_mqtt_t *const mppIn,
										 const char *nameFmtIn, ...);

void cxa_mqtt_rpc_node_bridge_multi_setAuthCb(cxa_mqtt_rpc_node_bridge_multi_t *const nodeIn, cxa_mqtt_rpc_node_bridge_multi_cb_authenticateClient_t authCbIn, void *const userVarIn);

size_t cxa_mqtt_rpc_node_bridge_multi_getNumRemoteNodes(cxa_mqtt_rpc_node_bridge_multi_t *const nodeIn);
void cxa_mqtt_rpc_node_bridge_multi_clearRemoteNodes(cxa_mqtt_rpc_node_bridge_multi_t *const nodeIn);

#endif // CXA_MQTT_RPC_NODEBRIDGE_MULTI_H_
