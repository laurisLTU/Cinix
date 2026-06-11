#ifndef CINIX_TUX_H
#define CINIX_TUX_H

#include <stdint.h>

#include "cinix.h"
#include "cinix_png.h"

typedef struct {
    int loaded;
    uint32_t* pixels;
    CinixImage image;
} CinixTux;

int cinix_tux_load(CinixTux* tux, const char* path);
void cinix_tux_free(CinixTux* tux);
void cinix_tux_draw(CinixApp* app, const CinixWindow* main_window, const CinixTux* tux);
void cinix_tux_draw_splash(CinixApp* app, const CinixTux* tux);

#endif
