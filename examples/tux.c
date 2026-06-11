#include "tux.h"

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int load_png_rgba(const char* path, uint32_t** out_pixels, int* out_w, int* out_h) {
    FILE* fp = fopen(path, "rb");
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep* rows;
    png_bytep raw = NULL;
    uint32_t* pixels = NULL;
    int x;
    int y;
    int width;
    int height;

    if (!fp) {
        return 0;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        return 0;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
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

    if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);
    }
    if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY && png_get_bit_depth(png_ptr, info_ptr) < 8) {
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png_ptr);
    }
    if (png_get_bit_depth(png_ptr, info_ptr) == 16) {
        png_set_strip_16(png_ptr);
    }
    if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY ||
        png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png_ptr);
    }
    if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGBA) {
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
    }

    png_read_update_info(png_ptr, info_ptr);

    raw = (png_bytep)malloc((size_t)height * (size_t)png_get_rowbytes(png_ptr, info_ptr));
    rows = (png_bytep*)malloc(sizeof(png_bytep) * (size_t)height);
    pixels = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)width * (size_t)height);

    if (!raw || !rows || !pixels) {
        free(rows);
        free(raw);
        free(pixels);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return 0;
    }

    for (y = 0; y < height; ++y) {
        rows[y] = raw + (size_t)y * png_get_rowbytes(png_ptr, info_ptr);
    }

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

int cinix_tux_load(CinixTux* tux, const char* path) {
    if (!tux) {
        return 0;
    }

    memset(tux, 0, sizeof(*tux));
    if (!load_png_rgba(path, &tux->pixels, &tux->image.width, &tux->image.height)) {
        return 0;
    }

    tux->image.pixels = tux->pixels;
    tux->loaded = 1;
    return 1;
}

void cinix_tux_free(CinixTux* tux) {
    if (!tux) {
        return;
    }
    free(tux->pixels);
    memset(tux, 0, sizeof(*tux));
}

void cinix_tux_draw(CinixApp* app, const CinixWindow* main_window, const CinixTux* tux) {
    if (!tux || !tux->loaded) {
        const char* msg = "Could not load images/Tux.png";
        XSetForeground(app->display, app->gc, cinix_rgb(app, 40, 40, 40));
        XDrawString(app->display, app->surface, app->gc, main_window->x + 30, main_window->y + 90, msg, (int)strlen(msg));
        return;
    }

    cinix_draw_image_argb(app, &tux->image, main_window->x + 30, main_window->y + 45);
}

void cinix_tux_draw_splash(CinixApp* app, const CinixTux* tux) {
    int max_dim = 220;
    int dst_w;
    int dst_h;
    int dst_x;
    int dst_y;
    int x;
    int y;
    int src_x;
    int src_y;

    XSetForeground(app->display, app->gc, cinix_rgb(app, 0, 0, 0));
    XFillRectangle(app->display, app->surface, app->gc, 0, 0, (unsigned int)app->width, (unsigned int)app->height);

    if (!tux || !tux->loaded || tux->image.width <= 0 || tux->image.height <= 0) {
        return;
    }

    if (tux->image.width >= tux->image.height) {
        dst_w = max_dim;
        dst_h = (tux->image.height * max_dim) / tux->image.width;
    } else {
        dst_h = max_dim;
        dst_w = (tux->image.width * max_dim) / tux->image.height;
    }

    if (dst_w < 1) dst_w = 1;
    if (dst_h < 1) dst_h = 1;

    dst_x = (app->width - dst_w) / 2;
    dst_y = (app->height - dst_h) / 2;

    for (y = 0; y < dst_h; ++y) {
        src_y = (y * tux->image.height) / dst_h;
        for (x = 0; x < dst_w; ++x) {
            uint32_t px;
            unsigned char a;
            unsigned char r;
            unsigned char g;
            unsigned char b;

            src_x = (x * tux->image.width) / dst_w;
            px = tux->image.pixels[src_y * tux->image.width + src_x];
            a = (unsigned char)((px >> 24) & 0xFF);
            if (a == 0) {
                continue;
            }
            r = (unsigned char)((px >> 16) & 0xFF);
            g = (unsigned char)((px >> 8) & 0xFF);
            b = (unsigned char)(px & 0xFF);
            XSetForeground(app->display, app->gc, cinix_rgb(app, r, g, b));
            XDrawPoint(app->display, app->surface, app->gc, dst_x + x, dst_y + y);
        }
    }
}
