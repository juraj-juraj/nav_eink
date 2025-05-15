/**
 * @file main.cpp
 * @brief Demo showing capabilities of the display and the library.
 */

#define DEBUG true

#include <Arduino.h>

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>

#include <eink_driver.h>
#include <spi_controller.h>
#include <display_wrapper.h>
#include <my_utils.h>

#include "bitmap_memory.h"

#define EPD_CS 5 // Set low to enable the device
#define EPD_DC 16 // High for sending data, low for sending commands
#define EPD_RST 27 // Reset, active in low. 10 ms pulse to reset
#define EPD_BUSY 25 // Busy pin, active high, output from display


auto panel = DisplayWrapper<Eink1in54Driver>(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY, 0.7, 10);
uint8_t hour = 11;
uint8_t minute = 35;
uint8_t second = 20;

void setup(){
  Serial.begin(115200);
  
  panel.clear_frame(EinkColor::WHITE);
  delay(1000);

  panel.set_font(&FreeMono9pt7b);


  panel.clear_frame(EinkColor::WHITE);
  panel.draw_rect(20, 10, 160, 40, EinkColor::BLACK);
  panel.print_text(30, 35, EinkColor::BLACK, "TESTING #101");
  panel.draw_line(10, 60, 180, 60, EinkColor::BLACK);
  
  panel.draw_circle(40, 85, 20, EinkColor::BLACK);
  panel.draw_line(40, 85, 40, 70, EinkColor::BLACK);
  panel.draw_line(40, 85, 60, 85, EinkColor::BLACK);

  panel.draw_line(10, 110, 180, 110, EinkColor::BLACK);
  panel.draw_bitmap(10, 120, VUT_LOGO_FULL, 189, 74, EinkColor::BLACK, EinkColor::WHITE);
  panel.display_frame();
  panel.display_frame();

  panel.set_font(&FreeMonoBold12pt7b);
  
}

void loop(){

  panel.printf_text(70, 90, EinkColor::WHITE, "%02d:%02d:%02d", hour, minute, second);
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
  panel.printf_text(70, 90, EinkColor::BLACK, "%02d:%02d:%02d", hour, minute, second);
  panel.display_frame();
  delay(1000);
}
