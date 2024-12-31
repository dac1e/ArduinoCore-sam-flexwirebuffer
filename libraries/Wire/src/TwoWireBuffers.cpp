/*
  twi.c - TWI/I2C library for Wiring & Arduino
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

#include "TwoWireBuffers.h"
#include "variant.h"

constexpr size_t RX_BUFFER_DEFAULT_LENGTH = 32;
constexpr size_t TX_BUFFER_DEFAULT_LENGTH = 32;

#if WIRE_INTERFACES_COUNT > 0
// Default buffers for the Wire object
namespace WireBuffers {
  __attribute__((weak)) SET_BUFFERS_FOR_BOTH(RX_BUFFER_DEFAULT_LENGTH, TX_BUFFER_DEFAULT_LENGTH);
} // namespace Twi
#endif

#if WIRE_INTERFACES_COUNT > 1
// Default buffers for the Wire1 object
namespace Wire1Buffers {
  __attribute__((weak)) SET_BUFFERS_FOR_BOTH(RX_BUFFER_DEFAULT_LENGTH, TX_BUFFER_DEFAULT_LENGTH);
} // namespace Twi
#endif
