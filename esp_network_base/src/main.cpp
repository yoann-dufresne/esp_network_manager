#include <Arduino.h>

#include "Network.hpp"


Network net;

void setup() {
  Serial.begin(115200);
  delay(3000);
  net.connect_network();
}

void loop() {
  while (not net.connect_server(1)) {
    delay(5000);
  };
  
  uint8_t idx = 0;
  while (net.send(&idx, 1))
  {
    idx += 1;
    delay(1000);
  }
}