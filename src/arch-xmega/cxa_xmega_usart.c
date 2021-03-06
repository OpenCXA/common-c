/*
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE', which is part of this source code package.
 *
 * @author Christopher Armenio
 */
#include "cxa_xmega_usart.h"


// ******** includes ********
#include <stdbool.h>
#include <avr/interrupt.h>
#include <cxa_assert.h>
#include <cxa_criticalSection.h>
#include <cxa_delay.h>
#include <cxa_xmega_clockController.h>
#include <cxa_xmega_pmic.h>


// ******** local macro definitions ********
#define RTS_CTS_OK								0
#define RTS_CTS_STOP							1
#define USART0_TX_PIN							3
#define USART0_RX_PIN							2
#define USART1_TX_PIN							7
#define USART1_RX_PIN							6

#define TXEN_TURNON_DELAY_MS					10
#define TXEN_TURNOFF_DELAY_MS					10


#ifndef CXA_XMEGA_USART_RX_INT_LEVEL
	#define CXA_XMEGA_USART_RX_INT_LEVEL		USART_RXCINTLVL_MED_gc
#endif

#ifndef CXA_XMEGA_USART_TX_INT_LEVEL
	#define CXA_XMEGA_USART_TX_INT_LEVEL		USART_DREINTLVL_MED_gc
#endif


// ******** local type definitions ********
typedef struct
{
	USART_t *const avrUsart;
	cxa_xmega_usart_t *cxaUsart;
}avrUsart_cxaUsart_map_entry_t;


// ******** local function prototypes ********
static void commonInit(cxa_xmega_usart_t *const usartIn, USART_t *avrUsartIn, const uint32_t baudRate_bpsIn, cxa_gpio_t *const rtsIn, cxa_gpio_t *const ctsIn, cxa_gpio_t *const txEnableIn);

static void usart_moduleClock_enable(cxa_xmega_usart_t *const usartIn);
static void usart_connectToPort(cxa_xmega_usart_t *const usartIn);
static bool usart_set_baudrate(cxa_xmega_usart_t *const usartIn, uint32_t baud_bpsIn);
static void avrCxaUsartMap_setCxaUsart(USART_t *const avrUsartIn, cxa_xmega_usart_t *const cxaUsartIn);
static cxa_xmega_usart_t* avrCxaUsartMap_getCxaUsart_fromAvrUsart(USART_t *const avrUsartIn);

static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn);
static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn);

static void criticalSection_cb_preEnter(void *userVar);
static void criticalSection_cb_postExit(void *userVar);
static inline void rxIsr(USART_t *const avrUsartIn);


// ********  local variable declarations *********
static avrUsart_cxaUsart_map_entry_t avrCxaUsartMap[] = {
	{&USARTC0, NULL},
	{&USARTC1, NULL},
	{&USARTD0, NULL},
	{&USARTD1, NULL},
	{&USARTE0, NULL},
#ifdef USARTE1
	{&USARTE1, NULL},
#endif
	{&USARTF0, NULL},
#ifdef USARTF1
	{&USARTF1, NULL},
#endif
};


// ******** global function implementations ********
void cxa_xmega_usart_init_noHH(cxa_xmega_usart_t *const usartIn, USART_t *avrUsartIn, const uint32_t baudRate_bpsIn)
{
	commonInit(usartIn, avrUsartIn, baudRate_bpsIn, NULL, NULL, NULL);
}


void cxa_xmega_usart_init_HH(cxa_xmega_usart_t *const usartIn, USART_t *avrUsartIn, const uint32_t baudRate_bpsIn,
	cxa_gpio_t *const rtsIn, cxa_gpio_t *const ctsIn)
{
	commonInit(usartIn, avrUsartIn, baudRate_bpsIn, rtsIn, ctsIn, NULL);
}


void cxa_xmega_usart_init_txEnable(cxa_xmega_usart_t *const usartIn, USART_t *avrUsartIn, const uint32_t baudRate_bpsIn,
								   cxa_gpio_t *const txEnableIn)
{
	commonInit(usartIn, avrUsartIn, baudRate_bpsIn, NULL, NULL, txEnableIn);
}


// ******** local function implementations ********
static void commonInit(cxa_xmega_usart_t *const usartIn, USART_t *avrUsartIn, const uint32_t baudRate_bpsIn, cxa_gpio_t *const rtsIn, cxa_gpio_t *const ctsIn, cxa_gpio_t *const txEnableIn)
{
	cxa_assert(usartIn);
	cxa_assert(avrUsartIn);

	// save our references
	usartIn->avrUsart = avrUsartIn;
	usartIn->cts = ctsIn;
	usartIn->rts = rtsIn;
	usartIn->txEnable = txEnableIn;

	// setup our handshaking pins
	usartIn->isHandshakingEnabled = (usartIn->cts != NULL) && (usartIn->rts != NULL);
	if( usartIn->isHandshakingEnabled )
	{
		cxa_gpio_setDirection(usartIn->cts, CXA_GPIO_DIR_INPUT);
		cxa_gpio_setPolarity(usartIn->cts, CXA_GPIO_POLARITY_NONINVERTED);
		cxa_gpio_setPolarity(usartIn->rts, CXA_GPIO_POLARITY_NONINVERTED);
		cxa_gpio_setValue(usartIn->rts, RTS_CTS_STOP);
		cxa_gpio_setDirection(usartIn->rts, CXA_GPIO_DIR_OUTPUT);
	}

	// setup our txEnable pin
	usartIn->isTxEnableEnabled = (usartIn->txEnable != NULL);
	if( usartIn->isTxEnableEnabled )
	{
		// initially set to 0
		cxa_gpio_setValue(usartIn->txEnable, 0);
	}

	// setup our fifos
	cxa_fixedFifo_init(&usartIn->rxFifo, CXA_FF_ON_FULL_DROP, sizeof(*usartIn->rxFifo_raw), (void *const)usartIn->rxFifo_raw, sizeof(usartIn->rxFifo_raw));
	cxa_fixedFifo_init(&usartIn->txFifo, CXA_FF_ON_FULL_DROP, sizeof(*usartIn->txFifo_raw), (void *const)usartIn->txFifo_raw, sizeof(usartIn->txFifo_raw));

	// enable power to our module
	usart_moduleClock_enable(usartIn);

	// asynchronous, 8,n,1...set baud rate
	usartIn->avrUsart->CTRLC = USART_CHSIZE_8BIT_gc;
	cxa_assert(usart_set_baudrate(usartIn, baudRate_bpsIn));

	// associate this cxa usart with the avrUsart (for interrupts)
	avrCxaUsartMap_setCxaUsart(usartIn->avrUsart, usartIn);

	// setup the usart subsystem to handle critical section events
	// so we can trigger CTS/RTS appropriately
	if( usartIn->isHandshakingEnabled ) cxa_criticalSection_addCallback(criticalSection_cb_preEnter, criticalSection_cb_postExit, NULL);

	// now enable our transmitter and receiver and connect to output pins!
	usart_connectToPort(usartIn);
	usartIn->avrUsart->CTRLB |= USART_TXEN_bm | USART_RXEN_bm;

	// finally, clear and enable rx interrupt (disable tx interrupt)
	usartIn->avrUsart->STATUS = 0;
	usartIn->avrUsart->CTRLA = (usartIn->avrUsart->CTRLA & 0xCCF) | CXA_XMEGA_USART_RX_INT_LEVEL;

	// setup our ioStream (last once everything is setup)
	cxa_ioStream_init(&usartIn->super.ioStream);
	cxa_ioStream_bind(&usartIn->super.ioStream, ioStream_cb_readByte, ioStream_cb_writeBytes, (void*)usartIn);

	// and de-assert our RTS line so our paired device knows it can transmit
	if( usartIn->isHandshakingEnabled ) cxa_gpio_setValue(usartIn->rts, RTS_CTS_OK);
}


static void usart_moduleClock_enable(cxa_xmega_usart_t *const usartIn)
{
	cxa_criticalSection_enter();

	     if( usartIn->avrUsart == &USARTC0 ) PR.PRPC &= ~(1 << PR_USART0_bp);
	else if( usartIn->avrUsart == &USARTC1 ) PR.PRPC &= ~(1 << PR_USART1_bp);
	else if( usartIn->avrUsart == &USARTD0 ) PR.PRPD &= ~(1 << PR_USART0_bp);
	else if( usartIn->avrUsart == &USARTD1 ) PR.PRPD &= ~(1 << PR_USART1_bp);
	else if( usartIn->avrUsart == &USARTE0 ) PR.PRPE &= ~(1 << PR_USART0_bp);
#ifdef USARTE1
	else if( usartIn->avrUsart == &USARTE1 ) PR.PRPE &= ~(1 << PR_USART1_bp);
#endif
	else if( usartIn->avrUsart == &USARTF0 ) PR.PRPF &= ~(1 << PR_USART0_bp);
#ifdef USARTF1
	else if( usartIn->avrUsart == &USARTF1 ) PR.PRPF &= ~(1 << PR_USART1_bp);
#endif
	else cxa_assert(0);

	cxa_criticalSection_exit();
}


static void usart_connectToPort(cxa_xmega_usart_t *const usartIn)
{
	cxa_assert(usartIn);

	if( usartIn->avrUsart == &USARTC0 ) PORTC.DIR = (PORTC.DIR & ~(1 << USART0_RX_PIN)) | (1 << USART0_TX_PIN);
	else if( usartIn->avrUsart == &USARTC1 ) PORTC.DIR = (PORTC.DIR & ~(1 << USART1_RX_PIN)) | (1 << USART1_TX_PIN);
	else if( usartIn->avrUsart == &USARTD0 ) PORTD.DIR = (PORTD.DIR & ~(1 << USART0_RX_PIN)) | (1 << USART0_TX_PIN);
	else if( usartIn->avrUsart == &USARTD1 ) PORTD.DIR = (PORTD.DIR & ~(1 << USART1_RX_PIN)) | (1 << USART1_TX_PIN);
	else if( usartIn->avrUsart == &USARTE0 ) PORTE.DIR = (PORTE.DIR & ~(1 << USART0_RX_PIN)) | (1 << USART0_TX_PIN);
#ifdef USARTE1
	else if( usartIn->avrUsart == &USARTE1 ) PORTE.DIR = (PORTE.DIR & ~(1 << USART1_RX_PIN)) | (1 << USART1_TX_PIN);
#endif
	else if( usartIn->avrUsart == &USARTF0 ) PORTF.DIR = (PORTF.DIR & ~(1 << USART0_RX_PIN)) | (1 << USART0_TX_PIN);
#ifdef USARTF1
	else if( usartIn->avrUsart == &USARTF1 ) PORTF.DIR = (PORTF.DIR & ~(1 << USART1_RX_PIN)) | (1 << USART1_TX_PIN);
#endif
	else cxa_assert(0);
}


static bool usart_set_baudrate(cxa_xmega_usart_t *const usartIn, uint32_t baudRate_bpsIn)
{
	int8_t exp;
	uint32_t div;
	uint32_t limit;
	uint32_t ratio;
	uint32_t min_rate;
	uint32_t max_rate;
	uint32_t cpu_hz = cxa_xmega_clockController_getSystemClockFrequency_hz();

	/*
	 * Check if the hardware supports the given baud rate
	 */
	// 8 = (2^0) * 8 * (2^0) = (2^BSCALE_MIN) * 8 * (BSEL_MIN)
	max_rate = cpu_hz / 8;
	// 4194304 = (2^7) * 8 * (2^12) = (2^BSCALE_MAX) * 8 * (BSEL_MAX+1)
	min_rate = cpu_hz / 4194304;

	if (!(usartIn->avrUsart->CTRLB & USART_CLK2X_bm)) {
		max_rate /= 2;
		min_rate /= 2;
	}

	if ((baudRate_bpsIn > max_rate) || (baudRate_bpsIn < min_rate)) {
		return false;
	}

	/*
	 * Check if double speed is enabled.
	 */
	if (!(usartIn->avrUsart->CTRLB & USART_CLK2X_bm)) {
		baudRate_bpsIn *= 2;
	}

	/*
	 * Find the lowest possible exponent.
	 */
	limit = 0xfffU >> 4;
	ratio = cpu_hz / baudRate_bpsIn;

	for (exp = -7; exp < 7; exp++) {
		if (ratio < limit) {
			break;
		}

		limit <<= 1;

		if (exp < -3) {
			limit |= 1;
		}
	}

	/*
	 * Depending on the value of exp, scale either the input frequency or
	 * the target baud rate. By always scaling upwards, we never introduce
	 * any additional inaccuracy.
	 *
	 * We are including the final divide-by-8 (aka. right-shift-by-3) in this
	 * operation as it ensures that we never exceeed 2**32 at any point.
	 *
	 * The formula for calculating BSEL is slightly different when exp is
	 * negative than it is when exp is positive.
	 */
	if (exp < 0) {
		/*
		 * We are supposed to subtract 1, then apply BSCALE. We want to apply
		 * BSCALE first, so we need to turn everything inside the parenthesis
		 * into a single fractional expression.
		 */
		cpu_hz -= 8 * baudRate_bpsIn;
		/*
		 * If we end up with a left-shift after taking the final divide-by-8
		 * into account, do the shift before the divide. Otherwise, left-shift
		 * the denominator instead (effectively resulting in an overall right
		 * shift.)
		 */
		if (exp <= -3) {
			div = ((cpu_hz << (-exp - 3)) + baudRate_bpsIn / 2) / baudRate_bpsIn;
		} else {
			baudRate_bpsIn <<= exp + 3;
			div = (cpu_hz + baudRate_bpsIn / 2) / baudRate_bpsIn;
		}
	} else {
		/*
		 * We will always do a right shift in this case, but we need to shift
		 * three extra positions because of the divide-by-8.
		 */
		baudRate_bpsIn <<= exp + 3;
		div = (cpu_hz + baudRate_bpsIn / 2) / baudRate_bpsIn - 1;
	}

	usartIn->avrUsart->BAUDCTRLB = (uint8_t)(((div >> 8) & 0X0F) | (exp << 4));
	usartIn->avrUsart->BAUDCTRLA = (uint8_t)div;

	return true;
}


static void avrCxaUsartMap_setCxaUsart(USART_t *const avrUsartIn, cxa_xmega_usart_t *const cxaUsartIn)
{
	cxa_assert(avrUsartIn);
	cxa_assert(cxaUsartIn);

	// iterate through our map to find our avrUsart
	for( size_t i = 0; i < (sizeof(avrCxaUsartMap)/sizeof(*avrCxaUsartMap)); i++ )
	{
		avrUsart_cxaUsart_map_entry_t *const currEntry = &avrCxaUsartMap[i];
		if( currEntry->avrUsart == avrUsartIn )
		{
			// we have a match, save the cxaUsart
			currEntry->cxaUsart = cxaUsartIn;
			return;
		}
	}

	// if we made it here, we couldn't find a matching avrUsart
	cxa_assert(0);
}


static cxa_xmega_usart_t* avrCxaUsartMap_getCxaUsart_fromAvrUsart(USART_t *const avrUsartIn)
{
	cxa_assert(avrUsartIn);

	// iterate through our map to find our avrUsart
	for( size_t i = 0; i < (sizeof(avrCxaUsartMap)/sizeof(*avrCxaUsartMap)); i++ )
	{
		avrUsart_cxaUsart_map_entry_t *const currEntry = &avrCxaUsartMap[i];
		if( currEntry->avrUsart == avrUsartIn ) return currEntry->cxaUsart;
	}

	// if we made it here, we couldn't find a matching avrUsart
	cxa_assert(0);
	return NULL;
}


static cxa_ioStream_readStatus_t ioStream_cb_readByte(uint8_t *const byteOut, void *const userVarIn)
{
	cxa_xmega_usart_t* usartIn = (cxa_xmega_usart_t*)userVarIn;
	cxa_assert(usartIn);

	return cxa_fixedFifo_dequeue(&usartIn->rxFifo, (void*)byteOut) ? CXA_IOSTREAM_READSTAT_GOTDATA : CXA_IOSTREAM_READSTAT_NODATA;
}


static bool ioStream_cb_writeBytes(void* buffIn, size_t bufferSize_bytesIn, void *const userVarIn)
{
	cxa_xmega_usart_t* usartIn = (cxa_xmega_usart_t*)userVarIn;
	cxa_assert(usartIn);

	size_t currByteIndex = 0;
	for( currByteIndex = 0; currByteIndex < bufferSize_bytesIn; currByteIndex++ )
	{
		// try to queue it up
		if( !cxa_fixedFifo_queue(&usartIn->txFifo, &(((uint8_t*)buffIn)[currByteIndex])) ) break;
	}

	// we either maxed our buffer, or we're done inserting...start sending
	bool isTxInterruptEnabled = (usartIn->avrUsart->CTRLA & 0x03);
	if( !isTxInterruptEnabled )
	{
		// send our current byte
		char txChar;
		if( cxa_fixedFifo_dequeue(&usartIn->txFifo, &txChar) )
		{
			if( usartIn->txEnable )
			{
				cxa_gpio_setValue(usartIn->txEnable, 1);
				cxa_delay_ms(TXEN_TURNON_DELAY_MS);
			}

			usartIn->avrUsart->DATA = txChar;
			usartIn->avrUsart->CTRLA = (usartIn->avrUsart->CTRLA & 0xFC) | CXA_XMEGA_USART_TX_INT_LEVEL;
		}
	}

	// try to add the rest
	while( (currByteIndex < bufferSize_bytesIn) )
	{
		if( cxa_fixedFifo_queue(&usartIn->txFifo, &(((uint8_t*)buffIn)[currByteIndex])) ) currByteIndex++;
	}


	/*
		// now that our previous transmission has finished, make sure our
		// receiver is ready to receive (if handshaking is enabled)
		while( usartIn->isHandshakingEnabled && (cxa_gpio_getValue(usartIn->cts) != RTS_CTS_OK) );

		// now send our data
		usartIn->avrUsart->DATA = ((uint8_t*)buffIn)[i];

		// wait for our interchar delay (if we're not using hardware handshaking AND we have one set
		if( !usartIn->isHandshakingEnabled && (usartIn->interCharDelay_ms > 0) ) cxa_delay_ms(usartIn->interCharDelay_ms);
	}
	*/
	return true;
}


static void criticalSection_cb_preEnter(void *userVar)
{
	// iterate through our map to find USARTS with handshaking enabled
	for( size_t i = 0; i < (sizeof(avrCxaUsartMap)/sizeof(*avrCxaUsartMap)); i++ )
	{
		avrUsart_cxaUsart_map_entry_t *const currEntry = &avrCxaUsartMap[i];
		if( (currEntry->cxaUsart != NULL) && (currEntry->cxaUsart->isHandshakingEnabled) )
		{
			// assert RTS so our paired device doesn't send any more data
			cxa_gpio_setValue(currEntry->cxaUsart->rts, RTS_CTS_STOP);
 		}
	}
}


static void criticalSection_cb_postExit(void *userVar)
{
	// iterate through our map to find USARTS with handshaking enabled
	for( size_t i = 0; i < (sizeof(avrCxaUsartMap)/sizeof(*avrCxaUsartMap)); i++ )
	{
		avrUsart_cxaUsart_map_entry_t *const currEntry = &avrCxaUsartMap[i];
		if( (currEntry->cxaUsart != NULL) && (currEntry->cxaUsart->isHandshakingEnabled) )
		{
			// de-assert RTS so our paired device knows it can send again
			cxa_gpio_setValue(currEntry->cxaUsart->rts, RTS_CTS_OK);
		}
	}
}


static inline void rxIsr(USART_t *const avrUsartIn)
{
	cxa_assert(avrUsartIn);
	cxa_xmega_usart_t *const cxaUsartIn = avrCxaUsartMap_getCxaUsart_fromAvrUsart(avrUsartIn);
	cxa_assert(cxaUsartIn);

	char rxChar = cxaUsartIn->avrUsart->DATA;
	cxa_fixedFifo_queue(&cxaUsartIn->rxFifo, (void*)&rxChar);
}


static inline void txIsr(USART_t *const avrUsartIn)
{
	cxa_assert(avrUsartIn);
	cxa_xmega_usart_t *const cxaUsartIn = avrCxaUsartMap_getCxaUsart_fromAvrUsart(avrUsartIn);
	cxa_assert(cxaUsartIn);

	char txChar;
	if( cxa_fixedFifo_dequeue(&cxaUsartIn->txFifo, &txChar) )
	{
		cxaUsartIn->avrUsart->DATA = txChar;
	}
	else
	{
		// no more data...stop interrupts
		cxaUsartIn->avrUsart->CTRLA &= 0xFC;
		if( cxaUsartIn->txEnable )
		{
			cxa_delay_ms(TXEN_TURNOFF_DELAY_MS);
			cxa_gpio_setValue(cxaUsartIn->txEnable, 0);
		}
	}
}


CXA_XMEGA_ISR_START(USARTC0_RXC_vect)
{
	rxIsr(&USARTC0);
}CXA_XMEGA_ISR_END

CXA_XMEGA_ISR_START(USARTC0_DRE_vect)
{
	txIsr(&USARTC0);
}CXA_XMEGA_ISR_END

CXA_XMEGA_ISR_START(USARTC1_RXC_vect)
{
	rxIsr(&USARTC1);
}CXA_XMEGA_ISR_END

CXA_XMEGA_ISR_START(USARTC1_DRE_vect)
{
	txIsr(&USARTC1);
}CXA_XMEGA_ISR_END

CXA_XMEGA_ISR_START(USARTD0_RXC_vect)
{
	rxIsr(&USARTD0);
}CXA_XMEGA_ISR_END

CXA_XMEGA_ISR_START(USARTD0_DRE_vect)
{
	txIsr(&USARTD0);
}CXA_XMEGA_ISR_END

CXA_XMEGA_ISR_START(USARTD1_RXC_vect)
{
	rxIsr(&USARTD1);
}CXA_XMEGA_ISR_END

CXA_XMEGA_ISR_START(USARTD1_DRE_vect)
{
	txIsr(&USARTD1);
}CXA_XMEGA_ISR_END

CXA_XMEGA_ISR_START(USARTE0_RXC_vect)
{
	rxIsr(&USARTE0);
}CXA_XMEGA_ISR_END

CXA_XMEGA_ISR_START(USARTE0_DRE_vect)
{
	txIsr(&USARTE0);
}CXA_XMEGA_ISR_END


#ifdef USARTE1
CXA_XMEGA_ISR_START(USARTE1_RXC_vect)
{
	rxIsr(&USARTE1);
}CXA_XMEGA_ISR_END

CXA_XMEGA_ISR_START(USARTE1_DRE_vect)
{
	txIsr(&USARTE1);
}CXA_XMEGA_ISR_END
#endif


CXA_XMEGA_ISR_START(USARTF0_RXC_vect)
{
	rxIsr(&USARTF0);
}CXA_XMEGA_ISR_END

CXA_XMEGA_ISR_START(USARTF0_DRE_vect)
{
	txIsr(&USARTF0);
}CXA_XMEGA_ISR_END


#ifdef USARTF1
CXA_XMEGA_ISR_START(USARTF1_RXC_vect)
{
	rxIsr(&USARTF1);
}CXA_XMEGA_ISR_END

CXA_XMEGA_ISR_START(USARTF1_DRE_vect)
{
	txIsr(&USARTF1);
}CXA_XMEGA_ISR_END
#endif
