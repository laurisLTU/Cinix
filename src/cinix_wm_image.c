#include "cinix_wm.h"

#include <png.h>
#include <stdio.h>
#include <stdlib.h>

int cinix_wm_load_png(const char* path, uint32_t** out_pixels, int* out_w, int* out_h) {
    FILE* fp = fopen(path, "rb");
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep* rows;
    png_bytep raw = NULL;
    uint32_t* pixels = NULL;
    int width;
    int height;
    int x;
    int y;

    if (fp == NULL) return 0;
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fclose(fp);
        return 0;
    }
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return 0;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        free(raw);
        free(pixels);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return 0;
    }

    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);
    width = (int)png_get_image_width(png_ptr, info_ptr);
    height = (int)png_get_image_height(png_ptr, info_ptr);
    if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
    if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY && png_get_bit_depth(png_ptr, info_ptr) < 8) png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
    if (png_get_bit_depth(png_ptr, info_ptr) == 16) png_set_strip_16(png_ptr);
    if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY || png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png_ptr);
    if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGBA) png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
    png_read_update_info(png_ptr, info_ptr);

    raw = (png_bytep)malloc((size_t)height * (size_t)png_get_rowbytes(png_ptr, info_ptr));
    rows = (png_bytep*)malloc(sizeof(png_bytep) * (size_t)height);
    pixels = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)width * (size_t)height);
    if (raw == NULL || rows == NULL || pixels == NULL) {
        free(rows);
        free(raw);
        free(pixels);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return 0;
    }

    for (y = 0; y < height; ++y) rows[y] = raw + (size_t)y * png_get_rowbytes(png_ptr, info_ptr);
    png_read_image(png_ptr, rows);
    for (y = 0; y < height; ++y) {
        png_bytep row = rows[y];
        for (x = 0; x < width; ++x) {
            unsigned char r = row[x * 4 + 0];
            unsigned char g = row[x * 4 + 1];
            unsigned char b = row[x * 4 + 2];
            unsigned char a = row[x * 4 + 3];
            pixels[y * width + x] = ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }

    free(rows);
    free(raw);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    *out_pixels = pixels;
    *out_w = width;
    *out_h = height;
    return 1;
}

void cinix_wm_free_image(uint32_t** pixels) {
    if (pixels != NULL) {
        free(*pixels);
        *pixels = NULL;
    }
}
