#pragma once 
#include <Arduino.h>
#include <type_traits>

#include <Adafruit_GFX.h>

#include "spi_controller.h"
#include "my_utils.h"
#include "eink_driver.h"

template <
    typename DriverType,
    typename = typename std::enable_if<
        std::is_base_of<DriverInterface, DriverType>::value
    >::type
>
class DisplayWrapper {
public:
    DisplayWrapper(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t busy) :
        m_spi(cs, dc), m_driver(rst, busy, m_spi) {
        m_driver.init();
        m_canvas = GFXCanvasBW(m_driver.get_width(), m_driver.get_height());
    }

    void clear_frame(EinkColor color = EinkColor::BLACK) {
        m_canvas.fillScreen(color);
        m_driver.clear_frame(color);
    }

    void display_frame() {
        m_driver.init();
        m_driver.set_frame_memory(m_canvas.getBuffer());
        m_driver.display_frame();
        m_driver.sleep();
    }

    void draw_pixel(int16_t x, int16_t y, uint16_t color) {
        m_canvas.drawPixel(x, y, color);
    }
    void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
        m_canvas.drawLine(x0, y0, x1, y1, color);
    }
    void draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        m_canvas.drawRect(x, y, w, h, color);
    }
    void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
        m_canvas.fillRect(x, y, w, h, color);
    }
    void draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
        m_canvas.drawCircle(x0, y0, r, color);
    }
    void fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
        m_canvas.fillCircle(x0, y0, r, color);
    }

    uint16_t get_canvas_width() const {
        return m_driver.get_width();
    }
    uint16_t get_canvas_height() const {
        return m_driver.get_height();
    }
private:
    SPIController m_spi;
    DriverType m_driver;
    GFXCanvasBW m_canvas;  
};


class GFXCanvasBW : public Adafruit_GFX {
public:
    GFXCanvasBW(uint16_t w, uint16_t h)
        : Adafruit_GFX(w, h) {
        buffer = new uint8_t[(w * h + 7) / 8];
        memset(buffer, 0, (w * h + 7) / 8);
    }

    ~GFXCanvasBW() {
        delete[] buffer;
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) return;
        int byteIndex = x + y * width();
        int bit = 7 - (byteIndex % 8);
        byteIndex /= 8;

        if (color)
            buffer[byteIndex] |= (1 << bit);
        else
            buffer[byteIndex] &= ~(1 << bit);
    }

    uint8_t *getBuffer() {
        return buffer;
    }

private:
    uint8_t *buffer;
};