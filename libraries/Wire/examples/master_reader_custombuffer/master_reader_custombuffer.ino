// Wire Master Reader Custom Buffer

// Demonstrates use of the Wire library with customized buffers
// Reads data from an I2C/TWI slave device
// Refer to the "Wire Slave Sender Custom Buffer" example for use with this

// Created 31 Dec 2024

// This example code is in the public domain.


#include <Wire.h>
#include <TwoWireBuffers.h>
#include "Arduino.h"

#define USE_WIRE1 false // Set to true for using Wire1

// request 6 bytes from slave device #8
constexpr size_t REQUESTED_BYTE_COUNT = 6;

constexpr size_t RECEIVE_BUFFER_SIZE  = REQUESTED_BYTE_COUNT;
constexpr size_t TRANSMIT_BUFFER_SIZE = 0; // There is no transmit in this sketch.

#if not USE_WIRE1

SET_WIRE_BUFFERS(RECEIVE_BUFFER_SIZE, TRANSMIT_BUFFER_SIZE,
    true /* master buffers needed */, false /* no slave buffers needed */ );

void setup() {
  Wire.begin();        // join I2C bus (address optional for master)
  Serial.begin(9600);  // start serial for output

  // This is just for curiosity and could be removed
  printWireBuffersCapacity(Serial);
}

void loop() {
  Wire.requestFrom(8, REQUESTED_BYTE_COUNT);

  while (Wire.available()) { // slave may send less than requested
    const char c = Wire.read(); // receive a byte as character
    Serial.print(c);         // print the character
  }
  Serial.println();

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
  Wire1.begin();        // join I2C bus (address optional for master)
  Serial.begin(9600);   // start serial for output

  // This is just for curiosity and could be removed
  printWire1BuffersCapacity(Serial);
}

void loop() {
  Wire1.requestFrom(8, REQUESTED_BYTE_COUNT);

  while (Wire1.available()) { // slave may send less than requested
    const char c = Wire1.read(); // receive a byte as character
    Serial.print(c);         // print the character
  }
  Serial.println();

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