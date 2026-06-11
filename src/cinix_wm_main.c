#define _POSIX_C_SOURCE 199309L

#include "cinix_wm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int g_other_wm_detected = 0;

static int cinix_wm_error_handler(Display* display, XErrorEvent* error) {
    (void)display;
    if (error->error_code == BadAccess) g_other_wm_detected = 1;
    return 0;
}

static void launch_app(CinixWM* wm, int app) {
    char cmdline[160];
    if (app < 0 || app >= wm->dock_app_count) return;
    snprintf(cmdline, sizeof(cmdline), "%s >/dev/null 2>&1 &", wm->dock_apps[app].command);
    wm->dock_launch[app] = 1.0;
    (void)system(cmdline);
}

static int dock_item_from_x(int x) {
    int item = (x - 24) / 82;
    int local_x = (x - 24) % 82;
    if (x < 24 || item < 0 || item >= CINIX_WM_DOCK_APPS || local_x < 0 || local_x > 70) return -1;
    return item;
}

static void launch_gui_app(int item) {
    const char* commands[CINIX_WM_GUI_APPS] = { "xterm", "xterm -e htop", "thunar", "firefox" };
    char cmdline[180];
    if (item < 0 || item >= CINIX_WM_GUI_APPS) return;
    snprintf(cmdline, sizeof(cmdline), "%s >/dev/null 2>&1 &", commands[item]);
    (void)system(cmdline);
}

static void add_dock_app(CinixWM* wm, int item) {
    const char* labels[CINIX_WM_GUI_APPS] = { "Xterm", "HTop", "Files", "Firefox" };
    const char* commands[CINIX_WM_GUI_APPS] = { "xterm", "xterm -e htop", "thunar", "firefox" };
    int i;
    if (item < 0 || item >= CINIX_WM_GUI_APPS || wm->dock_app_count >= CINIX_WM_DOCK_APPS) return;
    for (i = 0; i < wm->dock_app_count; ++i) {
        if (strcmp(wm->dock_apps[i].command, commands[item]) == 0) return;
    }
    snprintf(wm->dock_apps[wm->dock_app_count].label, sizeof(wm->dock_apps[wm->dock_app_count].label), "%s", labels[item]);
    snprintf(wm->dock_apps[wm->dock_app_count].command, sizeof(wm->dock_apps[wm->dock_app_count].command), "%s", commands[item]);
    wm->dock_apps[wm->dock_app_count].icon = item;
    wm->dock_app_count++;
    cinix_wm_draw_dock(wm);
}

static int init_wm(CinixWM* wm) {
    XSetWindowAttributes attrs;
    int dock_x;
    int dock_y;

    memset(wm, 0, sizeof(*wm));
    wm->drag_index = -1;
    wm->resize_index = -1;
    wm->dock_hover = -1;
    wm->dock_pressed = -1;
    wm->dock_app_count = 1;
    snprintf(wm->dock_apps[0].label, sizeof(wm->dock_apps[0].label), "Xterm");
    snprintf(wm->dock_apps[0].command, sizeof(wm->dock_apps[0].command), "xterm");
    wm->dock_apps[0].icon = 0;
    wm->display = XOpenDisplay(NULL);
    if (wm->display == NULL) {
        fprintf(stderr, "cinixwm: failed to open X display.\n");
        return 0;
    }

    wm->screen = DefaultScreen(wm->display);
    wm->root = RootWindow(wm->display, wm->screen);
    wm->root_width = DisplayWidth(wm->display, wm->screen);
    wm->root_height = DisplayHeight(wm->display, wm->screen);

    XSetErrorHandler(cinix_wm_error_handler);
    XSelectInput(wm->display, wm->root, SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask | ExposureMask);
    XSync(wm->display, False);
    if (g_other_wm_detected) {
        fprintf(stderr, "cinixwm: another window manager is already running.\n");
        XCloseDisplay(wm->display);
        return 0;
    }

    cinix_wm_theme_init(wm);
    if (!cinix_wm_load_png("images/Tux.png", &wm->tux_pixels, &wm->tux_width, &wm->tux_height)) {
        (void)cinix_wm_load_png("images/tux.png", &wm->tux_pixels, &wm->tux_width, &wm->tux_height);
    }

    attrs.override_redirect = True;
    attrs.background_pixel = wm->theme.topbar_top;
    wm->topbar = XCreateWindow(wm->display, wm->root, 0, 0, (unsigned int)wm->root_width, CINIX_WM_TOPBAR_H, 0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect | CWBackPixel, &attrs);
    XSelectInput(wm->display, wm->topbar, ExposureMask | ButtonPressMask);

    attrs.background_pixel = wm->theme.menu_bg;
    wm->menu = XCreateWindow(wm->display, wm->root, 8, CINIX_WM_TOPBAR_H, CINIX_WM_MENU_W, CINIX_WM_MENU_ITEM_H * 4, 0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect | CWBackPixel, &attrs);
    XSelectInput(wm->display, wm->menu, ExposureMask | ButtonPressMask | PointerMotionMask);

    wm->gui_menu = XCreateWindow(wm->display, wm->root, 8 + CINIX_WM_MENU_W + 5, CINIX_WM_TOPBAR_H, 190, CINIX_WM_MENU_ITEM_H * CINIX_WM_GUI_APPS, 0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect | CWBackPixel, &attrs);
    XSelectInput(wm->display, wm->gui_menu, ExposureMask | ButtonPressMask | PointerMotionMask);

    wm->settings = XCreateWindow(wm->display, wm->root, (wm->root_width - 300) / 2, 80, 300, 182, 0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect | CWBackPixel, &attrs);
    XSelectInput(wm->display, wm->settings, ExposureMask | ButtonPressMask | PointerMotionMask);

    dock_x = (wm->root_width - CINIX_WM_DOCK_W) / 2;
    dock_y = wm->root_height - CINIX_WM_DOCK_H - 14;
    if (dock_y < CINIX_WM_TOPBAR_H + 20) dock_y = CINIX_WM_TOPBAR_H + 20;
    attrs.background_pixel = wm->theme.dock_bg;
    wm->dock = XCreateWindow(wm->display, wm->root, dock_x, dock_y, CINIX_WM_DOCK_W, CINIX_WM_DOCK_H, 0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect | CWBackPixel, &attrs);
    XSelectInput(wm->display, wm->dock, ExposureMask | ButtonPressMask | PointerMotionMask | LeaveWindowMask);

    wm->gc = XCreateGC(wm->display, wm->root, 0, NULL);
    cinix_wm_draw_desktop(wm);
    XMapRaised(wm->display, wm->dock);
    XMapRaised(wm->display, wm->topbar);
    cinix_wm_draw_dock(wm);
    cinix_wm_draw_topbar(wm);
    wm->running = 1;
    return 1;
}

static void shutdown_wm(CinixWM* wm) {
    if (wm->display == NULL) return;
    XFreeGC(wm->display, wm->gc);
    XDestroyWindow(wm->display, wm->menu);
    XDestroyWindow(wm->display, wm->gui_menu);
    XDestroyWindow(wm->display, wm->settings);
    XDestroyWindow(wm->display, wm->dock);
    XDestroyWindow(wm->display, wm->topbar);
    cinix_wm_free_image(&wm->tux_pixels);
    XCloseDisplay(wm->display);
}

static void handle_button_press(CinixWM* wm, XButtonEvent* button) {
    if (button->window == wm->topbar && button->button == Button1) {
        if (button->x >= 8 && button->x <= 84 && button->y >= 4 && button->y <= 26) {
            wm->menu_open = !wm->menu_open;
            if (wm->menu_open) {
                XMapRaised(wm->display, wm->menu);
                cinix_wm_draw_menu(wm, -1);
            } else {
                XUnmapWindow(wm->display, wm->menu);
            }
        }
    } else if (button->window == wm->menu && button->button == Button1) {
        int item = button->y / CINIX_WM_MENU_ITEM_H;
        if (item == 0) {
            wm->gui_menu_open = !wm->gui_menu_open;
            if (wm->gui_menu_open) {
                XMapRaised(wm->display, wm->gui_menu);
                cinix_wm_draw_gui_menu(wm, -1);
            } else {
                XUnmapWindow(wm->display, wm->gui_menu);
            }
        } else if (item == 1) {
            wm->settings_open = !wm->settings_open;
            if (wm->settings_open) {
                XMapRaised(wm->display, wm->settings);
                cinix_wm_draw_settings(wm, -1);
            } else {
                XUnmapWindow(wm->display, wm->settings);
            }
        } else if (item == 2) {
            launch_app(wm, 0);
            XUnmapWindow(wm->display, wm->menu);
            wm->menu_open = 0;
        } else if (item == 3) {
            wm->running = 0;
        }
    } else if (button->window == wm->gui_menu && button->button == Button1) {
        int item = button->y / CINIX_WM_MENU_ITEM_H;
        launch_gui_app(item);
    } else if (button->window == wm->settings && button->button == Button1) {
        int item = (button->y - 46) / 30;
        add_dock_app(wm, item);
    } else if (button->window == wm->dock && button->button == Button1) {
        int item = dock_item_from_x(button->x);
        wm->dock_pressed = item;
        if (item >= 0) launch_app(wm, item);
    } else {
        int index = cinix_wm_find_client(wm, button->window);
        if (index >= 0 && button->button == Button1 && button->y <= CINIX_WM_TITLE_H) {
            int close_x = 10;
            int min_x = 30;
            int max_x = 50;
            if (button->x >= close_x && button->x <= close_x + 14 && button->y >= 6 && button->y <= 20) {
                cinix_wm_close_client(wm, &wm->clients[index]);
            } else if (button->x >= min_x && button->x <= min_x + 14 && button->y >= 6 && button->y <= 20) {
                cinix_wm_minimize_client(wm, &wm->clients[index]);
            } else if (button->x >= max_x && button->x <= max_x + 14 && button->y >= 6 && button->y <= 20) {
                cinix_wm_maximize_client(wm, &wm->clients[index]);
            } else {
                wm->drag_index = index;
                wm->drag_dx = button->x_root - wm->clients[index].x;
                wm->drag_dy = button->y_root - wm->clients[index].y;
                XRaiseWindow(wm->display, wm->clients[index].frame);
                cinix_wm_raise_shell(wm);
            }
        } else if (index >= 0 && button->button == Button1 &&
                   button->x >= wm->clients[index].width - 14 &&
                   button->y >= wm->clients[index].height + CINIX_WM_TITLE_H - 14) {
            wm->resize_index = index;
            wm->resize_dx = wm->clients[index].width - button->x_root;
            wm->resize_dy = wm->clients[index].height - button->y_root;
        }
    }
}

static void handle_event(CinixWM* wm, XEvent* event) {
    if (event->type == Expose) {
        int index = cinix_wm_find_client(wm, event->xexpose.window);
        if (event->xexpose.window == wm->root) cinix_wm_draw_desktop(wm);
        else if (event->xexpose.window == wm->topbar) cinix_wm_draw_topbar(wm);
        else if (event->xexpose.window == wm->menu) cinix_wm_draw_menu(wm, -1);
        else if (event->xexpose.window == wm->gui_menu) cinix_wm_draw_gui_menu(wm, -1);
        else if (event->xexpose.window == wm->settings) cinix_wm_draw_settings(wm, -1);
        else if (event->xexpose.window == wm->dock) cinix_wm_draw_dock(wm);
        else if (index >= 0) cinix_wm_draw_frame(wm, &wm->clients[index]);
    } else if (event->type == MapRequest) {
        cinix_wm_manage(wm, event->xmaprequest.window);
    } else if (event->type == ConfigureRequest) {
        cinix_wm_handle_configure(wm, &event->xconfigurerequest);
    } else if (event->type == DestroyNotify) {
        cinix_wm_unmanage(wm, event->xdestroywindow.window);
    } else if (event->type == UnmapNotify) {
        if (event->xunmap.event != wm->root) cinix_wm_unmanage(wm, event->xunmap.window);
    } else if (event->type == PropertyNotify) {
        int index = cinix_wm_find_client(wm, event->xproperty.window);
        if (index >= 0 && event->xproperty.atom == XA_WM_NAME) {
            cinix_wm_refresh_title(wm, &wm->clients[index]);
            cinix_wm_draw_frame(wm, &wm->clients[index]);
        }
    } else if (event->type == LeaveNotify) {
        if (event->xcrossing.window == wm->dock) wm->dock_hover = -1;
    } else if (event->type == ButtonPress) {
        handle_button_press(wm, &event->xbutton);
    } else if (event->type == ButtonRelease) {
        wm->drag_index = -1;
        wm->resize_index = -1;
        wm->dock_pressed = -1;
    } else if (event->type == MotionNotify) {
        if (event->xmotion.window == wm->menu) {
            cinix_wm_draw_menu(wm, event->xmotion.y);
        } else if (event->xmotion.window == wm->gui_menu) {
            cinix_wm_draw_gui_menu(wm, event->xmotion.y);
        } else if (event->xmotion.window == wm->settings) {
            cinix_wm_draw_settings(wm, event->xmotion.y);
        } else if (event->xmotion.window == wm->dock) {
            wm->dock_hover = dock_item_from_x(event->xmotion.x);
        } else if (wm->resize_index >= 0 && wm->resize_index < wm->client_count) {
            CinixClient* client = &wm->clients[wm->resize_index];
            cinix_wm_resize_client(wm, client, event->xmotion.x_root + wm->resize_dx, event->xmotion.y_root + wm->resize_dy);
        } else if (wm->drag_index >= 0 && wm->drag_index < wm->client_count) {
            CinixClient* client = &wm->clients[wm->drag_index];
            client->x = event->xmotion.x_root - wm->drag_dx;
            client->y = event->xmotion.y_root - wm->drag_dy;
            if (client->y < CINIX_WM_TOPBAR_H) client->y = CINIX_WM_TOPBAR_H;
            XMoveWindow(wm->display, client->frame, client->x, client->y);
        }
    }
}

int main(void) {
    CinixWM wm;
    struct timespec delay;
    delay.tv_sec = 0;
    delay.tv_nsec = 16 * 1000 * 1000;

    if (!init_wm(&wm)) return 1;

    while (wm.running) {
        while (XPending(wm.display) > 0) {
            XEvent event;
            XNextEvent(wm.display, &event);
            handle_event(&wm, &event);
        }
        if (cinix_wm_update_animations(&wm)) {
            cinix_wm_draw_dock(&wm);
        }
        XFlush(wm.display);
        nanosleep(&delay, NULL);
    }

    shutdown_wm(&wm);
    return 0;
}
