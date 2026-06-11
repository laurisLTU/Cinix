#ifndef CINIX_BUTTONS_H
#define CINIX_BUTTONS_H

#include "cinix.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int x;
    int y;
    int width;
    int height;
    const char* label;
    int pressed;
    unsigned long color_idle;
    unsigned long color_hover;
    unsigned long color_pressed;
} CinixButton;

void cinix_button_draw(CinixApp* app, const CinixButton* button, int mouse_x, int mouse_y);
int cinix_button_hit(const CinixButton* button, int x, int y);

#ifdef __cplusplus
}
#endif

#endif
