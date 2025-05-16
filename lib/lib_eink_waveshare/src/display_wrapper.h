#pragma once

#include <Arduino.h>
#include <type_traits>

#include <Adafruit_GFX.h>

#include "spi_controller.h"
#include "my_utils.h"
#include "eink_driver.h"


namespace EinkCanvas{

/**
 * @class GFXCanvasBW
 * @brief Black and white canvas implementation for Adafruit GFX library
 * 
 * GFXCanvasBW provides a memory buffer-based black and white drawing surface 
 * where each pixel is represented by a single bit (1-bit per pixel). This allows
 * for memory-efficient display buffers for monochrome displays.
 * 
 * The class inherits from Adafruit_GFX, providing all standard drawing primitives
 * while implementing the pixel-specific operations for a 1-bit per pixel buffer.
 * 
 * @note The buffer is organized with 8 pixels per byte. The bits within each byte
 *       are ordered from most significant bit (leftmost pixel) to least significant bit.
 */
class GFXCanvasBW : public Adafruit_GFX {
public:
    /**
     * @brief Construct a new black and white canvas
     * 
     * @param w Width of the canvas in pixels
     * @param h Height of the canvas in pixels
     */
    GFXCanvasBW(uint16_t w, uint16_t h): Adafruit_GFX(w, h) {
        buffer = new uint8_t[(w * h + 7) / 8];
        memset(buffer, 0, (w * h + 7) / 8);
    }

    ~GFXCanvasBW() {
        delete[] buffer;
    }

    /**
     * @brief Draw a pixel at the specified coordinates
     * 
     * @param x X coordinate
     * @param y Y coordinate
     * @param color Pixel color (any non-zero value is treated as "on")
     * @note The function handles bounds checking and will not draw outside the canvas
     */
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

    /**
     * @brief Get the internal pixel buffer
     * 
     * @return Pointer to the raw pixel buffer
     * @note Each byte contains 8 pixels, with the MSB representing the leftmost pixel
     */
    uint8_t *getBuffer() {
        return buffer;
    }

private:
    uint8_t *buffer;
};

} // namespace EinkCanvas


namespace EinkDisplay{

template <
    typename DriverType,
    typename = typename std::enable_if<
        std::is_base_of<EinkDriver::Interface, DriverType>::value
    >::type
>
/**
 * @class DisplayHandle
 * @brief A wrapper class for e-ink display operations that provides drawing functions and manages display refreshes.
 *
 * This class provides a high-level interface for interacting with e-ink displays, handling the
 * complexities of display refreshing (both full and partial), memory management, and drawing operations.
 * It maintains a canvas for drawing and implements intelligent refresh strategies to optimize 
 * display updates while minimizing ghosting effects.
 * 
 * The class tracks drawing operations through a bounding box mechanism to determine when a full
 * refresh is needed instead of a partial one, based on the amount of changed content.
 *
 * @note This wrapper is designed for use with Waveshare e-ink displays or compatible drivers.
 *
 * Usage example:
 * @code
 * DisplayHandle display(8, 9, 10, 11); // CS, DC, RST, BUSY pins
 * display.clear_frame(EinkColor::WHITE);
 * display.draw_text(10, 20, EinkColor::BLACK, "Hello World");
 * display.display_frame();
 * @endcode
 */
class DisplayHandle {
public:
    /**
     * @brief Constructs a new DisplayHandle for e-paper/e-ink displays
     * 
     * This class wraps the hardware driver for e-ink displays and provides drawing functionality
     * with smart refresh management. It tracks modified areas and can decide between partial
     * and full refreshes based on thresholds.
     * 
     * @param cs Chip select pin for SPI communication
     * @param dc Data/command pin for SPI communication
     * @param rst Reset pin for the display
     * @param busy Busy signal pin for the display
     * @param refresh_threshold Ratio (0.0-1.0) of screen change that triggers a full refresh instead of partial
     * @param refresh_number Number of refreshes before forcing a full refresh
     * 
     * @note The constructor initializes an internal canvas of 200x200 pixels and calculates
     *       refresh thresholds based on the display dimensions.
     */
    DisplayHandle(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t busy, float refresh_threshold = 0.7, uint8_t refresh_number = 10) :
        m_spi(cs, dc), m_driver(rst, busy, m_spi), m_refresh_threshold(refresh_number), m_refresh_number(0) {
        m_canvas = new EinkCanvas::GFXCanvasBW(200, 200);
        m_canvas->fillScreen(EinkColor::WHITE.value());
        full_refresh_threshold_height = m_driver.get_height() * refresh_threshold;
        full_refresh_threshold_width = m_driver.get_width() * refresh_threshold;
        reset_bounding_box();
    }

    ~DisplayHandle() {
        delete m_canvas;
    }

    /**
     * @brief Clears the entire e-ink display frame with a specified color.
     * 
     * This function performs a full refresh of the display, filling it with the
     * specified color (white by default)
     * @param color The color to fill the display with, defaults to WHITE
     */
    void clear_frame(EinkColor color = EinkColor::WHITE) {
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
    
    /**
     * @brief Displays the current frame on the e-ink display.
     * 
     * This function checks if a full refresh is needed based on the bounding box
     * and refresh thresholds. It then sends the frame buffer to the display.
     */
    void display_frame() {
        if (bounding_above_threshold() || m_refresh_number >= m_refresh_threshold) {
            debug::Print("Full refresh.\n");
            m_driver.init(false); // Full refresh
            m_refresh_number = 0;
            m_driver.set_frame_memory(m_canvas->getBuffer());
        } else {
            debug::Print("Partial refresh.\n");
            m_driver.init(true); // Partial refresh
            m_refresh_number++;
            m_driver.set_frame_memory(m_canvas->getBuffer(), m_min_bounding_box_x, m_min_bounding_box_y,
                                              m_max_bounding_box_x, m_max_bounding_box_y);
        }
        m_driver.display_frame();
        m_driver.sleep();
        reset_bounding_box();
    }

    /**
     * @brief Draws a pixel on the canvas at specified coordinates with a given color.
     *
     * @param x The x-coordinate of the pixel.
     * @param y The y-coordinate of the pixel.
     * @param color The color of the pixel.
     */
    void draw_pixel(int16_t x, int16_t y, EinkColor color) {
        update_bounding_box(x, y);
        m_canvas->drawPixel(x, y, color.value());
    }

    /**
     * @brief Draws a line on the canvas between two points with a given color.
     *
     * @param x0 The x-coordinate of the start point.
     * @param y0 The y-coordinate of the start point.
     * @param x1 The x-coordinate of the end point.
     * @param y1 The y-coordinate of the end point.
     * @param color The color of the line.
     */
    void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, EinkColor color) {
        update_bounding_box(x0, y0);
        update_bounding_box(x1, y1);
        m_canvas->drawLine(x0, y0, x1, y1, color.value());
    }

    /**
     * @brief Draws a rectangle on the canvas with a given color.
     *
     * @param x The x-coordinate of the top-left corner.
     * @param y The y-coordinate of the top-left corner.
     * @param w The width of the rectangle.
     * @param h The height of the rectangle.
     * @param color The color of the rectangle.
     */
    void draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, EinkColor color) {
        update_bounding_box(x, y);
        update_bounding_box(x + w, y + h);
        m_canvas->drawRect(x, y, w, h, color.value());
    }

    /**
     * @brief Fills a rectangle on the canvas with a given color.
     *
     * @param x The x-coordinate of the top-left corner.
     * @param y The y-coordinate of the top-left corner.
     * @param w The width of the rectangle.
     * @param h The height of the rectangle.
     * @param color The color of the rectangle.
     */
    void fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, EinkColor color) {
        update_bounding_box(x, y);
        update_bounding_box(x + w, y + h);
        m_canvas->fillRect(x, y, w, h, color.value());
    }

    /**
     * @brief Draws a circle on the canvas with a given color.
     *
     * @param x0 The x-coordinate of the center.
     * @param y0 The y-coordinate of the center.
     * @param r The radius of the circle.
     * @param color The color of the circle.
     */
    void draw_circle(int16_t x0, int16_t y0, int16_t r, EinkColor color) {
        update_bounding_box(x0 - r, y0 - r);
        update_bounding_box(x0 + r, y0 + r);
        m_canvas->drawCircle(x0, y0, r, color.value());
    }

    /**
     * @brief Fills a circle on the canvas with a given color.
     *
     * @param x0 The x-coordinate of the center.
     * @param y0 The y-coordinate of the center.
     * @param r The radius of the circle.
     * @param color The color of the circle.
     */
    void fill_circle(int16_t x0, int16_t y0, int16_t r, EinkColor color) {
        update_bounding_box(x0 - r, y0 - r);
        update_bounding_box(x0 + r, y0 + r);
        m_canvas->fillCircle(x0, y0, r, color.value());
    }

    /**
     * @brief Sets the rotation of the canvas.
     *
     * @param r The rotation value (0-3).
     */
    void set_rotation(uint8_t r) {
        m_canvas->setRotation(r);
    }

    /**
     * @brief Sets the font for text rendering on the canvas.
     *
     * @param font Pointer to the GFXfont structure representing the font.
     */
    void set_font(const GFXfont* font) {
        m_canvas->setFont(font);
    }

    /**
     * @brief Prints formatted text on the canvas.
     * Format string is similar to printf.
     * @param x The x-coordinate of the text.
     * @param y The y-coordinate of the text.
     * @param color The color of the text.
     * @param format The format string.
     * @param ... The values to be inserted into the format string.
     */
    void printf(int16_t x, int16_t y, EinkColor color, const char* format, ...) {
        if(format == nullptr) {
            debug::Print("Format is null.\n");
            return;
        }
        va_list args;
        va_start(args, format);
        char buffer[128];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        print(x, y, color, buffer);
    }

    /**
     * @brief Prints text on the canvas.
     *
     * @param x The x-coordinate of the text.
     * @param y The y-coordinate of the text.
     * @param color The color of the text.
     * @param text The text to be printed.
     */
    void print(int16_t x, int16_t y, EinkColor color, const char* text) {
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

    /**
     * @brief Draws a bitmap on the canvas.
     *
     * @param x The x-coordinate of the bitmap.
     * @param y The y-coordinate of the bitmap.
     * @param bitmap Pointer to the bitmap data.
     * @param w The width of the bitmap.
     * @param h The height of the bitmap.
     * @param fw_color The foreground color of the bitmap.
     * @param bg_color The background color of the bitmap.
     */
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
     * @brief Set the internal canvas to a specific color.
     * This will also reset the bounding box of canvas.
     * Nothing will be drawn on the display. This just resets the canvas.
     * @param color The color to fill the canvas with.
     */
    void clear_buffer(EinkColor color = EinkColor::WHITE) {
        m_canvas->fillScreen(EinkColor::WHITE.value());
        reset_bounding_box();
    }

    /**
     * @brief Get the width of the canvas, respectively the display in pixels.
     * @return The width of the canvas in pixels.
     */
    uint16_t get_canvas_width() const {
        return m_driver.get_width();
    }

    /**
     * @brief Get the height of the canvas, respectively the display in pixels.
     * @return The height of the canvas in pixels.
     */
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

    EinkSPI::SPIController m_spi;
    DriverType m_driver;
    EinkCanvas::GFXCanvasBW* m_canvas;

    uint16_t m_min_bounding_box_x;
    uint16_t m_min_bounding_box_y; 
    uint16_t m_max_bounding_box_x;
    uint16_t m_max_bounding_box_y;

    uint16_t full_refresh_threshold_height;
    uint16_t full_refresh_threshold_width;

    uint8_t m_refresh_number;
    const uint8_t m_refresh_threshold;
};

} // namespace EinkDisplay
