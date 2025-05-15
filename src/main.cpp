#include <Arduino.h>

#define DEBUG true


#include <eink_driver.h>
#include <spi_controller.h>
#include <display_wrapper.h>
#include <my_utils.h>

#define EPD_CS 5 // Set low to enable the device
#define EPD_DC 16 // High for sending data, low for sending commands
#define EPD_RST 27 // Reset, active in low. 10 ms pulse to reset
#define EPD_BUSY 25 // Busy pin, active high, output from display



void setup(){
  Serial.begin(115200);
  Serial.println("Hello World!");
  
  auto panel = DisplayWrapper<Eink1in54Driver>(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);
  panel.clear_frame(EinkColor::WHITE);
  delay(1000);

  panel.fill_rect(80, 80, 50, 50, EinkColor::BLACK);
  panel.display_frame();

  // auto m_spi = SPIController(EPD_CS, EPD_DC);
  // Eink1in54Driver panel = Eink1in54Driver(EPD_RST, EPD_BUSY, m_spi);
  // panel.init();
  // panel.clear_frame(EinkColor::WHITE);
  // panel.display_frame();

  // panel.clear_frame();
  // panel.display_frame();

  // panel.sleep();

}

void loop(){

}
