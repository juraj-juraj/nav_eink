#pragma once

#include <Arduino.h>

#define DEBUG true

void debugPrint(const char* msg) {
    if(DEBUG)
        Serial.println(msg);
}
