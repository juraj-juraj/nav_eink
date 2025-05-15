#pragma once

#include <Arduino.h>
#include <SPI.h>

/**
 * @class SPIController
 * @brief Controls SPI communication with an e-paper display
 *
 * This class manages the SPI interface for communicating with an e-paper display
 * by handling command and data transmission. It provides methods to send
 * commands and data to the display while managing the required control signals.
 *
 * @note This class is move-only and cannot be copied.
 */
class SPIController{
public:
    /**
     * @brief Copy constructor (deleted)
     * @note Copying is not allowed for this class
     */
    SPIController(const SPIController&) = delete;
    
    /**
     * @brief Copy assignment operator (deleted)
     * @note Copying is not allowed for this class
     */
    SPIController& operator=(const SPIController&) = delete;

    /**
     * @brief Move constructor
     */
    SPIController(SPIController&&) = default;
    
    /**
     * @brief Move assignment operator
     */
    SPIController& operator=(SPIController&&) = default;

    /**
     * @brief Constructs an SPI controller with specified pins
     * @param cs Chip select pin number
     * @param dc Data/Command control pin number
     */
    SPIController(uint8_t cs, uint8_t dc);

    /**
     * @brief Sends a command to the e-paper display
     * @param cmd Command byte to send
     */
    void sendCommand(uint8_t cmd);
    
    /**
     * @brief Sends a single data byte to the e-paper display
     * @param data Data byte to send
     */
    void sendData(uint8_t data);
    
    /**
     * @brief Sends multiple data bytes to the e-paper display
     * @param data Initializer list containing data bytes to send
     */
    void sendData(std::initializer_list<uint8_t> data);
    
    /**
     * @brief Sends an array of data bytes to the e-paper display
     * @param data Pointer to the array of data bytes
     * @param size Number of bytes to send
     */
    void sendData(const uint8_t *data, size_t size);
    
    /**
     * @brief Sends a command followed by data to the e-paper display
     * @param cmd Command byte to send
     * @param data Initializer list containing data bytes to send
     */
    void sendCommandWithData(uint8_t cmd, const std::initializer_list<uint8_t> data);

private:
    SPIClass m_SPI_com;    ///< SPI communication interface
    uint8_t m_epd_cs;      ///< Chip select pin
    uint8_t m_epd_dc;      ///< Data/Command control pin
};