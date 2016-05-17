/**
 * Copyright 2016 opencxa.org
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cxa_awcu300_gpio.h"


/**
 * @author Christopher Armenio
 */


// ******** includes ********
#include <stdio.h>
#include <cxa_assert.h>


// ******** local macro definitions ********


// ******** local type definitions ********


// ******** local function prototypes ********
static void configurePinMux(const GPIO_NO_Type pinNumIn);


// ********  local variable declarations *********
static bool isClockEnabled = false;


// ******** global function implementations ********
void cxa_awcu300_gpio_init_input(cxa_awcu300_gpio_t *const gpioIn, const GPIO_NO_Type pinNumIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );
	
	// save our references
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;
	
	configurePinMux(pinNumIn);

	// set our initial direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_INPUT);
}


void cxa_awcu300_gpio_init_output(cxa_awcu300_gpio_t *const gpioIn, const GPIO_NO_Type pinNumIn, const cxa_gpio_polarity_t polarityIn, const bool initValIn)
{
	cxa_assert(gpioIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );
	
	// save our references
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = polarityIn;
	
	configurePinMux(pinNumIn);

	// set our initial value (before we set direction to avoid glitches)
	cxa_gpio_setValue(&gpioIn->super, initValIn);
	
	// now set our direction
	cxa_gpio_setDirection(&gpioIn->super, CXA_GPIO_DIR_OUTPUT);
}


void cxa_awcu300_gpio_init_safe(cxa_awcu300_gpio_t *const gpioIn, const GPIO_NO_Type pinNumIn)
{
	cxa_assert(gpioIn);
	cxa_assert(pinNumIn < 8);
	
	// save our references
	gpioIn->pinNum = pinNumIn;
	gpioIn->polarity = CXA_GPIO_POLARITY_NONINVERTED;
	gpioIn->dir = CXA_GPIO_DIR_UNKNOWN;
	
	configurePinMux(pinNumIn);

	// don't set any value or direction...just leave everything as it is
}


void cxa_gpio_setDirection(cxa_gpio_t *const superIn, const cxa_gpio_direction_t dirIn)
{
	cxa_assert(superIn);
	cxa_assert( (dirIn == CXA_GPIO_DIR_INPUT) ||
				(dirIn == CXA_GPIO_DIR_OUTPUT) );

	// get a pointer to our class
	cxa_awcu300_gpio_t *const gpioIn = (cxa_awcu300_gpio_t *const)superIn;
	
	GPIO_SetPinDir(gpioIn->pinNum, (dirIn == CXA_GPIO_DIR_OUTPUT) ? GPIO_OUTPUT : GPIO_INPUT);
	gpioIn->dir = dirIn;
}


cxa_gpio_direction_t cxa_gpio_getDirection(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_awcu300_gpio_t *const gpioIn = (cxa_awcu300_gpio_t *const)superIn;
	
	return gpioIn->dir;
}


void cxa_gpio_setPolarity(cxa_gpio_t *const superIn, const cxa_gpio_polarity_t polarityIn)
{
	cxa_assert(superIn);
	cxa_assert( (polarityIn == CXA_GPIO_POLARITY_NONINVERTED) ||
				(polarityIn == CXA_GPIO_POLARITY_INVERTED) );

	// get a pointer to our class
	cxa_awcu300_gpio_t *const gpioIn = (cxa_awcu300_gpio_t *const)superIn;
				
	gpioIn->polarity = polarityIn;
}


cxa_gpio_polarity_t cxa_gpio_getPolarity(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);
	
	// get a pointer to our class
	cxa_awcu300_gpio_t *const gpioIn = (cxa_awcu300_gpio_t *const)superIn;
	
	return gpioIn->polarity;
}


void cxa_gpio_setValue(cxa_gpio_t *const superIn, const bool valIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_awcu300_gpio_t *const gpioIn = (cxa_awcu300_gpio_t *const)superIn;

	GPIO_WritePinOutput(gpioIn->pinNum, (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? valIn : valIn);
}


bool cxa_gpio_getValue(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	cxa_awcu300_gpio_t *const gpioIn = (cxa_awcu300_gpio_t *const)superIn;

	bool retVal = (GPIO_ReadPinLevel(gpioIn->pinNum) == GPIO_IO_HIGH);
	return (gpioIn->polarity == CXA_GPIO_POLARITY_INVERTED) ? !retVal : retVal;
}


void cxa_gpio_toggle(cxa_gpio_t *const superIn)
{
	cxa_assert(superIn);

	// get a pointer to our class
	//cxa_awcu300_gpio_t *const gpioIn = (cxa_awcu300_gpio_t *const)superIn;
	
	cxa_gpio_setValue(superIn, !cxa_gpio_getValue(superIn));
}


// ******** local function implementations ********
static void configurePinMux(const GPIO_NO_Type pinNumIn)
{
	if( !isClockEnabled )
	{
		CLK_ModuleClkEnable(CLK_GPIO);
		isClockEnabled = true;
	}

	GPIO_PinMuxFunc_Type pinMuxFunc = PINMUX_FUNCTION_0;
	if( (pinNumIn == GPIO_6 ) ||
		(pinNumIn == GPIO_7 ) ||
		(pinNumIn == GPIO_8 ) ||
		(pinNumIn == GPIO_9 ) ||
		(pinNumIn == GPIO_10) ||
		(pinNumIn == GPIO_22) ||
		(pinNumIn == GPIO_23) ||
		(pinNumIn == GPIO_24) ||
		(pinNumIn == GPIO_25) ||
		(pinNumIn == GPIO_26) ||
		(pinNumIn == GPIO_28) ||
		(pinNumIn == GPIO_29) ||
		(pinNumIn == GPIO_30) ||
		(pinNumIn == GPIO_31) ||
		(pinNumIn == GPIO_32) ||
		(pinNumIn == GPIO_33) )
	{
		pinMuxFunc = PINMUX_FUNCTION_1;
	}

	GPIO_PinMuxFun(pinNumIn, pinMuxFunc);
}

