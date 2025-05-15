#pragma once
#include <Arduino.h>
#include <SPI.h>


#define BLACK 0
#define WHITE 1

#define EINK_HEIGHT 200
#define EINK_WIDTH 200

void debugPrint(const char* msg) {
    if(DEBUG)
        Serial.println(msg);
}

class SPIController{
public:

    SPIController(const SPIController&) = delete;
    SPIController& operator=(const SPIController&) = delete;

    SPIController(SPIController&&) = default;
    SPIController& operator=(SPIController&&) = default;

    SPIController(uint8_t cs, uint8_t dc) :
    m_epd_cs(cs), m_epd_dc(dc), m_SPI_com(VSPI) {
        m_SPI_com.begin(SCK, MISO, MOSI, cs);
        m_SPI_com.setBitOrder(MSBFIRST);
        m_SPI_com.setDataMode(SPI_MODE0);
        m_SPI_com.setFrequency(1000000); // Set frequency to 1MHz
        m_SPI_com.setHwCs(false); // Use software CS control, we are controlling CS manually
        pinMode(m_epd_cs, OUTPUT);
        pinMode(m_epd_dc, OUTPUT);

        digitalWrite(m_epd_cs, HIGH);  //HIGH for disabling device
        digitalWrite(m_epd_dc, LOW);
    }

    void sendCommand(uint8_t cmd) {
        digitalWrite(m_epd_dc, LOW); // Command mode
        digitalWrite(m_epd_cs, LOW); // CS low to enable device
        m_SPI_com.write(cmd);
        digitalWrite(m_epd_cs, HIGH); // CS high to disable device
    }

    void sendData(uint8_t data) {
        digitalWrite(m_epd_dc, HIGH); // Data mode
        digitalWrite(m_epd_cs, LOW); // CS low to enable device
        m_SPI_com.write(data);
        digitalWrite(m_epd_cs, HIGH); // CS high to disable device
    }

    void sendData(std::initializer_list<uint8_t> data) {
        digitalWrite(m_epd_dc, HIGH); // Data mode
        digitalWrite(m_epd_cs, LOW); // CS low to enable device
        m_SPI_com.writeBytes(data.begin(), data.size());
        digitalWrite(m_epd_cs, HIGH); // CS high to disable device
    }

    void sendData(const uint8_t *data, size_t size) {
        digitalWrite(m_epd_dc, HIGH); // Data mode
        digitalWrite(m_epd_cs, LOW); // CS low to enable device
        m_SPI_com.writeBytes(data, size);
        digitalWrite(m_epd_cs, HIGH); // CS high to disable device
    }

    void sendCommandWithData(uint8_t cmd, const std::initializer_list<uint8_t> data) {
        digitalWrite(m_epd_dc, LOW); // Command mode
        digitalWrite(m_epd_cs, LOW); // CS low to enable device
        m_SPI_com.write(cmd);
        digitalWrite(m_epd_dc, HIGH); // Data mode
        m_SPI_com.writeBytes(data.begin(), data.size());
        digitalWrite(m_epd_cs, HIGH); // CS high to disable device
    }

private:
    SPIClass m_SPI_com;
    uint8_t m_epd_cs;
    uint8_t m_epd_dc;
};

class EinkPanel{
  public:
    EinkPanel(uint8_t rst, uint8_t busy, SPIController& spi_controller, int16_t width = EINK_WIDTH, int16_t height = EINK_HEIGHT) :
      m_width(width), m_height(height),
      m_SPI_controller(spi_controller), m_epd_rst(rst), m_epd_busy(busy) {
      pinMode(m_epd_rst, OUTPUT);
      pinMode(m_epd_busy, INPUT);

      digitalWrite(m_epd_rst, HIGH);
    }

    void init() {
        panel_reset();
        debugPrint("EinkPanel initialized\n");
        debugPrint("Busy pin state: ");
        debugPrint(digitalRead(m_epd_busy) == HIGH ? "BUSY\n" : "IDLE\n");

        m_SPI_controller.sendCommandWithData(0x01, {(EINK_HEIGHT - 1) & 0xFF, ((EINK_HEIGHT - 1) >> 8) & 0xFF, 0x00}); // DRIVER_OUTPUT_CONTROL
        m_SPI_controller.sendCommandWithData(0x0C, {0xD7, 0xD6, 0x9D}); // BOOSTER_SOFT_START_CONTROL
        m_SPI_controller.sendCommandWithData(0x2C, {0xA8}); // WRITE_VCOM_REGISTER
        m_SPI_controller.sendCommandWithData(0x3A, {0x1A}); // SET_DUMMY_LINE_PERIOD, 4 dummy lines per gate
        m_SPI_controller.sendCommandWithData(0x3B, {0x08}); // SET_GATE_TIME, to 2uS per line
        m_SPI_controller.sendCommandWithData(0x11, {0x03}); // DATA_ENTRY_MODE_SETTING, X increment, Y increment

        m_SPI_controller.sendCommandWithData(0x32, { // Lut for full refresh
            0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22,
            0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88,
            0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
            0x35, 0x51, 0x51, 0x19, 0x01, 0x00});
        debugPrint("Write LUT register done.\nInit done.\n");
    }

    void set_frame_memory(const uint8_t* image_buffer){
        if (image_buffer == nullptr) {
            debugPrint("Image buffer is null.\n");
            return;
        }
        
        set_window(0, 0, m_width-1, m_height-1);
        set_cursor(0, 0);
        m_SPI_controller.sendCommand(0x24); // WRITE_RAM command
        m_SPI_controller.sendData(image_buffer, m_width * m_height / 8); // Send image data
        debugPrint("Image data sent.\n");
    }

    void clear_frame(uint8_t color = 0xFF) {
        if(color == BLACK) {
            color = 0x00; // Set to black
        } else {
            color = 0xFF; // Set to white
        }
        set_window(0, 0, m_width-1, m_height-1);
        set_cursor(0, 0);
        m_SPI_controller.sendCommand(0x24); // WRITE_RAM command
        for (uint16_t i = 0; i < (m_width * m_height / 8); i++) {
            m_SPI_controller.sendData(color); // Send color data
        }
        debugPrint("Image data cleared.\n");
    }

    void display_frame() {
        m_SPI_controller.sendCommandWithData(0x22, {0xC4}); // DISPLAY_REFRESH command
        delay(10);
        m_SPI_controller.sendCommand(0x20); // Trigger display refresh
        m_SPI_controller.sendCommand(0xFF); // Wait for the display to be ready
        wait_until_idle();
        debugPrint("Display frame done.\n");
    }

    /**
     * Set the window for drawing.
     * @param x_start Start x coordinate
     * @param y_start Start y coordinate
     * @param x_end End x coordinate
     * @param y_end End y coordinate
     */
    void set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end) {
        m_SPI_controller.sendCommandWithData(0x44, {
            (uint8_t)((x_start >> 3) & 0xFF), 
            (uint8_t)((x_end >> 3) & 0xFF)}); // Set X address start and end position

        m_SPI_controller.sendCommandWithData(0x45, {
            (uint8_t)(y_start & 0xFF),
            (uint8_t)((y_start >> 8) & 0xFF),
            (uint8_t)(y_end & 0xFF),
            (uint8_t)((y_end >> 8) & 0xFF)}); // Set Y address start and end position

        debugPrint("Window set.\n");
    }

    void set_cursor(uint16_t x, uint16_t y) {
        m_SPI_controller.sendCommandWithData(0x4E, {(uint8_t)((x >> 3) & 0xFF)}); // Set RAM X address count to x
        m_SPI_controller.sendCommandWithData(0x4F, { (uint8_t)(y & 0xFF), (uint8_t)((y >> 8) & 0xFF)}); // Set RAM Y address count to y
        wait_until_idle();
        debugPrint("Cursor set.\n");
    }

    void panel_reset() {
      digitalWrite(m_epd_rst, HIGH);
      delay(100);
      digitalWrite(m_epd_rst, LOW);
      delay(100);
      digitalWrite(m_epd_rst, HIGH);
      delay(100);
    }

    void wait_until_idle() {
      while (digitalRead(m_epd_busy) == HIGH) {
        delay(1);
      }
    }

    void sleep(){
        debugPrint("Entering sleep mode...\n");
        m_SPI_controller.sendCommandWithData(0x10, {0x01}); // Enter deep sleep mode
    }

  private:
    uint16_t m_width;
    uint16_t m_height;
    uint8_t m_epd_rst;
    uint8_t m_epd_busy;
    SPIController& m_SPI_controller;
};

