// Wire1 connnected to Wire. (scl <-> scl1, sda <-> sda1)

// Demonstrates use of the Wire library on a single Arduino board
// with 2 Wire interfaces (like Arduino Due).
// Uses the option of customizing the buffers.
//
// Wire reads data from an I2C/TWI slave device
// Wire1 sends data as an I2C/TWI slave device

// Created 02 Jan 2025

// This example code is in the public domain.


#include <Wire.h>
#include <TwoWireBuffers.h>
#include "Arduino.h"

static_assert(WIRE_INTERFACES_COUNT > 1, "You need two I2C interfaces on the Arduino board to run this sketch");

static const char text[] = "hello "; // respond with message of 6 bytes

// Wire is the master reader
constexpr size_t M_RECEIVE_BUFFER_SIZE  = sizeof(text)-1; // Don't need a byte for the \0
constexpr size_t M_TRANSMIT_BUFFER_SIZE = 0; // There is no transmit in this sketch.
SET_WIRE_BUFFERS(M_RECEIVE_BUFFER_SIZE, M_TRANSMIT_BUFFER_SIZE,
    true /* master buffers needed */, false /* no slave buffers needed */ );

// Wire1 is the slave sender
constexpr size_t S_RECEIVE_BUFFER_SIZE  = 0; // There is no receive in this sketch.
constexpr size_t S_TRANSMIT_BUFFER_SIZE = sizeof(text)-1; // Don't need a byte for the \0
SET_WIRE1_BUFFERS(S_RECEIVE_BUFFER_SIZE, S_TRANSMIT_BUFFER_SIZE,
    false /* no master buffers needed */, true /* slave buffers needed */ );

void setup() {
  Serial.begin(9600);            // start serial for output
  Wire.begin();                  // master joins I2C bus (address optional for master)
  Wire1.begin(8);                // slave joins I2C bus with address #8
  Wire1.onRequest(requestEvent); // register slave event

  // This is just for curiosity and could be removed
  printWireBuffersCapacity(Serial);
  printWire1BuffersCapacity(Serial);
}

void loop() {
  Wire.requestFrom(8, M_RECEIVE_BUFFER_SIZE);

  while (Wire.available()) {
    const char c = Wire.read(); // receive a byte as character
    Serial.print(c);            // print the character
  }
  Serial.println();

  delay(500);
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
  Wire1.write(text);
  // as expected by master
}

// print Wire buffer sizes
void printWireBuffersCapacity(Stream& stream) {
  const auto& buffers = GET_WIRE_BUFFERS();

  stream.print("Wire transmit buffer size is ");
  stream.println(buffers.txWireBufferCapacity());

  stream.print("Wire receive buffer size is ");
  stream.println(buffers.rxWireBufferCapacity());

  stream.print("Wire service buffer size is ");
  stream.println(buffers.srvWireBufferCapacity());
}

// print Wire1 buffer sizes
void printWire1BuffersCapacity(Stream& stream) {
  const auto& buffers = GET_WIRE1_BUFFERS();

  stream.print("Wire1 transmit buffer size is ");
  stream.println(buffers.txWireBufferCapacity());

  stream.print("Wire1 receive buffer size is ");
  stream.println(buffers.rxWireBufferCapacity());

  stream.print("Wire1 service buffer size is ");
  stream.println(buffers.srvWireBufferCapacity());
}
