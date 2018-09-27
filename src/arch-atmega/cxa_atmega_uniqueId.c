/**
 * @copyright 2018 opencxa.org
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
#include "cxa_uniqueId.h"


// ******** includes ********
#include <avr/io.h>
#include <avr/boot.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void init(void);


// ********  local variable declarations *********
static bool isInit = false;
static uint8_t id_bytes[10];
static char id_str[30];


// ******** global function implementations ********
void cxa_uniqueId_getBytes(uint8_t** bytesOut, size_t* numBytesOut)
{
	if( !isInit ) init();

	if( bytesOut ) *bytesOut = (uint8_t*)&id_bytes;
	if( numBytesOut ) *numBytesOut = sizeof(id_bytes);
}


char* cxa_uniqueId_getHexString(void)
{
	if( !isInit ) init();

	return id_str;
}


// ******** local function implementations ********
static void init(void)
{
	for( size_t i = 0; i < sizeof(id_bytes); i++ )
	{
		id_bytes[i] = boot_signature_byte_get(14+i);
	}

	sprintf(id_str, "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
			id_bytes[0], id_bytes[1], id_bytes[2], id_bytes[3], id_bytes[4],
			id_bytes[5], id_bytes[6], id_bytes[7], id_bytes[8], id_bytes[9] );
	id_str[sizeof(id_str)-1] = 0;

	isInit = true;
}
