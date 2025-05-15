#define DEBUG true

#include <Arduino.h>

#include <Fonts/FreeMono9pt7b.h>
#
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
  
  auto panel = DisplayWrapper<Eink1in54Driver>(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY, 0.7, 2);
  panel.clear_frame(EinkColor::WHITE);
  delay(1000);

  panel.set_font(&FreeMono9pt7b);

  panel.fill_rect(80, 80, 50, 50, EinkColor::BLACK);
  panel.print_text(10, 10, "Hello World!", EinkColor::BLACK);
  panel.display_frame();
  Serial.println("Stage 1");
  delay(2000);
  panel.print_text(10, 10, "Hello World!", EinkColor::WHITE);
  panel.print_text(10, 10, "dlrow olleH!", EinkColor::BLACK);
  panel.display_frame();
  delay(2000);
  panel.print_text(10, 10, "dlrow olleH!", EinkColor::WHITE);
  panel.print_text(10, 10, "Hello World!", EinkColor::BLACK);
  panel.display_frame();
  
  delay(2000);
  panel.print_text(10, 10, "Hello World!", EinkColor::WHITE);
  panel.print_text(10, 10, "dlrow olleH!", EinkColor::BLACK);
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
  // put your main code here, to run repeatedly:
  delay(1000);
}
