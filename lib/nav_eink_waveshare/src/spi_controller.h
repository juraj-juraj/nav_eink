#pragma once

#include <Arduino.h>
#include <SPI.h>

class SPIController{
public:

    SPIController(const SPIController&) = delete;
    SPIController& operator=(const SPIController&) = delete;

    SPIController(SPIController&&) = default;
    SPIController& operator=(SPIController&&) = default;

    SPIController(uint8_t cs, uint8_t dc);

    void sendCommand(uint8_t cmd);
    void sendData(uint8_t data);
    void sendData(std::initializer_list<uint8_t> data);
    void sendData(const uint8_t *data, size_t size);
    void sendCommandWithData(uint8_t cmd, const std::initializer_list<uint8_t> data);

private:
    SPIClass m_SPI_com;
    uint8_t m_epd_cs;
    uint8_t m_epd_dc;
};