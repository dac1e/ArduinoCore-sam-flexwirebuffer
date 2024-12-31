/*
 * TwoWire.h - TWI/I2C library for Arduino Due
 * Copyright (c) 2011 Cristian Maglie <c.maglie@arduino.cc>
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wunused-parameter"

extern "C" {
#include <string.h>
}

#include "Wire.h"
#include "TwoWireBuffers.h"

static inline bool TWI_FailedAcknowledge(Twi *pTwi) {
	return pTwi->TWI_SR & TWI_SR_NACK;
}

static inline bool TWI_WaitTransferComplete(Twi *_twi, uint32_t _timeout) {
	uint32_t _status_reg = 0;
	while ((_status_reg & TWI_SR_TXCOMP) != TWI_SR_TXCOMP) {
		_status_reg = TWI_GetStatus(_twi);

		if (_status_reg & TWI_SR_NACK)
			return false;

		if (--_timeout == 0)
			return false;
	}
	return true;
}

static inline bool TWI_WaitByteSent(Twi *_twi, uint32_t _timeout) {
	uint32_t _status_reg = 0;
	while ((_status_reg & TWI_SR_TXRDY) != TWI_SR_TXRDY) {
		_status_reg = TWI_GetStatus(_twi);

		if (_status_reg & TWI_SR_NACK)
			return false;

		if (--_timeout == 0)
			return false;
	}

	return true;
}

static inline bool TWI_WaitByteReceived(Twi *_twi, uint32_t _timeout) {
	uint32_t _status_reg = 0;
	while ((_status_reg & TWI_SR_RXRDY) != TWI_SR_RXRDY) {
		_status_reg = TWI_GetStatus(_twi);

		if (_status_reg & TWI_SR_NACK)
			return false;

		if (--_timeout == 0)
			return false;
	}

	return true;
}

static inline bool TWI_STATUS_SVREAD(uint32_t status) {
	return (status & TWI_SR_SVREAD) == TWI_SR_SVREAD;
}

static inline bool TWI_STATUS_SVACC(uint32_t status) {
	return (status & TWI_SR_SVACC) == TWI_SR_SVACC;
}

static inline bool TWI_STATUS_GACC(uint32_t status) {
	return (status & TWI_SR_GACC) == TWI_SR_GACC;
}

static inline bool TWI_STATUS_EOSACC(uint32_t status) {
	return (status & TWI_SR_EOSACC) == TWI_SR_EOSACC;
}

static inline bool TWI_STATUS_NACK(uint32_t status) {
	return (status & TWI_SR_NACK) == TWI_SR_NACK;
}

TwoWire::TwoWire(TwoWireBuffers::Interface& _twbi, Twi *_twi, void(*_beginCb)(void), void(*_endCb)(void)) :
    buffers(_twbi), twi(_twi), rxBufferIndex(0), rxBufferLength(0), txAddress(0),
			txBufferLength(0), srvBufferIndex(0), srvBufferLength(0), status(
					UNINITIALIZED), onBeginCallback(_beginCb), 
						onEndCallback(_endCb), twiClock(TWI_CLOCK) {
}

uint8_t* TwoWire::srvBuffer()const {return buffers.srvWireBuffer();}
size_t TwoWire::srvBufferCapacity()const {return buffers.srvWireBufferCapacity();}
uint8_t* TwoWire::rxBuffer()const {return buffers.rxWireBuffer();}
size_t TwoWire::rxBufferCapacity()const {return buffers.rxWireBufferCapacity();}
uint8_t* TwoWire::txBuffer()const {return buffers.txWireBuffer();}
size_t TwoWire::txBufferCapacity()const {return buffers.txWireBufferCapacity();}

void TwoWire::begin(void) {
	if (onBeginCallback)
		onBeginCallback();

	// Disable PDC channel
	twi->TWI_PTCR = UART_PTCR_RXTDIS | UART_PTCR_TXTDIS;

	TWI_ConfigureMaster(twi, twiClock, VARIANT_MCK);
	status = MASTER_IDLE;
}

void TwoWire::begin(uint8_t address) {
	if (onBeginCallback)
		onBeginCallback();

	// Disable PDC channel
	twi->TWI_PTCR = UART_PTCR_RXTDIS | UART_PTCR_TXTDIS;

	TWI_ConfigureSlave(twi, address);
	status = SLAVE_IDLE;
	TWI_EnableIt(twi, TWI_IER_SVACC);
	//| TWI_IER_RXRDY | TWI_IER_TXRDY	| TWI_IER_TXCOMP);
}

void TwoWire::begin(int address) {
	begin((uint8_t) address);
}

void TwoWire::end(void) {
	TWI_Disable(twi);

	// Enable PDC channel
	twi->TWI_PTCR &= ~(UART_PTCR_RXTDIS | UART_PTCR_TXTDIS);

	if (onEndCallback)
		onEndCallback();
}

void TwoWire::setClock(uint32_t frequency) {
	twiClock = frequency;
	TWI_SetClock(twi, twiClock, VARIANT_MCK);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint32_t iaddress, uint8_t isize, uint8_t sendStop) {
	if (quantity > rxBufferCapacity())
		quantity = rxBufferCapacity();

	// perform blocking read into buffer
	int readed = 0;
	TWI_StartRead(twi, address, iaddress, isize);

	uint8_t* const rxBuf = rxBuffer();
	do {
		// Stop condition must be set during the reception of last byte
		if (readed + 1 == quantity)
			TWI_SendSTOPCondition( twi);

		if (TWI_WaitByteReceived(twi, RECV_TIMEOUT))
			rxBuf[readed++] = TWI_ReadByte(twi);
		else
			break;
	} while (readed < quantity);
	TWI_WaitTransferComplete(twi, RECV_TIMEOUT);

	// set rx buffer iterator vars
	rxBufferIndex = 0;
	rxBufferLength = readed;

	return readed;
}

void TwoWire::beginTransmission(uint8_t address) {
	status = MASTER_SEND;

	// save address of target and empty buffer
	txAddress = address;
	txBufferLength = 0;
}

//
//	Originally, 'endTransmission' was an f(void) function.
//	It has been modified to take one parameter indicating
//	whether or not a STOP should be performed on the bus.
//	Calling endTransmission(false) allows a sketch to
//	perform a repeated start.
//
//	WARNING: Nothing in the library keeps track of whether
//	the bus tenure has been properly ended with a STOP. It
//	is very possible to leave the bus in a hung state if
//	no call to endTransmission(true) is made. Some I2C
//	devices will behave oddly if they do not see a STOP.
//
uint8_t TwoWire::endTransmission(uint8_t sendStop) {
	uint8_t error = 0;
	// transmit buffer (blocking)
		uint8_t* const txBuf = txBuffer();
	TWI_StartWrite(twi, txAddress, 0, 0, txBuf[0]);
	if (!TWI_WaitByteSent(twi, XMIT_TIMEOUT))
		error = 2;	// error, got NACK on address transmit
	
	if (error == 0) {
		uint16_t sent = 1;
		while (sent < txBufferLength) {
			TWI_WriteByte(twi, txBuf[sent++]);
			if (!TWI_WaitByteSent(twi, XMIT_TIMEOUT))
				error = 3;	// error, got NACK during data transmmit
		}
	}
	
	if (error == 0) {
		TWI_Stop(twi);
		if (!TWI_WaitTransferComplete(twi, XMIT_TIMEOUT))
			error = 4;	// error, finishing up
	}

	txBufferLength = 0;		// empty buffer
	status = MASTER_IDLE;
	return error;
}

//	This provides backwards compatibility with the original
//	definition, and expected behaviour, of endTransmission
//
uint8_t TwoWire::endTransmission(void)
{
	return endTransmission(true);
}

size_t TwoWire::write(uint8_t data) {
	if (status == MASTER_SEND) {
		if (txBufferLength >= txBufferCapacity())
			return 0;
		txBuffer()[txBufferLength++] = data;
		return 1;
	} else {
		if (srvBufferLength >= srvBufferCapacity())
			return 0;
		srvBuffer()[srvBufferLength++] = data;
		return 1;
	}
}

size_t TwoWire::write(const uint8_t *data, size_t quantity) {
	if (status == MASTER_SEND) {
    uint8_t* const txBuf = txBuffer();
		for (size_t i = 0; i < quantity; ++i) {
			if (txBufferLength >= txBufferCapacity())
				return i;
			txBuf[txBufferLength++] = data[i];
		}
	} else {
	  uint8_t* const srvBuf = srvBuffer();
		for (size_t i = 0; i < quantity; ++i) {
			if (srvBufferLength >= srvBufferCapacity())
				return i;
			srvBuf[srvBufferLength++] = data[i];
		}
	}
	return quantity;
}

int TwoWire::available(void) {
	return rxBufferLength - rxBufferIndex;
}

int TwoWire::read(void) {
	if (rxBufferIndex < rxBufferLength)
		return rxBuffer()[rxBufferIndex++];
	return -1;
}

int TwoWire::peek(void) {
	if (rxBufferIndex < rxBufferLength)
		return rxBuffer()[rxBufferIndex];
	return -1;
}

void TwoWire::flush(void) {
	// Do nothing, use endTransmission(..) to force
	// data transfer.
}

void TwoWire::onReceive(void(*function)(int)) {
	onReceiveCallback = function;
}

void TwoWire::onRequest(void(*function)(void)) {
	onRequestCallback = function;
}

void TwoWire::onService(void) {
	// Retrieve interrupt status
	uint32_t sr = TWI_GetStatus(twi);

	if (status == SLAVE_IDLE && TWI_STATUS_SVACC(sr)) {
		TWI_DisableIt(twi, TWI_IDR_SVACC);
		TWI_EnableIt(twi, TWI_IER_RXRDY | TWI_IER_GACC | TWI_IER_NACK
				| TWI_IER_EOSACC | TWI_IER_SCL_WS | TWI_IER_TXCOMP);

		srvBufferLength = 0;
		srvBufferIndex = 0;

		// Detect if we should go into RECV or SEND status
		// SVREAD==1 means *master* reading -> SLAVE_SEND
		if (!TWI_STATUS_SVREAD(sr)) {
			status = SLAVE_RECV;
		} else {
			status = SLAVE_SEND;

			// Alert calling program to generate a response ASAP
			if (onRequestCallback)
				onRequestCallback();
			else
				// create a default 1-byte response
				write(static_cast<uint8_t>(0));
		}
	}

	if (status != SLAVE_IDLE && TWI_STATUS_EOSACC(sr)) {
		if (status == SLAVE_RECV && onReceiveCallback) {
			// Copy data into rxBuffer
			// (allows to receive another packet while the
			// user program reads actual data)

		  uint8_t* const srvBuff = srvBuffer();
		  uint8_t* const rxBuff = rxBuffer();
		  for (size_t i = 0; i < srvBufferLength; ++i) {
				rxBuff[i] = srvBuff[i];
		  }

			rxBufferIndex = 0;
			rxBufferLength = srvBufferLength;

			// Alert calling program
			onReceiveCallback( rxBufferLength);
		}

		// Transfer completed
		TWI_EnableIt(twi, TWI_SR_SVACC);
		TWI_DisableIt(twi, TWI_IDR_RXRDY | TWI_IDR_GACC | TWI_IDR_NACK
				| TWI_IDR_EOSACC | TWI_IDR_SCL_WS | TWI_IER_TXCOMP);
		status = SLAVE_IDLE;
	}

	if (status == SLAVE_RECV) {
		if (TWI_STATUS_RXRDY(sr)) {
			if (srvBufferLength < srvBufferCapacity())
				srvBuffer()[srvBufferLength++] = TWI_ReadByte(twi);
		}
	}

	if (status == SLAVE_SEND) {
		if (TWI_STATUS_TXRDY(sr) && !TWI_STATUS_NACK(sr)) {
			uint8_t c = 'x';
			if (srvBufferIndex < srvBufferLength)
				c = srvBuffer()[srvBufferIndex++];
			TWI_WriteByte(twi, c);
		}
	}
}

#if WIRE_INTERFACES_COUNT > 0
static void Wire_Init(void) {
	pmc_enable_periph_clk(WIRE_INTERFACE_ID);
	PIO_Configure(
			g_APinDescription[PIN_WIRE_SDA].pPort,
			g_APinDescription[PIN_WIRE_SDA].ulPinType,
			g_APinDescription[PIN_WIRE_SDA].ulPin,
			g_APinDescription[PIN_WIRE_SDA].ulPinConfiguration);
	PIO_Configure(
			g_APinDescription[PIN_WIRE_SCL].pPort,
			g_APinDescription[PIN_WIRE_SCL].ulPinType,
			g_APinDescription[PIN_WIRE_SCL].ulPin,
			g_APinDescription[PIN_WIRE_SCL].ulPinConfiguration);

	NVIC_DisableIRQ(WIRE_ISR_ID);
	NVIC_ClearPendingIRQ(WIRE_ISR_ID);
	NVIC_SetPriority(WIRE_ISR_ID, 0);
	NVIC_EnableIRQ(WIRE_ISR_ID);
}

static void Wire_Deinit(void) {
	NVIC_DisableIRQ(WIRE_ISR_ID);
	NVIC_ClearPendingIRQ(WIRE_ISR_ID);

	pmc_disable_periph_clk(WIRE_INTERFACE_ID);

	// no need to undo PIO_Configure, 
	// as Peripheral A was enable by default before,
	// and pullups were not enabled
}

TwoWire Wire = TwoWire(WireBuffers::instance(), WIRE_INTERFACE, Wire_Init, Wire_Deinit);

void WIRE_ISR_HANDLER(void) {
	Wire.onService();
}
#endif

#if WIRE_INTERFACES_COUNT > 1
static void Wire1_Init(void) {
	pmc_enable_periph_clk(WIRE1_INTERFACE_ID);
	PIO_Configure(
			g_APinDescription[PIN_WIRE1_SDA].pPort,
			g_APinDescription[PIN_WIRE1_SDA].ulPinType,
			g_APinDescription[PIN_WIRE1_SDA].ulPin,
			g_APinDescription[PIN_WIRE1_SDA].ulPinConfiguration);
	PIO_Configure(
			g_APinDescription[PIN_WIRE1_SCL].pPort,
			g_APinDescription[PIN_WIRE1_SCL].ulPinType,
			g_APinDescription[PIN_WIRE1_SCL].ulPin,
			g_APinDescription[PIN_WIRE1_SCL].ulPinConfiguration);

	NVIC_DisableIRQ(WIRE1_ISR_ID);
	NVIC_ClearPendingIRQ(WIRE1_ISR_ID);
	NVIC_SetPriority(WIRE1_ISR_ID, 0);
	NVIC_EnableIRQ(WIRE1_ISR_ID);
}

static void Wire1_Deinit(void) {
	NVIC_DisableIRQ(WIRE1_ISR_ID);
	NVIC_ClearPendingIRQ(WIRE1_ISR_ID);

	pmc_disable_periph_clk(WIRE1_INTERFACE_ID);

	// no need to undo PIO_Configure, 
	// as Peripheral A was enable by default before,
	// and pullups were not enabled
}

TwoWire Wire1 = TwoWire(Wire1Buffers::instance(), WIRE1_INTERFACE, Wire1_Init, Wire1_Deinit);

void WIRE1_ISR_HANDLER(void) {
	Wire1.onService();
}

#endif

#pragma GCC diagnostic pop
