/*
  twi.h - TWI/I2C library for Wiring & Arduino
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#ifndef TwiBuffers_h
#define TwiBuffers_h

#include <stdint.h>
#include <stddef.h>
#include <Wire.h>

namespace TwoWireBuffers {

/* Template class that implements an compile time fixed size array. */
template<unsigned CAPACITY>
class StaticBuffer {
  uint8_t mByteArray[CAPACITY];
public:
  inline uint8_t capacity() const {return CAPACITY;}
  inline uint8_t* storage() {return mByteArray;}
};

/* Specialization of StaticBuffer template class with zero size. */
template<>
class StaticBuffer<0> {
public:
  inline uint8_t capacity() const {return 0;}
  inline uint8_t* storage() {return nullptr;}
};


/* Interface that provides buffers for twi driver and TwoWire objects */
class Interface {
public:
  virtual uint8_t* rxWireBuffer() = 0;
  virtual size_t rxWireBufferCapacity()const = 0;

  virtual uint8_t* txWireBuffer() = 0;
  virtual size_t txWireBufferCapacity()const = 0;

  virtual uint8_t* srvWireBuffer() = 0;
  virtual size_t srvWireBufferCapacity()const = 0;
};

/* Template class implementing Interface with template parameter
 * determined buffer sizes.
 */
template<
  size_t RX_CAPACITY, // Receive buffer size. May be zero, if only transmitting data is needed
  size_t TX_CAPACITY  // Transmit buffer size. May be zero, if only receiving data is needed
  >
class Impl : public Interface {
  // Service buffer is neede for transmit and receive.
  static constexpr size_t SRV_CAPACITY =
      RX_CAPACITY > TX_CAPACITY ? RX_CAPACITY : TX_CAPACITY;

  // Set the capacity for a TwoWire object.
  TwoWireBuffers::StaticBuffer<RX_CAPACITY> mRxWireBuffer;
  TwoWireBuffers::StaticBuffer<TX_CAPACITY> mTxWireBuffer;
  TwoWireBuffers::StaticBuffer<SRV_CAPACITY> mSrvWireBuffer;

public:
  virtual uint8_t* rxWireBuffer() override {return mRxWireBuffer.storage();}
  virtual size_t rxWireBufferCapacity()const override {return mRxWireBuffer.capacity();}

  virtual uint8_t* txWireBuffer() override {return mTxWireBuffer.storage();}
  virtual size_t txWireBufferCapacity()const override {return mTxWireBuffer.capacity();}

  virtual uint8_t* srvWireBuffer() override {return mSrvWireBuffer.storage();}
  virtual size_t srvWireBufferCapacity()const override {return mSrvWireBuffer.capacity();}
};

} // namespace TwoWireBuffers

template<size_t wireNum> struct WireBuffers { // The buffers for the Wire object
  static TwoWireBuffers::Interface& instance();
};

#define SET_WIRE_BUFFERS_(wireNum, rxBufferCapacity, txBufferCapacity, enableMaster, enableSlave) \
    template<> TwoWireBuffers::Interface& WireBuffers<wireNum>::instance() { \
      static TwoWireBuffers::Impl<rxBufferCapacity, txBufferCapacity> buffers; \
      return buffers; \
    }

#define GET_WIRE_BUFFERS_(wireNum) WireBuffers<wireNum>::instance()

#if WIRE_INTERFACES_COUNT > 0
  #define SET_WIRE_BUFFERS(rxBufferCapacity, txBufferCapacity, enableMaster, enableSlave) \
    SET_WIRE_BUFFERS_(0, rxBufferCapacity, txBufferCapacity, enableMaster, enableSlave)

  #define GET_WIRE_BUFFERS() GET_WIRE_BUFFERS_(0)
#endif

#if WIRE_INTERFACES_COUNT > 1
  #define SET_WIRE1_BUFFERS(rxBufferCapacity, txBufferCapacity, enableMaster, enableSlave) \
     SET_WIRE_BUFFERS_(1, rxBufferCapacity, txBufferCapacity, enableMaster, enableSlave)

  #define GET_WIRE1_BUFFERS() GET_WIRE_BUFFERS_(1)
#endif

#endif /* TwiBuffers_h */
