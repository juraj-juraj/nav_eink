#include <Arduino.h>

// #define ENABLE_GxEPD2_GFX 0

// #include <Adafruit_GFX.h>
// #include <GxEPD2_BW.h>
// #include <GxEPD2_3C.h>
// #include <Fonts/FreeMonoBold18pt7b.h>
// #include <SPI.h>
#define DEBUG true

#include "eink_lib.h"

#define EPD_CS 5 // Set low to enable the device
#define EPD_DC 16 // High for sending data, low for sending commands
#define EPD_RST 27 // Reset, active in low. 10 ms pulse to reset
#define EPD_BUSY 25 // Busy pin, active high, output from display



void setup(){
  Serial.begin(115200);
  Serial.println("Hello World!");
  
  auto m_spi = SPIController(EPD_CS, EPD_DC);
  auto panel = EinkPanel(EPD_RST, EPD_BUSY, m_spi);
  panel.init();
  panel.clear_frame(WHITE);
  panel.display_frame();

  panel.clear_frame(WHITE);
  panel.display_frame();

  panel.sleep();

}

void loop(){

}
