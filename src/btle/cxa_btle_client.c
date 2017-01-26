/**
 * @copyright 2016 opencxa.org
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
#include "cxa_btle_client.h"


// ******** includes ********
#include <string.h>

#include <cxa_assert.h>


#define CXA_LOG_LEVEL			CXA_LOG_LEVEL_TRACE
#include <cxa_logger_implementation.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void parseAdvField(cxa_btle_advField_t *advFieldIn, uint8_t* bytesIn);


// ********  local variable declarations *********


// ******** global function implementations ********
void cxa_btle_client_init(cxa_btle_client_t *const btlecIn,
						  cxa_btle_client_scm_isReady_t scm_isReadyIn,
						  cxa_btle_client_scm_startScan_t scm_startScanIn,
						  cxa_btle_client_scm_stopScan_t scm_stopScanIn,
						  cxa_btle_client_scm_isScanning_t scm_isScanningIn,
						  cxa_btle_client_scm_startConnection_t scm_startConnectionIn,
						  cxa_btle_client_scm_stopConnection_t scm_stopConnectionIn,
						  cxa_btle_client_scm_isConnected_t scm_isConnectedIn,
						  cxa_btle_client_scm_writeToCharacteristic_t scm_writeToCharIn)
{
	cxa_assert(btlecIn);
	cxa_assert(scm_startScanIn);
	cxa_assert(scm_stopScanIn);
	cxa_assert(scm_isScanningIn);

	// save our references
	btlecIn->scms.isReady = scm_isReadyIn;
	btlecIn->scms.startScan = scm_startScanIn;
	btlecIn->scms.stopScan = scm_stopScanIn;
	btlecIn->scms.isScanning = scm_isScanningIn;
	btlecIn->scms.startConnection = scm_startConnectionIn;
	btlecIn->scms.stopConnection = scm_stopConnectionIn;
	btlecIn->scms.isConnected = scm_isConnectedIn;
	btlecIn->scms.writeToCharacteristic = scm_writeToCharIn;

	// clear our callbacks
	memset((void*)&btlecIn->cbs, 0, sizeof(btlecIn->cbs));
	cxa_array_initStd(&btlecIn->listeners, btlecIn->listeners_raw);
}


void cxa_btle_client_addListener(cxa_btle_client_t *const btlecIn,
								 cxa_btle_client_cb_onReady_t cb_onReadyIn,
								 cxa_btle_client_cb_onFailedInit_t cb_onFailedInitIn,
								 void* userVarIn)
{
	cxa_assert(btlecIn);

	cxa_btle_client_listener_entry_t newEntry =
	{
			.cb_onReady = cb_onReadyIn,
			.cb_onFailedInit = cb_onFailedInitIn,
			.userVar = userVarIn
	};
	cxa_assert(cxa_array_append(&btlecIn->listeners, &newEntry));
}


void cxa_btle_client_startScan_passive(cxa_btle_client_t *const btlecIn,
									   cxa_btle_client_cb_onScanStart_t cb_scanStartIn,
									   cxa_btle_client_cb_onAdvertRx_t cb_advIn,
									   void* userVarIn)
{
	cxa_assert(btlecIn);

	// save our callbacks
	btlecIn->cbs.scanning.onAdvert = cb_advIn;
	btlecIn->cbs.scanning.onScanStart = cb_scanStartIn;
	btlecIn->cbs.scanning.userVar = userVarIn;

	// start our scan
	cxa_assert(btlecIn->scms.startScan);
	btlecIn->scms.startScan(btlecIn, false);
}


void cxa_btle_client_startScan_active(cxa_btle_client_t *const btlecIn,
									  cxa_btle_client_cb_onScanStart_t cb_scanStartIn,
									  cxa_btle_client_cb_onAdvertRx_t cb_advIn,
									  cxa_btle_client_cb_onScanResponseRx_t cb_scanRespIn,
									  void* userVarIn)
{
	cxa_assert(btlecIn);

	// save our callbacks
	btlecIn->cbs.scanning.onAdvert = cb_advIn;
	btlecIn->cbs.scanning.onScanResp = cb_scanRespIn;
	btlecIn->cbs.scanning.onScanStart = cb_scanStartIn;
	btlecIn->cbs.scanning.userVar = userVarIn;

	// start our scan
	cxa_assert(btlecIn->scms.startScan);
	btlecIn->scms.startScan(btlecIn, false);
}


void cxa_btle_client_stopScan(cxa_btle_client_t *const btlecIn,
							  cxa_btle_client_cb_onScanStop_t cb_scanStopIn,
							  void* userVarIn)
{
	cxa_assert(btlecIn);

	// save our callbacks
	btlecIn->cbs.scanning.onScanStop = cb_scanStopIn;
	btlecIn->cbs.scanning.userVar = userVarIn;

	// stop our scan
	cxa_assert(btlecIn->scms.stopScan);
	btlecIn->scms.stopScan(btlecIn);
}


bool cxa_btle_client_isReady(cxa_btle_client_t *const btlecIn)
{
	cxa_assert(btlecIn);

	return (btlecIn->scms.isReady != NULL) ? btlecIn->scms.isReady(btlecIn) : true;
}


bool cxa_btle_client_isScanning(cxa_btle_client_t *const btlecIn)
{
	cxa_assert(btlecIn);

	cxa_assert(btlecIn->scms.isScanning != NULL);
	return btlecIn->scms.isScanning(btlecIn);
}


void cxa_btle_client_startConnection(cxa_btle_client_t *const btlecIn, cxa_eui48_t *const addrIn, bool isRandomAddrIn,
									 cxa_btle_client_cb_onConnectionOpened_t cb_connectionOpenedIn,
									 cxa_btle_client_cb_onConnectionClosed_t cb_connectionUnintentionallyClosedIn,
									 void* userVarIn)
{
	cxa_assert(btlecIn);

	// save our callbacks
	btlecIn->cbs.connecting.onConnectionOpened = cb_connectionOpenedIn;
	btlecIn->cbs.connecting.onConnectionClosed = cb_connectionUnintentionallyClosedIn;
	btlecIn->cbs.connecting.userVar = userVarIn;

	cxa_assert(btlecIn->scms.startConnection != NULL);
	btlecIn->scms.startConnection(btlecIn, addrIn, isRandomAddrIn);
}


void cxa_btle_client_stopConnection(cxa_btle_client_t *const btlecIn,
									cxa_btle_client_cb_onConnectionClosed_t cb_connectionClosedIn,
									void *userVarIn)
{
	cxa_assert(btlecIn);

	// save our callbacks
	btlecIn->cbs.connecting.onConnectionClosed = cb_connectionClosedIn;
	btlecIn->cbs.connecting.userVar = userVarIn;

	cxa_assert(btlecIn->scms.stopConnection != NULL);
	btlecIn->scms.stopConnection(btlecIn);
}


bool cxa_btle_client_isConnected(cxa_btle_client_t *const btlecIn)
{
	cxa_assert(btlecIn);

	return (btlecIn->scms.isConnected != NULL) ? btlecIn->scms.isConnected(btlecIn) : false;
}


void cxa_btle_client_writeToCharacteristic(cxa_btle_client_t *const btlecIn,
										   const char *const serviceIdIn,
										   const char *const characteristicIdIn,
										   cxa_fixedByteBuffer_t *const dataIn,
										   cxa_btle_client_cb_onWriteComplete_t cb_writeCompleteIn,
										   void* userVarIn)
{
	cxa_assert(btlecIn);

	// save our callbacks
	btlecIn->cbs.writing.onWriteComplete = cb_writeCompleteIn;
	btlecIn->cbs.writing.userVar = userVarIn;

	cxa_assert(btlecIn->scms.writeToCharacteristic != NULL);
	btlecIn->scms.writeToCharacteristic(btlecIn, serviceIdIn, characteristicIdIn, dataIn);
}


void cxa_btle_client_notify_onBecomesReady(cxa_btle_client_t *const btlecIn)
{
	cxa_assert(btlecIn);

	cxa_array_iterate(&btlecIn->listeners, currListener, cxa_btle_client_listener_entry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onReady != NULL ) currListener->cb_onReady(btlecIn, currListener->userVar);
	}
}


void cxa_btle_client_notify_onFailedInit(cxa_btle_client_t *const btlecIn, bool willAutoRetryIn)
{
	cxa_assert(btlecIn);

	cxa_array_iterate(&btlecIn->listeners, currListener, cxa_btle_client_listener_entry_t)
	{
		if( currListener == NULL ) continue;

		if( currListener->cb_onFailedInit != NULL ) currListener->cb_onFailedInit(btlecIn, willAutoRetryIn, currListener->userVar);
	}
}


void cxa_btle_client_notify_advertRx(cxa_btle_client_t *const btlecIn, cxa_btle_advPacket_t *packetIn)
{
	cxa_assert(btlecIn);

	if( btlecIn->cbs.scanning.onAdvert != NULL ) btlecIn->cbs.scanning.onAdvert(packetIn, btlecIn->cbs.scanning.userVar);
}


void cxa_btle_client_notify_scanStart(cxa_btle_client_t *const btlecIn, bool wasSuccessfulIn)
{
	cxa_assert(btlecIn);

	if( btlecIn->cbs.scanning.onScanStart != NULL ) btlecIn->cbs.scanning.onScanStart(btlecIn->cbs.scanning.userVar, wasSuccessfulIn);
}


void cxa_btle_client_notify_scanStop(cxa_btle_client_t *const btlecIn)
{
	cxa_assert(btlecIn);

	if( btlecIn->cbs.scanning.onScanStop != NULL ) btlecIn->cbs.scanning.onScanStop(btlecIn->cbs.scanning.userVar);
}


void cxa_btle_client_notify_connectionStarted(cxa_btle_client_t *const btlecIn, bool wasSuccessfulIn)
{
	cxa_assert(btlecIn);

	if( btlecIn->cbs.connecting.onConnectionOpened != NULL ) btlecIn->cbs.connecting.onConnectionOpened(btlecIn->cbs.connecting.userVar, wasSuccessfulIn);
}


void cxa_btle_client_notify_connectionClose(cxa_btle_client_t *const btlecIn)
{
	cxa_assert(btlecIn);

	if( btlecIn->cbs.connecting.onConnectionClosed != NULL ) btlecIn->cbs.connecting.onConnectionClosed(btlecIn->cbs.connecting.userVar);
}


void cxa_btle_client_notify_writeComplete(cxa_btle_client_t *const btlecIn, bool wasSuccessfulIn)
{
	cxa_assert(btlecIn);

	if( btlecIn->cbs.writing.onWriteComplete != NULL ) btlecIn->cbs.writing.onWriteComplete(btlecIn->cbs.writing.userVar, wasSuccessfulIn);
}


bool cxa_btle_client_countAdvFields(uint8_t *const bytesIn, size_t maxLen_bytesIn, size_t *const numAdvFieldsOut)
{
	int numAdvFields = 0;
	for( size_t i = 0; i < maxLen_bytesIn; i++ )
	{
		// at the start of a record...first byte is length
		int fieldLen = bytesIn[i];
		if( fieldLen == 0 ) break;

		if( (fieldLen + i) > maxLen_bytesIn )
		{
			return false;
		}

		numAdvFields++;
		i += fieldLen;
	}
	if( numAdvFieldsOut != NULL ) *numAdvFieldsOut = numAdvFields;
	return true;
}


void cxa_btle_client_notify_readComplete(cxa_btle_client_t *const btlecIn, bool wasSuccessfulIn)
{

}


bool cxa_btle_client_parseAdvFieldsForPacket(cxa_btle_advPacket_t *packetIn, size_t numAdvFieldsIn, uint8_t *const bytesIn, size_t maxLen_bytesIn)
{
	cxa_assert(packetIn);
	cxa_assert(bytesIn);

	if( numAdvFieldsIn > 0 )
	{
		size_t currByteIndex = 0;
		for( size_t i = 0; i < numAdvFieldsIn; i++ )
		{
			cxa_btle_advField_t* currField = (cxa_btle_advField_t*)cxa_array_append_empty(&packetIn->advFields);
			if( currField == NULL ) return false;

			parseAdvField(currField, &bytesIn[currByteIndex]);

			// +1 is for the length byte itself
			currByteIndex += currField->length + 1;
		}
	}

	return true;
}


// ******** local function implementations ********
static void parseAdvField(cxa_btle_advField_t *advFieldIn, uint8_t* bytesIn)
{
	cxa_assert(advFieldIn);
	cxa_assert(bytesIn);

	// first byte is always length byte
	advFieldIn->length = *bytesIn;
	bytesIn++;

	// next is type
	advFieldIn->type = (cxa_btle_advFieldType_t)*bytesIn;
	bytesIn++;

	// the rest depends on the type
	switch( advFieldIn->type )
	{
		case CXA_BTLE_ADVFIELDTYPE_FLAGS:
			advFieldIn->asFlags.flags = *bytesIn;
			break;

		case CXA_BTLE_ADVFIELDTYPE_TXPOWER:
			advFieldIn->asTxPower.txPower_dBm = *bytesIn;
			break;

		case CXA_BTLE_ADVFIELDTYPE_MAN_DATA:
			advFieldIn->asManufacturerData.companyId = (uint16_t)*bytesIn;
			bytesIn++;
			advFieldIn->asManufacturerData.companyId |= ((uint16_t)*bytesIn) << 8;
			bytesIn++;
			cxa_fixedByteBuffer_init_inPlace(&advFieldIn->asManufacturerData.manBytes, advFieldIn->length - 3, bytesIn, advFieldIn->length - 3);
			break;
	}
}
