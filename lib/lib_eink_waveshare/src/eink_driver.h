#pragma once

#include <Arduino.h>

#include "my_utils.h"
#include "spi_controller.h"


/**
 * @class EinkColor
 * @brief Represents a color for e-ink displays.
 * 
 * This class encapsulates color values for e-ink displays, providing
 * predefined constants for colors (BLACK and WHITE).
 * The color is represented internally as an 8-bit value.
 * White is represented as 1 and black as 0.
 * 
 */
class EinkColor {
public:
    static const EinkColor BLACK;
    static const EinkColor WHITE;

    /**
     * @brief Get the color value.
     * 
     * @return The 8-bit color value. Either 0 (BLACK) or 1 (WHITE).
     */
    uint8_t value() const { return m_value; }

    bool operator==(const EinkColor& other) const { return m_value == other.m_value; }

private:
    explicit constexpr EinkColor(uint8_t val) : m_value(val) {}
    uint8_t m_value;
};

constexpr EinkColor EinkColor::BLACK(0);
constexpr EinkColor EinkColor::WHITE(1);


/**
 * @class DriverInterface
 * @brief Interface for E-ink display drivers.
 * 
 * This abstract class defines the interface that all E-ink display driver implementations
 * must conform to. It provides methods for initializing the display, setting display content,
 * clearing the display, and controlling power states.
 */
class DriverInterface{
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup in derived classes.
     */
    virtual ~DriverInterface() = default;

    /**
     * @brief Initialize the display.
     * @param partial_update If true, enables partial update mode (if supported by hardware).
     */
    virtual void init(bool partial_update) = 0;
    
    /**
     * @brief Set the display frame buffer with provided image data.
     * @param image_buffer Pointer to buffer containing the image data to display.
     */
    virtual void set_frame_memory(const uint8_t* image_buffer) = 0;

    /**
     * @brief Set the display frame buffer with provided image data and coordinates.
     * Partially updates the display memory with the provided image data.
     * @param image_buffer Pointer to buffer containing the image data to display.
     * @param x_start Start x coordinate for the image.
     * @param y_start Start y coordinate for the image.
     * @param x_end End x coordinate for the image.
     * @param y_end End y coordinate for the image.
     */
    virtual void set_frame_memory(const uint8_t* image_buffer, uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end) = 0;
    
    /**
     * @brief Clear the entire display to a specific color.
     * @param color The color to fill the display with.
     */
    virtual void clear_frame(EinkColor color) = 0;
    
    /**
     * @brief Update the physical display with current frame buffer contents.
     */
    virtual void display_frame() = 0;
    
    /**
     * @brief Put the display into sleep mode to save power.
     */
    virtual void sleep() = 0;
    
    /**
     * @brief Get the display width in pixels.
     * @return The width of the display.
     */
    virtual uint16_t get_width() const = 0;
    
    /**
     * @brief Get the display height in pixels.
     * @return The height of the display.
     */
    virtual uint16_t get_height() const = 0;
};

/**
 * @class Eink1in54Driver
 * @brief E-ink display driver for the 1.54 inch e-ink display.
 * 
 * This class implements the DriverInterface for the 1.54 inch e-ink display.
 * It provides methods to initialize the display, set the frame memory, clear the display,
 * and control the display's power state.
 */
class Eink1in54Driver: public DriverInterface{
public:
    /**
     * @brief Constructor for the Eink1in54Driver class.
     * This only initializes the pins and the class. SPI controller has to be already initialized.
     * @param rst Pin number for the reset pin.
     * @param busy Pin number for the busy pin.
     * @param spi_controller Reference to the SPI controller object.
     */
    Eink1in54Driver(uint8_t rst, uint8_t busy, SPIController& spi_controller);

    /**
     * @brief Initialize the display.
     * This function initializes the display and sets it to a known state.
     * @param partial_update If true, enables partial update mode.
     */
    void init(bool partial_update = false);

    /**
     * @brief Set the frame memory with the provided image buffer.
     * The image buffer should be in the correct size for display, 5000 bytes long array
     * (200x200 pixels, 1 bit per pixel).
     * @param image_buffer Pointer to the image buffer to be displayed.
     */
    void set_frame_memory(const uint8_t* image_buffer);

    /**
     * @brief Set the frame memory with the provided image buffer and coordinates.
     * This function partially updates the display memory with the provided image data.
     * @param image_buffer Pointer to the image buffer to be displayed.
     * @param x_start Start x coordinate for the image.
     * @param y_start Start y coordinate for the image.
     * @param x_end End x coordinate for the image.
     * @param y_end End y coordinate for the image.
     * @note The image buffer should be in the correct size for display, 5000 bytes long array
     * (200x200 pixels, 1 bit per pixel).
     */
    void set_frame_memory(const uint8_t* image_buffer, uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);

    /**
     * @brief Clear the display frame with a specific color.
     * This function fills the entire display with the specified color.
     * @param color The color to fill the display with (BLACK or WHITE).
     */
    void clear_frame(EinkColor color);

    /**
     * @brief Clear the display frame.
     * This function fills the entire display with white color.
     * Display has to be already initialized and not in sleep mode.
     */
    void display_frame();

    /**
     * @brief Put the display into sleep mode.
     * This function puts the display into a low-power state to save energy.
     */
    void sleep();

    /**
     * @brief Get the width of the display.
     * @return The width of the display in pixels.
     */
    uint16_t get_width() const {
        return M_WIDTH;
    }

    /**
     * @brief Get the height of the display.
     * @return The height of the display in pixels.
     */
    uint16_t get_height() const {
        return M_HEIGHT;
    }

    static constexpr uint16_t M_WIDTH = 200;
    static constexpr uint16_t M_HEIGHT = 200;

private:
    /**
     * @brief Set the window for drawing.
     * @param x_start Start x coordinate
     * @param y_start Start y coordinate
     * @param x_end End x coordinate
     * @param y_end End y coordinate
     */
    void set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);

    /**
     * @brief Set the cursor position.
     * @param x X coordinate
     * @param y Y coordinate
     * @note This function is used to set the cursor position for drawing.
     */
    void set_cursor(uint16_t x, uint16_t y);

    /**
     * @brief Reset the display panel using reset pin only.
     */
    void panel_reset();

    /**
     * @brief Blocking wait until the display is idle.
     */
    void wait_until_idle();

    uint8_t m_epd_rst;
    uint8_t m_epd_busy;
    SPIController& m_SPI_controller;
};
