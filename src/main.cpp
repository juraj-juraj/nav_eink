#include <Arduino.h>

#define ENABLE_GxEPD2_GFX 0

#include <Adafruit_GFX.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <SPI.h>


#define EPD_CS 5 // Set low to enable the device
#define EPD_DC 16 // High for sending data, low for sending commands
#define EPD_RST 27 // Reset, active in low. 10 ms pulse to reset
#define EPD_BUSY 4 // Busy pin, active high, output from display

SPIClass SPI_EPD(VSPI);

void sendCommand(uint8_t cmd) {
  digitalWrite(EPD_DC, LOW);
  digitalWrite(EPD_CS, LOW);
  SPI_EPD.transfer(cmd);
  digitalWrite(EPD_CS, HIGH);
}

void sendData(uint8_t data) {
  digitalWrite(EPD_DC, HIGH);
  digitalWrite(EPD_CS, HIGH);
  SPI_EPD.transfer(data);
  digitalWrite(EPD_CS, LOW);
  digitalWrite(EPD_CS, HIGH);
}

void sendCommandWithData(uint8_t cmd, std::initializer_list<uint8_t> data) {
  digitalWrite(EPD_DC, LOW);
  digitalWrite(EPD_CS, LOW);
  SPI_EPD.transfer(cmd);
  digitalWrite(EPD_DC, HIGH);
  for(auto B: data) {
    SPI_EPD.transfer(B);
  }
  digitalWrite(EPD_CS, LOW);
  digitalWrite(EPD_CS, HIGH);
}

void waitUntilIdle() {
  // BUSY = HIGH --> busy, BUSY = LOW --> idle
  while (digitalRead(EPD_BUSY) == HIGH) {
    delay(1);
  }
}

void panelReset() {
  digitalWrite(EPD_RST, LOW);
  delay(10);
  digitalWrite(EPD_RST, HIGH);
  delay(10);
}

void setup(){
  Serial.begin(115200);
  Serial.println("Hello World!");
  pinMode(EPD_CS, OUTPUT);
  pinMode(EPD_DC, OUTPUT);
  pinMode(EPD_RST, OUTPUT);
  pinMode(EPD_BUSY, INPUT_PULLUP);
  digitalWrite(EPD_CS, LOW);
  digitalWrite(EPD_DC, LOW);

  digitalWrite(EPD_RST, HIGH);
  delay(1000);
  digitalWrite(EPD_RST, LOW);
  delay(10);
  digitalWrite(EPD_RST, HIGH);
  delay(1000);
  Serial.print("Busy pin state: ");
  Serial.println(digitalRead(EPD_BUSY));
  Serial.print("Writing commands to EPD...");


  SPI_EPD.begin(SCK, MISO, MOSI, EPD_CS);
  SPI_EPD.setFrequency(2000000); // 2 MHz is safe for most e-papers
  SPI_EPD.setDataMode(SPI_MODE0);
  SPI_EPD.setBitOrder(MSBFIRST);

  panelReset();
  Serial.print("Busy pin state: ");
  Serial.println(digitalRead(EPD_BUSY));
  sendCommand(0x12);           // SOFTWARE_RESET
  waitUntilIdle();
  Serial.println("Software reset done.");

  sendCommandWithData(0x01, {0xC7, 0x00, 0x00}); // DRIVER_OUTPUT_CONTROL

  Serial.println("Driver output control done.");

  // Data entry mode
  sendCommandWithData(0x11, {0x01}); // DATA_ENTRY_MODE_SETTING, X increment, Y decrement

  // Set RAM X start/end (0 to 24 columns)
  sendCommandWithData(0x44, {0x00, 0x18}); // SET_RAM_X_ADDRESS_START_END
  Serial.println("Set RAM X start/end done.");

  // Set RAM Y start/end (here 199 down to 0 → 200 pixels)
  sendCommand(0x45);
  sendData(0xC7);  // Y start LSB
  sendData(0x00);  // Y start MSB
  sendData(0x00);  // Y end   LSB
  sendData(0x00);  // Y end   MSB
  Serial.println("Set RAM Y start/end done.");

  sendCommand(0x20);  // activate display update sequence
  delay(100);
  waitUntilIdle();

  // Set RAM counters to (0,0)
  sendCommand(0x4E); sendData(0x00);               // SET_RAM_X_ADDRESS_COUNTER
  sendCommand(0x4F);
  sendData(0xC7);  // Y = 199 LSB
  sendData(0x00);  // Y = 199 MSB
  delay(100);
  // waitUntilIdle();
  Serial.println("Set RAM counters done.");

  // === Write Image Buffers ===
  // 1) Black/White
  Serial.println("Writing black/white data...");
  sendCommand(0x24);
  for (int i = 0; i < 4000; i++) {
    sendData(0xFF);  // replace with your actual BW data
  }
  for(int i = 0; i < 1000; i++) {
    sendData(0x00);  // replace with your actual BW data
  }

  // === Trigger Display Update ===
  // display update sequence
  // enable clock signal, enable analog, display with display mode 1, disable analog, disable oscilator
  sendCommandWithData(0x22, {0xC7}); 
  sendCommand(0x20);        // Activate display update sequence
  waitUntilIdle();

  // Enter deep sleep
  sendCommand(0x10);
  sendData(0x01);

}

void loop(){

}


// alternately you can copy the constructor from GxEPD2_display_selection.h or GxEPD2_display_selection_added.h to here
// GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(/*CS=D8*/ EPD_CS, /*DC=D3*/ EPD_DC, /*RST=D4*/ EPD_RST, /*BUSY=D2*/ EPD_BUSY)); // GDEH0154D67






// const char HelloWorld[] = "Hello World!";
// const char HelloWeACtStudio[] = "Yaser Ali Husen";

// void helloWorld()
// {
//   display.setRotation(0);
//   display.setFont(&FreeMonoBold18pt7b);
//   display.setTextColor(GxEPD_BLACK);
//   int16_t tbx, tby;
//   uint16_t tbw, tbh;
//   display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
//   // center the bounding box by transposition of the origin:
//   uint16_t x = ((display.width() - tbw) / 2) - tbx;
//   uint16_t y = ((display.height() - tbh) / 2) - tby;
//   display.setFullWindow();
//   display.firstPage();
//   do
//   {
//     display.fillScreen(GxEPD_WHITE);
//     display.setCursor(x, y-tbh);
//     display.print(HelloWorld);
//     display.setTextColor(display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
//     display.getTextBounds(HelloWeACtStudio, 0, 0, &tbx, &tby, &tbw, &tbh);
//     x = ((display.width() - tbw) / 2) - tbx;
//     display.setCursor(x, y+tbh);
//     display.print(HelloWeACtStudio);
//   }
//   while (display.nextPage());                   
// }


// void setup()
// {
//   display.init(115200,true,10,false);
//   helloWorld();
//   delay(1000);
//   display.hibernate();
// }


// void loop() {

// }