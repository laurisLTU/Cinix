#ifndef CINIX_PNG_H
#define CINIX_PNG_H

#include <stddef.h>
#include <stdint.h>

#include "cinix.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int width;
    int height;
    const uint32_t* pixels;
} CinixImage;

/*
 * Minimal image helper for Cinix demos.
 * This stores pixels as 0xAARRGGBB and draws them with X11 primitives.
 */
void cinix_draw_image_argb(CinixApp* app, const CinixImage* image, int dst_x, int dst_y);

/* Small built-in logo-like sprite */
extern const uint32_t cinix_logo_pixels[16 * 16];
extern const CinixImage cinix_logo;

#ifdef __cplusplus
}
#endif

#endif
