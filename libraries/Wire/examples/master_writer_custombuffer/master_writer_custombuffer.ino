// Wire Master Writer Custom Buffer

// Demonstrates use of the Wire library
// Writes data to an I2C/TWI slave device
// Refer to the "Wire Slave Receiver Custom Buffer" example for use with this

// Created 31 Dec 2024

// This example code is in the public domain.


#include <Wire.h>
#include <TwoWireBuffers.h>
#include "Arduino.h"

#define USE_WIRE1 false // Set to true for using Wire1

// The following text will not fit into the default buffer of 32 bytes.
static const char text[] = "You really won't believe it, but x is ";

constexpr size_t RECEIVE_BUFFER_SIZE  = 0;  // There is no receive in this sketch.
constexpr size_t TRANSMIT_BUFFER_SIZE = 42; // Enhance the buffer to 42 characters.

#if not USE_WIRE1

SET_WIRE_BUFFERS(RECEIVE_BUFFER_SIZE, TRANSMIT_BUFFER_SIZE,
    true /* master buffers needed */, false /* no slave buffers needed */ );

void setup() {
  Wire.begin(); // join I2C bus (address optional for master)

  // This is just for curiosity and can be removed
  Serial.begin(9600);   // start serial for output
  printWireBuffersCapacity(Serial);
}

static byte x = 0;

void loop() {
  Wire.beginTransmission(8); // transmit to device #8
  Wire.write(text);          // sends multiple bytes
  Wire.write(x);             // sends one byte
  Wire.endTransmission();    // stop transmitting

  x++;
  delay(500);
}

void printWireBuffersCapacity(Stream& stream) {
  const auto& buffers = GET_WIRE_BUFFERS();

  stream.print("Wire transmit buffer size is ");
  stream.println(buffers.txWireBufferCapacity());

  stream.print("Wire receive buffer size is ");
  stream.println(buffers.rxWireBufferCapacity());

  stream.print("Wire service buffer size is ");
  stream.println(buffers.srvWireBufferCapacity());
}

#else

SET_WIRE1_BUFFERS(RECEIVE_BUFFER_SIZE, TRANSMIT_BUFFER_SIZE,
    true /* master buffers needed */, false /* no slave buffers needed */ );

void setup() {
  Wire1.begin(); // join I2C bus (address optional for master)

  // This is just for curiosity and can be removed
  Serial.begin(9600);   // start serial for output
  printWire1BuffersCapacity(Serial);
}

static byte x = 0;

void loop() {
  Wire1.beginTransmission(8); // transmit to device #8
  Wire1.write(text);          // sends multiple bytes
  Wire1.write(x);             // sends one byte
  Wire1.endTransmission();    // stop transmitting

  x++;
  delay(500);
}

void printWire1BuffersCapacity(Stream& stream) {
  const auto& buffers = GET_WIRE1_BUFFERS();

  stream.print("Wire1 transmit buffer size is ");
  stream.println(buffers.txWireBufferCapacity());

  stream.print("Wire1 receive buffer size is ");
  stream.println(buffers.rxWireBufferCapacity());

  stream.print("Wire1 service buffer size is ");
  stream.println(buffers.srvWireBufferCapacity());
}

#endif
