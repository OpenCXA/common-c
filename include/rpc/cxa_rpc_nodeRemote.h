/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#ifndef CXA_RPC_NODEREMOTE_H_
#define CXA_RPC_NODEREMOTE_H_


// ******** includes ********
#include <stdbool.h>
#include <cxa_array.h>
#include <cxa_logger_header.h>
#include <cxa_timeBase.h>
#include <cxa_rpc_messageHandler.h>
#include <cxa_protocolParser.h>
#include <cxa_timeDiff.h>

#include <cxa_config.h>


// ******** global macro definitions ********
#ifndef CXA_RPC_NODEREMOTE_MAXNUM_LINK_LISTENERS
	#define CXA_RPC_NODEREMOTE_MAXNUM_LINK_LISTENERS		2
#endif


// ******** global type definitions *********
/**
 * @public
 */
typedef void (*cxa_rpc_nodeRemote_cb_linkEstablished_t)(cxa_rpc_nodeRemote_t *const nrIn, void* userVarIn);


/**
 * @private
 */
typedef struct
{
	cxa_rpc_nodeRemote_cb_linkEstablished_t cb_linkEstablished;
	void *userVar;
}cxa_rpc_nodeRemote_linkListener_t;


/**
 * @private
 */
struct cxa_rpc_nodeRemote
{
	cxa_rpc_messageHandler_t super;
	bool isProvisioned;

	cxa_protocolParser_t protocolParser;
	cxa_rpc_node_t *downstreamSubNode;

	cxa_timeDiff_t td_provision;

	cxa_array_t linkListeners;
	cxa_rpc_nodeRemote_linkListener_t linkListeners_raw[CXA_RPC_NODEREMOTE_MAXNUM_LINK_LISTENERS];
};


// ******** global function prototypes ********
void cxa_rpc_nodeRemote_init_upstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_ioStream_t *const ioStreamIn, cxa_timeBase_t *const timeBaseIn);
bool cxa_rpc_nodeRemote_init_downstream(cxa_rpc_nodeRemote_t *const nrIn, cxa_ioStream_t *const ioStreamIn, cxa_timeBase_t *const timeBaseIn, cxa_rpc_node_t *const subNodeIn);
void cxa_rpc_nodeRemote_deinit(cxa_rpc_nodeRemote_t *const nrIn);

bool cxa_rpc_nodeRemote_addLinkListener(cxa_rpc_nodeRemote_t *const nrIn, cxa_rpc_nodeRemote_cb_linkEstablished_t cb_linkEstablishedIn, void *const userVarIn);

void cxa_rpc_nodeRemote_update(cxa_rpc_nodeRemote_t *const nrIn);

#endif // CXA_RPC_NODEREMOTE_H_
