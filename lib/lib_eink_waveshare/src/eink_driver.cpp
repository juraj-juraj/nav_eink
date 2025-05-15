#include "eink_driver.h"

Eink1in54Driver::Eink1in54Driver(uint8_t rst, uint8_t busy, SPIController& spi_controller) :
    m_SPI_controller(spi_controller), m_epd_rst(rst), m_epd_busy(busy) {
    pinMode(m_epd_rst, OUTPUT);
    pinMode(m_epd_busy, INPUT);

    digitalWrite(m_epd_rst, HIGH);
}

void Eink1in54Driver::init(bool partial_update) {
    panel_reset();
    debug::Print("Eink1in54Driver initialized\n");
    debug::Print("Busy pin state: ");
    debug::Print(digitalRead(m_epd_busy) == HIGH ? "BUSY\n" : "IDLE\n");

    m_SPI_controller.sendCommandWithData(0x01, {(M_HEIGHT - 1) & 0xFF, ((M_HEIGHT - 1) >> 8) & 0xFF, 0x00}); // DRIVER_OUTPUT_CONTROL
    m_SPI_controller.sendCommandWithData(0x0C, {0xD7, 0xD6, 0x9D}); // BOOSTER_SOFT_START_CONTROL
    m_SPI_controller.sendCommandWithData(0x2C, {0xA8}); // WRITE_VCOM_REGISTER
    m_SPI_controller.sendCommandWithData(0x3A, {0x1A}); // SET_DUMMY_LINE_PERIOD, 4 dummy lines per gate
    m_SPI_controller.sendCommandWithData(0x3B, {0x08}); // SET_GATE_TIME, to 2uS per line
    m_SPI_controller.sendCommandWithData(0x11, {0x03}); // DATA_ENTRY_MODE_SETTING, X increment, Y increment

    if(partial_update){
        m_SPI_controller.sendCommandWithData(0x32, { // Lut for partial refresh
            0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12, 
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        });
    }
    else {
        m_SPI_controller.sendCommandWithData(0x32, { // Lut for full refresh
            0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22,
            0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88,
            0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
            0x35, 0x51, 0x51, 0x19, 0x01, 0x00});
    }
}

void Eink1in54Driver::set_frame_memory(const uint8_t* image_buffer){
    if (image_buffer == nullptr) {
        debug::Print("Image buffer is null.\n");
        return;
    }
    
    set_window(0, 0, M_WIDTH-1, M_HEIGHT-1);
    set_cursor(0, 0);
    m_SPI_controller.sendCommand(0x24); // WRITE_RAM command
    m_SPI_controller.sendData(image_buffer, M_WIDTH * M_HEIGHT / 8); // Send image data
    debug::Print("Image data sent.\n");
}

/**
 * @brief Rounds down to the nearest multiple of 8.
 * @param n The number to round down.
 * @return The rounded number.
 */
uint16_t floorToMultipleOf8(uint16_t n) {
    return n & ~0x07; // OR use (n / 8) * 8;
}

/**
 * @brief Rounds up to the nearest multiple of 8.
 * @param n The number to round up.
 * @return The rounded number.
 */
uint16_t ceilToMultipleOf8(uint16_t n) {
    return (n + 7) & ~0x07;
}

void Eink1in54Driver::set_frame_memory(const uint8_t* image_buffer, uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end){
    if (image_buffer == nullptr) {
        debug::Print("Image buffer is null.\n");
        return;
    }

    if (x_start < 0 || x_end < 0 || y_start < 0 || y_end < 0) {
        debug::Print("Coordinates out of bounds.\n");
        return;
    }

    x_start = floorToMultipleOf8(x_start);
    x_end = ceilToMultipleOf8(x_end);

    uint16_t width = x_end - x_start;
    uint16_t height = y_end - y_start + 1;

    if (width > M_WIDTH || height > M_HEIGHT) {
        debug::Print("Image dimensions exceed display size.\n");
        return;
    }

    set_window(x_start, y_start, x_end, y_end);
    set_cursor(x_start, y_start);
    m_SPI_controller.sendCommand(0x24); // WRITE_RAM command
    uint16_t line_size = width / 8;
    uint16_t line_offset = y_start * (M_WIDTH / 8) + (x_start / 8);

    for(uint16_t i = 0; i < height; i++) {
        m_SPI_controller.sendData(image_buffer + line_offset + i * (M_WIDTH / 8), line_size); // Send image data
    }
    debug::Print("Partial image data sent.\n");
}


void Eink1in54Driver::clear_frame(EinkColor color) {
    uint8_t val = 0xFF; // Default to white
    if(color == EinkColor::BLACK) {
        uint8_t val = 0x00; // Set to black
    }
    set_window(0, 0, M_WIDTH-1, M_HEIGHT-1);
    set_cursor(0, 0);
    m_SPI_controller.sendCommand(0x24); // WRITE_RAM command
    for (uint16_t i = 0; i < (M_WIDTH * M_HEIGHT / 8); i++) {
        m_SPI_controller.sendData(val); // Send color data
    }
    debug::Print("Image data cleared.\n");
}

void Eink1in54Driver::display_frame() {
    m_SPI_controller.sendCommandWithData(0x22, {0xC4}); // DISPLAY_REFRESH command
    delay(10);
    m_SPI_controller.sendCommand(0x20); // Trigger display refresh
    m_SPI_controller.sendCommand(0xFF); // Wait for the display to be ready
    wait_until_idle();
}

void Eink1in54Driver::set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end) {
    m_SPI_controller.sendCommandWithData(0x44, {
        (uint8_t)((x_start >> 3) & 0xFF), 
        (uint8_t)((x_end >> 3) & 0xFF)}); // Set X address start and end position

    m_SPI_controller.sendCommandWithData(0x45, {
        (uint8_t)(y_start & 0xFF),
        (uint8_t)((y_start >> 8) & 0xFF),
        (uint8_t)(y_end & 0xFF),
        (uint8_t)((y_end >> 8) & 0xFF)}); // Set Y address start and end position
}


void Eink1in54Driver::set_cursor(uint16_t x, uint16_t y) {
    m_SPI_controller.sendCommandWithData(0x4E, {(uint8_t)((x >> 3) & 0xFF)}); // Set RAM X address count to x
    m_SPI_controller.sendCommandWithData(0x4F, { (uint8_t)(y & 0xFF), (uint8_t)((y >> 8) & 0xFF)}); // Set RAM Y address count to y
    wait_until_idle();
}


void Eink1in54Driver::panel_reset() {
    digitalWrite(m_epd_rst, HIGH);
    delay(100);
    digitalWrite(m_epd_rst, LOW);
    delay(100);
    digitalWrite(m_epd_rst, HIGH);
    delay(100);
}



void Eink1in54Driver::wait_until_idle() {
    while (digitalRead(m_epd_busy) == HIGH) {
    delay(1);
    }
}


void Eink1in54Driver::sleep(){
    debug::Print("Entering sleep mode...\n");
    m_SPI_controller.sendCommandWithData(0x10, {0x01}); // Enter deep sleep mode
}

