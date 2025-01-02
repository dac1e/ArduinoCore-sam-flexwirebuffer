// Wire Slave Receiver Custom Buffer

// Demonstrates use of the Wire library with customized buffers
// Receives data as an I2C/TWI slave device
// Refer to the "Wire Master Writer Custom Buffer" example for use with this

// Created 31 Dec 2024

// This example code is in the public domain.


#include <Wire.h>
#include <TwoWireBuffers.h>
#include "Arduino.h"

#define USE_WIRE1 false // Set to true for using Wire1

constexpr size_t RECEIVE_BUFFER_SIZE  = 42; // Be able receive up to 42 characters in one message.
constexpr size_t TRANSMIT_BUFFER_SIZE = 0;  // There is no transmit in this sketch.

#if not USE_WIRE1

SET_WIRE_BUFFERS(RECEIVE_BUFFER_SIZE, TRANSMIT_BUFFER_SIZE,
    false /* no master buffers needed */, true /* slave buffers needed */ );

void setup() {
  Wire.begin(8);                // join I2C bus with address #8
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(9600);           // start serial for output

  // This is just for curiosity and could be removed
  printWireBuffersCapacity(Serial);
}

void loop() {
  delay(100);
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {
  while (1 < Wire.available()) { // loop through all but the last
    const char c = Wire.read(); // receive byte as a character
    Serial.print(c);         // print the character
  }
  const int x = Wire.read();    // receive byte as an integer
  Serial.println(x);         // print the integer
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
  Wire1.onReceive(receiveEvent); // register event
  Serial.begin(9600);           // start serial for output

  // This is just for curiosity and could be removed
  printWire1BuffersCapacity(Serial);
}

void loop() {
  delay(100);
}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveEvent(int howMany) {
  while (1 < Wire1.available()) { // loop through all but the last
    const char c = Wire1.read(); // receive byte as a character
    Serial.print(c);         // print the character
  }
  const int x = Wire1.read();    // receive byte as an integer
  Serial.println(x);         // print the integer
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
