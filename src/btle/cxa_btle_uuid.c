/**
 * @copyright 2017 opencxa.org
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
#include "cxa_btle_uuid.h"


// ******** includes ********
#include <string.h>

#include <cxa_assert.h>
#include <cxa_stringUtils.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********


// ********  local variable declarations *********


// ******** global function implementations ********
bool cxa_btle_uuid_initFromBuffer(cxa_btle_uuid_t *const uuidIn, cxa_fixedByteBuffer_t *const fbbIn, size_t indexIn, size_t numBytesIn)
{
	cxa_assert(uuidIn);
	cxa_assert(fbbIn);

	bool retVal = false;
	if( numBytesIn == 2 )
	{
		uuidIn->type = CXA_BTLE_UUID_TYPE_16BIT;
		retVal = cxa_fixedByteBuffer_get_uint16LE(fbbIn, indexIn, uuidIn->as16Bit);
	}
	else if( numBytesIn == 16 )
	{
		uuidIn->type = CXA_BTLE_UUID_TYPE_128BIT;
		retVal = cxa_uuid128_initFromBuffer(&uuidIn->as128Bit, fbbIn, indexIn);
	}
	return retVal;
}


bool cxa_btle_uuid_initFromString(cxa_btle_uuid_t *const uuidIn, const char *const strIn)
{
	cxa_assert(uuidIn);

	bool retVal = false;

	size_t strLen_bytes = strlen(strIn);
	if( strLen_bytes == 4 )
	{
		// 16-bit UUID
		uuidIn->type = CXA_BTLE_UUID_TYPE_16BIT;
		retVal =  cxa_stringUtils_hexStringToBytes(strIn, sizeof(uuidIn->as16Bit), true, (uint8_t*)&uuidIn->as16Bit);
	}
	else if( strLen_bytes == 36 )
	{
		// 128-bit
		uuidIn->type = CXA_BTLE_UUID_TYPE_16BIT;
		retVal = cxa_uuid128_initFromString(&uuidIn->as128Bit, strIn);
	}

	return retVal;
}


bool cxa_btle_uuid_isEqual(cxa_btle_uuid_t *const uuid1In, cxa_btle_uuid_t *const uuid2In)
{
	cxa_assert(uuid1In);
	cxa_assert(uuid2In);

	if( uuid1In->type != uuid2In->type ) return false;

	return (uuid1In->type == CXA_BTLE_UUID_TYPE_16BIT) ?
		   (uuid1In->as16Bit == uuid2In->as16Bit) :
		   cxa_uuid128_isEqual(&uuid1In->as128Bit, &uuid2In->as128Bit);
}


void cxa_btle_uuid_toString(cxa_btle_uuid_t *const uuidIn, cxa_btle_uuid_string_t *const strOut)
{
	cxa_assert(uuidIn);
	cxa_assert(strOut);

	switch( uuidIn->type )
	{
		case CXA_BTLE_UUID_TYPE_16BIT:
			cxa_stringUtils_bytesToHexString((uint8_t*)&uuidIn->as16Bit, 2, true, strOut->str, 5);
			break;

		case CXA_BTLE_UUID_TYPE_128BIT:
		{
			cxa_uuid128_string_t uuid128_str;
			cxa_uuid128_toString(&uuidIn->as128Bit, &uuid128_str);
			memcpy(strOut->str, uuid128_str.str, sizeof(strOut->str));
			break;
		}
	}
}


// ******** local function implementations ********
