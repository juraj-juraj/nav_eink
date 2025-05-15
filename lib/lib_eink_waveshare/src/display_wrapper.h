#pragma once

#include <Arduino.h>
#include <type_traits>

#include <Adafruit_GFX.h>

#include "spi_controller.h"
#include "my_utils.h"
#include "eink_driver.h"

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

template <
    typename DriverType,
    typename = typename std::enable_if<
        std::is_base_of<DriverInterface, DriverType>::value
    >::type
>
class DisplayWrapper {
public:
    DisplayWrapper(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t busy, float refresh_threshold = 0.7, uint8_t refresh_number = 4) :
        m_spi(cs, dc), m_driver(rst, busy, m_spi), m_refresh_threshold(refresh_number), m_refresh_number(0) {
        m_canvas = new GFXCanvasBW(200, 200);
        full_refresh_threshold_height = m_driver.get_height() * refresh_threshold;
        full_refresh_threshold_width = m_driver.get_width() * refresh_threshold;
        reset_bounding_box();
    }

    ~DisplayWrapper() {
        delete m_canvas;
    }

    void clear_frame(EinkColor color = EinkColor::BLACK) {
        m_canvas->fillScreen(color.value());
        reset_bounding_box();
        m_refresh_number = 0;

        m_driver.init(false);
        m_driver.clear_frame(color);
        m_driver.display_frame();

        m_driver.clear_frame(color);
        m_driver.display_frame();
        m_driver.sleep();
    }

    void display_frame() {
        if (bounding_above_threshold() || m_refresh_number >= m_refresh_threshold) {
            debug::Print("Full refresh.\n");
            m_driver.init(false); // Full refresh
            m_refresh_number = 0;

        } else {
            debug::Print("Partial refresh.\n");
            m_driver.init(true); // Partial refresh
            m_refresh_number++;
        }
        m_driver.set_frame_memory(m_canvas->getBuffer());
        m_driver.display_frame();
        m_driver.sleep();
        reset_bounding_box();
    }

    void draw_pixel(int16_t x, int16_t y, EinkColor color) {
        update_bounding_box(x, y);
        m_canvas->drawPixel(x, y, color.value());
    }
    void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, EinkColor color) {
        update_bounding_box(x0, y0);
        update_bounding_box(x1, y1);
        m_canvas->drawLine(x0, y0, x1, y1, color.value());
    }
    void draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, EinkColor color) {
        update_bounding_box(x, y);
        update_bounding_box(x + w, y + h);
        m_canvas->drawRect(x, y, w, h, color.value());
    }
    void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, EinkColor color) {
        update_bounding_box(x, y);
        update_bounding_box(x + w, y + h);
        m_canvas->fillRect(x, y, w, h, color.value());
    }
    void draw_circle(int16_t x0, int16_t y0, int16_t r, EinkColor color) {
        update_bounding_box(x0 - r, y0 - r);
        update_bounding_box(x0 + r, y0 + r);
        m_canvas->drawCircle(x0, y0, r, color.value());
    }
    void fill_circle(int16_t x0, int16_t y0, int16_t r, EinkColor color) {
        update_bounding_box(x0 - r, y0 - r);
        update_bounding_box(x0 + r, y0 + r);
        m_canvas->fillCircle(x0, y0, r, color.value());
    }

    void set_rotation(uint8_t r) {
        m_canvas->setRotation(r);
    }

    void set_font(const GFXfont* font) {
        m_canvas->setFont(font);
    }

    void printf_text(int16_t x, int16_t y, EinkColor color, const char* format, ...) {
        if(format == nullptr) {
            debug::Print("Format is null.\n");
            return;
        }
        va_list args;
        va_start(args, format);
        char buffer[128];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        print_text(x, y, color, buffer);
    }

    void print_text(int16_t x, int16_t y, EinkColor color, const char* text) {
        if(text == nullptr) {
            debug::Print("Text is null.\n");
            return;
        }
        int16_t ul_x, ul_y;
        uint16_t w, h;
        m_canvas->getTextBounds(text, x, y, &ul_x, &ul_y, &w, &h);
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        update_bounding_box(ul_x, ul_y);
        update_bounding_box(ul_x + w, ul_y + h);

        m_canvas->setTextColor(color.value());
        m_canvas->setCursor(x, y);
        m_canvas->print(text);
    }

    void draw_bitmap(int16_t x, int16_t y, const uint8_t* bitmap, uint16_t w, uint16_t h, EinkColor fw_color, EinkColor bg_color) {
        if(bitmap == nullptr) {
            debug::Print("Bitmap is null.\n");
            return;
        }
        update_bounding_box(x, y);
        update_bounding_box(x + w, y + h);
        m_canvas->drawBitmap(x, y, bitmap, w, h, fw_color.value(), bg_color.value());
    }

    /**
     * @brief Set the display to dark mode.
     * @note Just for fun :)
     */
    void darkmode(){
        clear_frame(EinkColor::BLACK);
    }

    /**
     * @brief Set the internal cavnas to a specific color.
     * This will also reset the bounding box of canvas.
     * Nothing will be drawn on the display. This just resets the canvas.
     * @param color The color to fill the canvas with.
     */
    void clear_canvas(EinkColor color = EinkColor::WHITE) {
        m_canvas->fillScreen(EinkColor::WHITE.value());
        reset_bounding_box();
    }

    uint16_t get_canvas_width() const {
        return m_driver.get_width();
    }
    uint16_t get_canvas_height() const {
        return m_driver.get_height();
    }

private:

    /**
     * @brief Reset the bounding box to the maximum size of the display.
     * This funtion reset minimal bounding box to default settings.
     */
    void reset_bounding_box() {
        m_min_bounding_box_x = m_driver.get_width();
        m_min_bounding_box_y = m_driver.get_height();
        m_max_bounding_box_x = 0;
        m_max_bounding_box_y = 0;
    }

    /**
     * @brief Update the bounding box with new coordinates.
     * This will ensure that the bounding box always contains the drawn content.
     * @param x X coordinate
     * @param y Y coordinate
     */
    void update_bounding_box(int16_t x, int16_t y) {
        if (x < m_min_bounding_box_x) m_min_bounding_box_x = x;
        if (y < m_min_bounding_box_y) m_min_bounding_box_y = y;
        if (x > m_max_bounding_box_x) m_max_bounding_box_x = x;
        if (y > m_max_bounding_box_y) m_max_bounding_box_y = y;
    }

    /**
     * @brief Check if the bounding box is valid, right after reseting it is not valid.
     * @return true if the bounding box is valid, false otherwise.
     */
    bool is_bounding_box_valid() const {
        return (m_min_bounding_box_x <= m_max_bounding_box_x) && (m_min_bounding_box_y <= m_max_bounding_box_y);
    }
    
    /**
     * @brief Check if the bounding box is above the threshold box.
     * @return true if the bounding box is above the threshold, false otherwise.
     */
    bool bounding_above_threshold() const {
        if (!is_bounding_box_valid()) {
            return false;
        }
        uint16_t width = m_max_bounding_box_x - m_min_bounding_box_x;
        uint16_t height = m_max_bounding_box_y - m_min_bounding_box_y;
        return (width > full_refresh_threshold_width) && (height > full_refresh_threshold_height);
    }

    SPIController m_spi;
    DriverType m_driver;
    GFXCanvasBW* m_canvas;

    uint16_t m_min_bounding_box_x;
    uint16_t m_min_bounding_box_y; 
    uint16_t m_max_bounding_box_x;
    uint16_t m_max_bounding_box_y;

    uint16_t full_refresh_threshold_height;
    uint16_t full_refresh_threshold_width;

    uint8_t m_refresh_number;
    const uint8_t m_refresh_threshold;
};
