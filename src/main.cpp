/**
 * @file main.cpp
 * @brief Demo showing capabilities of the display and the library.
 */

#define DEBUG true

#include <Arduino.h>

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>

#include <eink_waveshare.h>

#include "bitmap_memory.h"

#define EPD_CS 5 // Set low to enable the device
#define EPD_DC 16 // High for sending data, low for sending commands
#define EPD_RST 27 // Reset, active in low.
#define EPD_BUSY 25 // Busy pin, active high, output from display


auto handle = EinkDisplay::DisplayHandle<EinkDriver::Eink1in54>(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY, 0.7, 10);
uint8_t hour = 11;
uint8_t minute = 35;
uint8_t second = 20;

void setup(){
  Serial.begin(115200);
  
  handle.clear_frame(EinkColor::WHITE);
  delay(1000);

  handle.set_font(&FreeMono9pt7b);

  handle.draw_rect(20, 10, 160, 40, EinkColor::BLACK);
  handle.print(30, 35, EinkColor::BLACK, "TESTING #101");
  handle.draw_line(10, 60, 180, 60, EinkColor::BLACK);
  
  handle.draw_circle(40, 85, 20, EinkColor::BLACK);
  handle.draw_line(40, 85, 40, 70, EinkColor::BLACK);
  handle.draw_line(40, 85, 60, 85, EinkColor::BLACK);

  handle.draw_line(10, 110, 180, 110, EinkColor::BLACK);
  handle.draw_bitmap(10, 120, VUT_LOGO_FULL, 189, 74, EinkColor::BLACK, EinkColor::WHITE);
  handle.display_frame();
  handle.display_frame();

  handle.set_font(&FreeMonoBold12pt7b);
  
}

void loop(){

  handle.printf(70, 90, EinkColor::WHITE, "%02d:%02d:%02d", hour, minute, second);
  second++;  
  if(second == 60){
    second = 0;
    minute++;
  }
  if(minute == 60){
    minute = 0;
    hour++;
  }
  if(hour == 24){
    hour = 0;
  }
  if(hour == 24){
    hour = 0;
  }
  handle.printf(70, 90, EinkColor::BLACK, "%02d:%02d:%02d", hour, minute, second);
  handle.display_frame();
  delay(1000);
}
