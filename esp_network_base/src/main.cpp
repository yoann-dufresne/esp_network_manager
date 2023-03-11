#include <Arduino.h>

#include "Network.hpp"


Network net;

void setup() {
  Serial.begin(115200);
  delay(3000);
  net.connect_network();
}

void loop() {
  // put your main code here, to run repeatedly:
}