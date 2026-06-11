#include <string.h>

#include "about.h"

void cinix_about_draw(CinixApp* app, const CinixTheme* theme, const CinixWindow* main_window) {
    XSetForeground(app->display, app->gc, theme->text_main);
    XDrawString(app->display, app->surface, app->gc, main_window->x + 30, main_window->y + 80, "About Cinix", (int)strlen("About Cinix"));
    XDrawString(app->display, app->surface, app->gc, main_window->x + 30, main_window->y + 105, "Cinix desktop chrome is now built into cinix.c", (int)strlen("Cinix desktop chrome is now built into cinix.c"));
    XDrawString(app->display, app->surface, app->gc, main_window->x + 30, main_window->y + 130, "Use Start -> Tux to open the Tux view.", (int)strlen("Use Start -> Tux to open the Tux view."));
}
