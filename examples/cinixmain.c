#define _POSIX_C_SOURCE 199309L

#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cinix.h"
#include "cinix_png.h"
#include "about.h"
#include "controls.h"
#include "tux.h"

#define VIEW_HOME 0
#define VIEW_ABOUT 1
#define VIEW_TUX 2

int main(void) {
    CinixApp app;
    CinixDesktop desktop;
    CinixTheme theme;
    CinixControls controls;
    CinixTux tux;
    XEvent event;

    int view = VIEW_HOME;
    CinixWindow windows[2];

    if (!cinix_init(&app, 920, 560, "Cinix C UI (Linux/X11)")) {
        return 1;
    }

    cinix_theme_default(&app, &theme);
    cinix_desktop_init(&app, &desktop, &theme);

    windows[0].x = 50;
    windows[0].y = 60;
    windows[0].width = 560;
    windows[0].height = 430;
    windows[0].title = "Cinix Main Window";
    cinix_window_apply_theme(&windows[0], &theme, 0);

    windows[1].x = 640;
    windows[1].y = 110;
    windows[1].width = 230;
    windows[1].height = 300;
    windows[1].title = "Controls";
    cinix_window_apply_theme(&windows[1], &theme, 1);

    cinix_controls_init(&app, &controls);
    cinix_tux_load(&tux, "images/Tux.png");
    if (!tux.loaded) {
        cinix_tux_load(&tux, "images/tux.png");
    }

    while (app.running) {
        struct timespec frame_delay;
        frame_delay.tv_sec = 0;
        frame_delay.tv_nsec = 8 * 1000 * 1000;

        cinix_controls_update_positions(&controls, &windows[1]);

        while (XPending(app.display) > 0) {
            XNextEvent(app.display, &event);
            cinix_desktop_handle_event(&app, &desktop, &event, windows, 2);

            if (event.type == ButtonPress && event.xbutton.button == Button1) {
                cinix_controls_handle_press(&controls, event.xbutton.x, event.xbutton.y);
            }

            if (event.type == ButtonRelease && event.xbutton.button == Button1) {
                int ctrl_action = cinix_controls_handle_release(&controls, event.xbutton.x, event.xbutton.y);
                if (ctrl_action == 1) {
                    puts("Cinix: Launch button clicked.");
                } else if (ctrl_action == 2) {
                    app.running = 0;
                }
            }
        }

        {
            int action = cinix_desktop_consume_action(&desktop);
            if (action == CINIX_START_ACTION_ABOUT) {
                view = VIEW_ABOUT;
            } else if (action == CINIX_START_ACTION_TUX) {
                view = VIEW_TUX;
            } else if (action == CINIX_START_ACTION_SETTINGS) {
                view = VIEW_HOME;
            } else if (action == CINIX_START_ACTION_EXIT) {
                app.running = 0;
            }
        }

        cinix_begin_frame(&app);

        cinix_draw_window(&app, &windows[0]);
        cinix_draw_window(&app, &windows[1]);

        if (view == VIEW_ABOUT) {
            cinix_about_draw(&app, &theme, &windows[0]);
        } else if (view == VIEW_TUX) {
            cinix_tux_draw(&app, &windows[0], &tux);
        } else {
            cinix_draw_image_argb(&app, &cinix_logo, windows[0].x + 30, windows[0].y + 35);
            XSetForeground(app.display, app.gc, theme.text_main);
            XDrawString(app.display, app.surface, app.gc, windows[0].x + 30, windows[0].y + 80, "Cinix in C for Linux", (int)strlen("Cinix in C for Linux"));
            XDrawString(app.display, app.surface, app.gc, windows[0].x + 30, windows[0].y + 105, "Start menu and desktop logic are in cinix.c", (int)strlen("Start menu and desktop logic are in cinix.c"));
        }

        cinix_controls_draw(&app, &controls, desktop.mouse_x, desktop.mouse_y);
        cinix_desktop_draw_chrome(&app, &desktop);
        cinix_present(&app);
        nanosleep(&frame_delay, NULL);
    }

    cinix_tux_free(&tux);
    cinix_shutdown(&app);
    return 0;
}
