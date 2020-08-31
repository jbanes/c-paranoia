/* 
 * File:   renderer.h
 * Author: jerasonbanes
 *
 * Created on August 27, 2020, 9:50 PM
 */

#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>

typedef uint16_t gfx_color;

typedef struct {
    void*      address;
    uint16_t   width, height;
} gfx_texture;

typedef struct {
    uint16_t x, y;
} gfx_cursor;

#define display_width  320
#define display_height 240

void renderer_init();
gfx_texture* renderer_load_tga(const char* inPath);
void renderer_clear(uint8_t red, uint8_t green, uint8_t blue);
void renderer_draw_texture(int16_t inX, int16_t inY, gfx_texture* texture);
uint16_t renderer_font_width(char* string);
uint16_t renderer_font_height();
void renderer_font_print(gfx_cursor* cursor, char* inString);
void renderer_fill_rect(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t red, uint8_t green, uint8_t blue);
int renderer_save_tga(const char* path, gfx_texture* texture);
void renderer_screenshot();
void renderer_release();


#endif /* RENDERER_H */

