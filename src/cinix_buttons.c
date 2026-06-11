#include "cinix_buttons.h"
#include <string.h>

void cinix_button_draw(CinixApp* app, const CinixButton* button, int mouse_x, int mouse_y) {
    unsigned long fill;
    int inside;

    if (app == NULL || button == NULL) {
        return;
    }

    inside = cinix_button_hit(button, mouse_x, mouse_y);
    if (button->pressed) {
        fill = button->color_pressed;
    } else if (inside) {
        fill = button->color_hover;
    } else {
        fill = button->color_idle;
    }

    XSetForeground(app->display, app->gc, fill);
    XFillRectangle(app->display, app->surface, app->gc, button->x, button->y, (unsigned int)button->width, (unsigned int)button->height);

    XSetForeground(app->display, app->gc, cinix_rgb(app, 30, 30, 30));
    XDrawRectangle(app->display, app->surface, app->gc, button->x, button->y, (unsigned int)button->width, (unsigned int)button->height);

    if (button->label != 0) {
        XDrawString(app->display, app->surface, app->gc, button->x + 10, button->y + (button->height / 2) + 4, button->label, (int)strlen(button->label));
    }
}

int cinix_button_hit(const CinixButton* button, int x, int y) {
    if (button == NULL) {
        return 0;
    }

    return x >= button->x &&
           y >= button->y &&
           x <= button->x + button->width &&
           y <= button->y + button->height;
}

