#include "my_utils.h"

void debug::Print(const char* msg) {
    if(DEBUG)
        Serial.println(msg);
}