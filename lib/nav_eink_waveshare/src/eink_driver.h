#pragma once

#include <Arduino.h>

#include "my_utils.h"
#include "spi_controller.h"

class EinkColor {
public:
    static const EinkColor BLACK;
    static const EinkColor WHITE;

    uint8_t value() const { return m_value; }

    bool operator==(const EinkColor& other) const { return m_value == other.m_value; }

private:
    explicit constexpr EinkColor(uint8_t val) : m_value(val) {}
    uint8_t m_value;
};

constexpr EinkColor EinkColor::BLACK(0);
constexpr EinkColor EinkColor::WHITE(1);

class DriverInterface{
public:
    virtual ~DriverInterface() = default;

    virtual void init(bool partial_update) = 0;
    virtual void set_frame_memory(const uint8_t* image_buffer) = 0;
    virtual void clear_frame(EinkColor color) = 0;
    virtual void display_frame() = 0;
    virtual void sleep() = 0;
    virtual uint16_t get_width() const = 0;
    virtual uint16_t get_height() const = 0;

};

class Eink1in54Driver: public DriverInterface{
public:
    Eink1in54Driver(uint8_t rst, uint8_t busy, SPIController& spi_controller);

    void init(bool partial_update = false);

    void set_frame_memory(const uint8_t* image_buffer);

    void clear_frame(EinkColor color);

    void display_frame();

    void sleep();

    uint16_t get_width() const {
        return M_WIDTH;
    }
    uint16_t get_height() const {
        return M_HEIGHT;
    }

    static constexpr uint16_t M_WIDTH = 200;
    static constexpr uint16_t M_HEIGHT = 200;

private:
    /**
     * Set the window for drawing.
     * @param x_start Start x coordinate
     * @param y_start Start y coordinate
     * @param x_end End x coordinate
     * @param y_end End y coordinate
     */
    void set_window(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);

    void set_cursor(uint16_t x, uint16_t y);

    void panel_reset();

    void wait_until_idle();


    uint8_t m_epd_rst;
    uint8_t m_epd_busy;
    SPIController& m_SPI_controller;
};
