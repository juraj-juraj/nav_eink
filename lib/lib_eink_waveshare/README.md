# E-Ink library for Waveshare displays

This library provides high level interface for using of waveshare eink display. Now only 1.54 inch display is suported, but it's interface allows for easy integration of other displays also. 

Graphic capabilities are accelerated using adafruit GFX library, which provides most graphic methods. Communication with display is heavily inspired from official library from vendor Waveshare.

## Overview

Aim of this library is to provide high level, easy  to use interface for e-ink displays. What differs this library from others are few main features:

* Automatic full and partial refresh of display, no need to worry about it.
* Easy to implement new display drivers, they only need to implement methods from `EinkDriver::Interface`.
* Modern usage of C++ features, this library is written in C++17 standard.
* High level interface for drawing, based on adafruit GFX library.

**Automatic refresh**: This library automatically decides when is the right time for only partial refreshes, optimizing the display update process. It computes minimal bounding box of currently drawn shapes and detects if that area is smaller than set threshold. Like if the new drawings are going to lay on more than 70% of the display, it will do a full refresh. The threshold can be set in the constructor of the `EinkDisplay::DisplayHandle` class. Also E-ink displays need to do a full refresh every once a while. This will be done automatically, the display will do full refresh every few times it is updated. The number of updates can be set in the constructor of the `EinkDisplay::DisplayHandle` class.

**Easy to implement new display drivers**: The library is designed to be easy to extend. If you want to add support for a new display, you only need to implement the `EinkDriver::Interface` interface. This interface provides methods for initializing the display, sending commands, and writing data to the display. The library will take care of the rest. Then you can just specify the driver as template parameter of the `EinkDisplay::DisplayHandle` class.

**Modern usage of C++ features**: The library is written in modern C++17 standard. It uses templates, sfinae, and other modern C++ features to provide a clean and easy to use interface.

**High level interface for drawing**: The library provides a high level interface for drawing shapes on the display. Internally it is based on the adafruit GFX library, which provides a wide range of drawing functions. The `EinkDisplay::DisplayHandle` class provides wrapper methods for the adafruit GFX library functions. Adafruit GFX methods are wrapped so that the handle can compute minimal  bounding box and other optimizations to determine right time for full or partial refresh.

## Usage

### Installation

This library is designed to be used with PlatformIO extension in Visual Studio Code. Destination framework is Arduino.
To install library in your project, copy its content to `lib/lib_eink_waveshare` folder in your project. Then you can include the library in your code with:

```cpp
#include <EinkDisplay.h>
```

### Example

This simple program shows how to use the library. It initializes the display, draws some shapes, and updates the display.

```cpp
#include <EinkDisplay.h>
#include <Fonts/FreeMono9pt7b.h>

// Define connections for the display
// These are valid for wemos D1 r32
// 3.3V -> 3V3
// GND -> GND
#define PIN_CS 5
#define PIN_DC 16
#define PIN_RST 27
#define PIN_BUSY 25
#define PIN_DIN 23
#define PIN_CLK 18

#define REFRESH_THRESHOLD 0.7 // Update when 70% of display was to be updated
#define REFRESH_PERIOD 10 // Full refresh every 10 updates

// Create display handle
auto display_handle = EinkDisplay::DisplayHandle<EinkDriver::Eink1in54>(PIN_CS, PIN_DC, PIN_RST, PIN_BUSY, REFRESH_THRESHOLD, REFRESH_PERIOD);

void setup() {
    display_handle.clear_frame(EinkColor::WHITE); //Physically clear the display to white
    handle.set_font(&FreeMono9pt7b); // Set font for text

    handle.draw_rect(20, 10, 160, 40, EinkColor::BLACK); // Draw rectangle to buffer
    handle.printf(30, 35, EinkColor::BLACK, "TESTING #%d", 101); // Draw text to buffer like printf
    handle.draw_bitmap(10, 120, VUT_LOGO_FULL, 189, 74, EinkColor::BLACK, EinkColor::WHITE); // Draw bitmap to buffer
    handle.display_frame(); // Update display with content of buffer
}

void loop() {
    // Do nothing
}
```





