#include <Arduino.h>

#include "Network.hpp"


Network net;

void setup() {
  Serial.begin(115200);
  delay(3000);
  net.connect_network();
}

void loop() {
  net.connect_server(50);
  delay(10000);
}