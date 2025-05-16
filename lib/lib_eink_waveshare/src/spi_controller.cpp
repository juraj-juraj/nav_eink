#include "spi_controller.h"

namespace EinkSPI {

SPIController::SPIController(uint8_t cs, uint8_t dc):
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


void SPIController::sendCommand(uint8_t cmd) {
    digitalWrite(m_epd_dc, LOW); // Command mode
    digitalWrite(m_epd_cs, LOW); // CS low to enable device
    m_SPI_com.write(cmd);
    digitalWrite(m_epd_cs, HIGH);
}

void SPIController::sendData(uint8_t data) {
    digitalWrite(m_epd_dc, HIGH); // Data mode
    digitalWrite(m_epd_cs, LOW);
    m_SPI_com.write(data);
    digitalWrite(m_epd_cs, HIGH);
}

void SPIController::sendData(std::initializer_list<uint8_t> data) {
    digitalWrite(m_epd_dc, HIGH);
    digitalWrite(m_epd_cs, LOW);
    m_SPI_com.writeBytes(data.begin(), data.size());
    digitalWrite(m_epd_cs, HIGH);
}

void SPIController::sendData(const uint8_t *data, size_t size) {
    digitalWrite(m_epd_dc, HIGH);
    digitalWrite(m_epd_cs, LOW);
    m_SPI_com.writeBytes(data, size);
    digitalWrite(m_epd_cs, HIGH);
}

void SPIController::sendCommandWithData(uint8_t cmd, const std::initializer_list<uint8_t> data) {
    digitalWrite(m_epd_dc, LOW); // Command mode
    digitalWrite(m_epd_cs, LOW); // CS low to enable device
    m_SPI_com.write(cmd);
    digitalWrite(m_epd_dc, HIGH); // Data mode
    m_SPI_com.writeBytes(data.begin(), data.size());
    digitalWrite(m_epd_cs, HIGH); // CS high to disable device
}

} // namespace EinkSPI
