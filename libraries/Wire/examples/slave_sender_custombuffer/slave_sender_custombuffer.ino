// Wire Slave Sender Custom Buffer

// Demonstrates use of the Wire library with customized buffers
// Sends data as an I2C/TWI slave device
// Refer to the "Wire Master Reader Custom Buffer" example for use with this

// Created 31 Dec 2024

// This example code is in the public domain.


#include <Wire.h>
#include <TwoWireBuffers.h>
#include "Arduino.h"

#define USE_WIRE1 false // Set to true for using Wire1

static const char text[] = "hello "; // respond with message of 6 bytes

constexpr size_t RECEIVE_BUFFER_SIZE  = 0; // There is no receive in this sketch.
constexpr size_t TRANSMIT_BUFFER_SIZE = sizeof(text)-1; // Don't need a byte for the \0

#if not USE_WIRE1

SET_WIRE_BUFFERS(RECEIVE_BUFFER_SIZE, TRANSMIT_BUFFER_SIZE,
    false /* no master buffers needed */, true /* slave buffers needed */ );

void setup() {
  Wire.begin(8);                // join I2C bus with address #8
  Wire.onRequest(requestEvent); // register event

  // This is just for curiosity and could be removed
  Serial.begin(9600);
  printWireBuffersCapacity(Serial);
}

void loop() {
  delay(100);
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
  Wire.write(text);
  // as expected by master
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
    false /* no master buffers needed */, true /* slave buffers needed */ );

void setup() {
  Wire1.begin(8);                // join I2C bus with address #8
  Wire1.onRequest(requestEvent); // register event

  // This is just for curiosity and could be removed
  Serial.begin(9600);
  printWire1BuffersCapacity(Serial);
}

void loop() {
  delay(100);
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
  Wire1.write(text);
  // as expected by master
}

void printWire1BuffersCapacity(Stream& stream) {
  const auto& buffers = GET_WIRE_BUFFERS();

  stream.print("Wire1 transmit buffer size is ");
  stream.println(buffers.txWireBufferCapacity());

  stream.print("Wire1 receive buffer size is ");
  stream.println(buffers.rxWireBufferCapacity());

  stream.print("Wire1 service buffer size is ");
  stream.println(buffers.srvWireBufferCapacity());
}

#endif
