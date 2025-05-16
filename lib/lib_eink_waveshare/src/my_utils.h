#pragma once

#include <Arduino.h>

#define DEBUG true


namespace debug{

/**
 * @brief Prints a message to the serial console if DEBUG macro is defined to true.
 * @note Serial communication must be initialized before calling this function.
 */
void Print(const char* msg);


}
