#include "renderer.h"
#include "SDL.h"


SDL_Surface* screen;

gfx_texture  gfx_frame_buffer = { NULL, display_width, display_height };
gfx_texture* gfx_render_target = &gfx_frame_buffer;
gfx_texture* font;

inline gfx_color renderer_color_rgb(uint8_t red, uint8_t green, uint8_t blue) 
{
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
}

void renderer_init(char *gamePath) 
{
    char temp[256];
    
    sprintf(temp, "%sfont.tga", gamePath);
    
//    screen = SDL_SetVideoMode(display_width, display_height, 16, SDL_DOUBLEBUF | SDL_HWSURFACE);
    screen = SDL_SetVideoMode(display_width, display_height, 16, SDL_HWSURFACE);
    font = renderer_load_tga(temp);
    
    gfx_frame_buffer.address = screen->pixels;
    gfx_frame_buffer.width   = display_width;
    gfx_frame_buffer.height  = display_height;
    
    renderer_clear(0, 0, 0);
}

gfx_texture* renderer_load_tga(const char* path) 
{
    FILE* file;
    gfx_texture* texture;

    uint8_t  tga_ident_size;
    uint8_t  tga_color_map_type;
    uint8_t  tga_image_type;
    uint16_t tga_color_map_start;
    uint16_t tga_color_map_length;
    uint8_t  tga_color_map_bpp;
    uint16_t tga_origin_x;
    uint16_t tga_origin_y;
    uint16_t tga_width;
    uint16_t tga_height;
    uint8_t  tga_bpp;
    uint8_t  tga_descriptor;
    
    uintptr_t i;
    uint8_t color[3];
    uint16_t* texPtr;
    
    
    if(path == NULL) return NULL;

    file = fopen(path, "rb");
        
    if(file == NULL) return NULL;

    fread(&tga_ident_size, 1, 1, file);
    fread(&tga_color_map_type, 1, 1, file);
    fread(&tga_image_type, 1, 1, file);
    fread(&tga_color_map_start, 2, 1, file);
    fread(&tga_color_map_length, 2, 1, file);
    fread(&tga_color_map_bpp, 1, 1, file);
    fread(&tga_origin_x, 2, 1, file);
    fread(&tga_origin_y, 2, 1, file);
    fread(&tga_width, 2, 1, file);
    fread(&tga_height, 2, 1, file);
    fread(&tga_bpp, 1, 1, file);
    fread(&tga_descriptor, 1, 1, file);

    texture = (gfx_texture*)malloc(sizeof(gfx_texture) + (tga_width * tga_height * 2));
    
    if(texture == NULL) 
    {
        fclose(file);
        return NULL;
    }
    
    texture->address = (void*)((uintptr_t)texture + sizeof(gfx_texture));
    texture->width = tga_width;
    texture->height = tga_height;

    texPtr = (uint16_t*)texture->address;
    
    for(i = 0; i < (tga_width * tga_height); i++) 
    {
        fread(&color[2], 1, 1, file);
        fread(&color[1], 1, 1, file);
        fread(&color[0], 1, 1, file);
        texPtr[i] = renderer_color_rgb(color[0], color[1], color[2]);
    }
    
    fclose(file);

    return texture;
}

void renderer_clear(uint8_t red, uint8_t green, uint8_t blue) 
{
    uint32_t* tempPtr = (uint32_t*)gfx_frame_buffer.address;
    uint32_t* end = (uint32_t*)((uintptr_t)tempPtr + (display_width * display_height << 1));
    uint32_t  color = (blue >> 3) | ((green & 0xFC) << 3) | ((red & 0xF8) << 10);
    
    color |= (color << 16);
    
    while(tempPtr < end) 
    {
        *(tempPtr++) = color;
        *(tempPtr++) = color;
        *(tempPtr++) = color;
        *(tempPtr++) = color;
        *(tempPtr++) = color;
        *(tempPtr++) = color;
        *(tempPtr++) = color;
        *(tempPtr++) = color;
    }
}

void renderer_draw_texture(int16_t inX, int16_t inY, gfx_texture* texture) 
{
    if(texture == NULL) return;

    uint16_t* buffer = (uint16_t*)gfx_render_target->address;
    uint16_t* temp = (uint16_t*)texture->address;

    int16_t x, y, i, j;
    
    for(j = 0, y = inY; (j < texture->height) && (y < gfx_render_target->height); j++, y++)
    {
        for(i = 0, x = inX; (i < texture->width) && (x < gfx_render_target->width); i++, x++) 
        {
            buffer[(y * gfx_render_target->width) + x] = temp[(j * texture->width) + i];
        }
    }
}

uint16_t renderer_font_width(char* string) 
{
    uintptr_t i, tempCur, tempMax;
    
    if((font == NULL) || (string == NULL)) return 0;
        
    for(i = 0, tempCur = 0, tempMax = 0; string[i] != '\0'; i++) 
    {
        if(string[i] == '\t') tempCur += 4;
        else if((string[i] == '\r') || (string[i] == '\n')) tempCur = 0;
        else tempCur++;
        
        if(tempCur > tempMax) tempMax = tempCur;
    }
    
    tempMax *= (font->width >> 4);
    
    return tempMax;
}

uint16_t renderer_font_height() 
{
    if(font == NULL) return 0;

    return (font->height >> 4);
}

uint16_t renderer_next_word_width(char* inString)
{
    int count = 0;
    char* tempChar;
    
    for(tempChar = inString; *tempChar != '\0'; tempChar++) 
    {
        if(*tempChar == ' ') break;
        if(*tempChar == '\t') break;
        if(*tempChar == '\r') break;
        if(*tempChar == '\n') break;
        
        count++;
        
        if(*tempChar == ',') break;
        if(*tempChar == '\"') break;
//        if(*tempChar == '\'') break;
        if(*tempChar == '/') break;
        if(*tempChar == '\\') break;
        if(*tempChar == '&') break;
        if(*tempChar == ';') break;
    }
    
    return (count * (font->width >> 4));
}

void renderer_font_print(gfx_cursor *cursor, char* inString) 
{
    uint16_t* buffer = (uint16_t*)gfx_render_target->address;
    uint16_t* tempFont = (uint16_t*)font->address;
    char* tempChar = inString;
    uintptr_t i, j, x, y;
    
    if((font == NULL) || (inString == NULL)) return;

    for(tempChar = inString; *tempChar != '\0'; tempChar++) 
    {
        if(*tempChar == ' ') 
        {
            cursor->x += (font->width >> 4);
            continue;
        }
        
        if(*tempChar == '\t') 
        {
            cursor->x += ((font->width >> 4) << 2);
            continue;
        }
        
        if(*tempChar == '\r') 
        {
            cursor->x = 0;
            continue;
        }
        
        if(*tempChar == '\n') 
        {
            cursor->x = 0;
            cursor->y += (font->height >> 4);
            
            continue;
        }
        
        if(cursor->x + renderer_next_word_width(tempChar) >= display_width)
        {
            cursor->x = 0;
            cursor->y += (font->height >> 4);
        }
        
        for(j = ((*tempChar >> 4) * (font->height >> 4)), y = cursor->y; (j < (((*tempChar >> 4) + 1) * (font->height >> 4))) && (y < gfx_render_target->height); j++, y++) 
        {
            for(i = ((*tempChar & 0x0F) * (font->width >> 4)), x = cursor->x; (i < (((*tempChar & 0x0F) + 1) * (font->width >> 4))) && (x < gfx_render_target->width); i++, x++) 
            {
                buffer[(y * gfx_render_target->width) + x] |= tempFont[(j * font->width) + i];
            }
        }
        
        cursor->x += (font->width >> 4);
    }
}

void renderer_fill_rect(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t red, uint8_t green, uint8_t blue) 
{    
    gfx_color color = renderer_color_rgb(red, green, blue);
    uint16_t* buffer = (uint16_t*)gfx_render_target->address;
    uintptr_t i, j;
    
    if(x < 0) 
    {
        width += x;
        x = 0;
    }
    
    if(y < 0) 
    {
        height += y;
        y = 0;
    }

    for(j = y; (j < gfx_render_target->height) && (j < (y + height)); j++) 
    {
        for(i = x; (i < gfx_render_target->width) && (i < (x + width)); i++) 
        {
            buffer[(j * gfx_render_target->width) + i] = color;
        }
    }
}

int renderer_save_tga(const char* path, gfx_texture* texture) 
{
    if((path == NULL) || (texture == NULL) || (texture->address == NULL)) return 0;
    
    FILE* file = fopen(path, "wb");

    uint8_t  tga_ident_size = 0;
    uint8_t  tga_color_map_type = 0;
    uint8_t  tga_image_type = 2;
    uint16_t tga_color_map_start = 0;
    uint16_t tga_color_map_length = 0;
    uint8_t  tga_color_map_bpp = 0;
    uint16_t tga_origin_x = 0;
    uint16_t tga_origin_y = 0;
    int16_t  tga_width = texture->width;
    int16_t  tga_height = texture->height;
    uint8_t  tga_bpp = 24;
    uint8_t  tga_descriptor = 0x20;
    
    uintptr_t i;
    uint8_t color[3];
    uint16_t* buffer = (uint16_t*)texture->address;
    
    if(file == NULL) return 0;

    fwrite(&tga_ident_size, 1, 1, file);
    fwrite(&tga_color_map_type, 1, 1, file);
    fwrite(&tga_image_type, 1, 1, file);
    fwrite(&tga_color_map_start, 2, 1, file);
    fwrite(&tga_color_map_length, 2, 1, file);
    fwrite(&tga_color_map_bpp, 1, 1, file);
    fwrite(&tga_origin_x, 2, 1, file);
    fwrite(&tga_origin_y, 2, 1, file);
    fwrite(&tga_width, 2, 1, file);
    fwrite(&tga_height, 2, 1, file);
    fwrite(&tga_bpp, 1, 1, file);
    fwrite(&tga_descriptor, 1, 1, file);

    for(i = 0; i < (tga_width * tga_height); i++) 
    {
        color[0]  = ((buffer[i] >> 8) & 0xF8);
        color[0] |= (color[0] >> 5);
        color[1]  = ((buffer[i] >> 3) & 0xFC);
        color[1] |= (color[1] >> 6);
        color[2]  = ((buffer[i] << 3) & 0xF8);
        color[2] |= (color[2] >> 5);
        
        fwrite(&color[2], 1, 1, file);
        fwrite(&color[1], 1, 1, file);
        fwrite(&color[0], 1, 1, file);
    }
    
    fclose(file);

    return 1;
}

void renderer_screenshot()
{
    char path[256];
    unsigned long int number = 0;
    FILE* file;
    
    while(1) 
    {
        sprintf(path, "%s/screenshot%lu.tga", getenv("HOME"), number);
        
        file = fopen(path, "rb");
        
        if(file == NULL) break;
        
        fclose(file);
        number++;
    }
    
    renderer_save_tga(path, gfx_render_target);
}

void renderer_release() 
{
    SDL_FreeSurface(screen);
}