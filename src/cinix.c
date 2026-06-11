#include "cinix.h"

#include <stdio.h>
#include <string.h>

#define CINIX_START_MENU_X 8
#define CINIX_START_MENU_Y CINIX_TOPBAR_H
#define CINIX_START_MENU_W 210
#define CINIX_START_MENU_ITEM_H 30
#define CINIX_START_MENU_ITEMS 5
#define CINIX_XPROG_MENU_W 170
#define CINIX_XPROG_MENU_ITEMS 3

static int bits_in_mask(unsigned long mask) {
    int bits = 0;
    while (mask != 0) {
        bits += (int)(mask & 1UL);
        mask >>= 1U;
    }
    return bits;
}

static int mask_shift(unsigned long mask) {
    int shift = 0;
    if (mask == 0) {
        return 0;
    }
    while ((mask & 1UL) == 0) {
        shift++;
        mask >>= 1U;
    }
    return shift;
}

static unsigned long channel_to_visual(unsigned char value, unsigned long mask) {
    int bits;
    int shift;
    unsigned long max_value;
    unsigned long scaled;

    if (mask == 0) {
        return 0;
    }

    bits = bits_in_mask(mask);
    shift = mask_shift(mask);
    max_value = (1UL << bits) - 1UL;
    scaled = (unsigned long)((value * max_value + 127UL) / 255UL);
    return (scaled << shift) & mask;
}

static int point_in_rect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
}

static int clampi(int value, int minv, int maxv) {
    if (value < minv) return minv;
    if (value > maxv) return maxv;
    return value;
}

static void clamp_window(CinixApp* app, CinixWindow* w) {
    int max_x = app->width - w->width;
    int max_y = app->height - w->height;
    w->x = clampi(w->x, 0, max_x < 0 ? 0 : max_x);
    w->y = clampi(w->y, CINIX_TOPBAR_H, max_y < CINIX_TOPBAR_H ? CINIX_TOPBAR_H : max_y);
}

static void cinix_hide_system_cursor(CinixApp* app) {
    static char no_data[] = { 0 };
    XColor dummy;
    Pixmap blank;
    Cursor cursor;

    dummy.red = 0;
    dummy.green = 0;
    dummy.blue = 0;
    dummy.flags = 0;

    blank = XCreateBitmapFromData(app->display, app->host, no_data, 1, 1);
    cursor = XCreatePixmapCursor(app->display, blank, blank, &dummy, &dummy, 0, 0);
    XDefineCursor(app->display, app->host, cursor);
    XFreeCursor(app->display, cursor);
    XFreePixmap(app->display, blank);
}

static void cinix_draw_title(CinixApp* app, const CinixWindow* w) {
    XSetForeground(app->display, app->gc, w->title_color);
    XFillRectangle(app->display, app->surface, app->gc, w->x, w->y, (unsigned int)w->width, (unsigned int)CINIX_TITLE_H);

    if (w->title != NULL) {
        XSetForeground(app->display, app->gc, cinix_rgb(app, 245, 245, 245));
        XDrawString(app->display, app->surface, app->gc, w->x + 10, w->y + 16, w->title, (int)strlen(w->title));
    }
}

static void cinix_draw_cursor(CinixApp* app, const CinixDesktop* desktop) {
    int x = desktop->mouse_x;
    int y = desktop->mouse_y;

    XSetForeground(app->display, app->gc, desktop->theme.cursor_outline);
    XDrawLine(app->display, app->surface, app->gc, x, y, x + 11, y + 11);
    XDrawLine(app->display, app->surface, app->gc, x, y, x, y + 16);
    XDrawLine(app->display, app->surface, app->gc, x, y, x + 9, y);
    XDrawLine(app->display, app->surface, app->gc, x + 8, y + 10, x + 13, y + 16);

    XSetForeground(app->display, app->gc, desktop->theme.cursor_fill);
    XDrawLine(app->display, app->surface, app->gc, x + 1, y + 1, x + 10, y + 10);
    XDrawLine(app->display, app->surface, app->gc, x + 1, y + 1, x + 1, y + 14);
    XDrawLine(app->display, app->surface, app->gc, x + 1, y + 1, x + 8, y + 1);
    XDrawLine(app->display, app->surface, app->gc, x + 8, y + 11, x + 12, y + 15);
}

static void cinix_draw_topbar(CinixApp* app, const CinixDesktop* desktop) {
    const int start_x = 8;
    const int start_y = 4;
    const int start_w = 72;
    const int start_h = 22;
    int hovering = point_in_rect(desktop->mouse_x, desktop->mouse_y, start_x, start_y, start_w, start_h);

    XSetForeground(app->display, app->gc, desktop->theme.topbar_bg);
    XFillRectangle(app->display, app->surface, app->gc, 0, 0, (unsigned int)app->width, CINIX_TOPBAR_H);

    XSetForeground(
        app->display,
        app->gc,
        desktop->start_pressed ? desktop->theme.start_btn_pressed : ((hovering || desktop->start_open) ? desktop->theme.start_btn_hover : desktop->theme.start_btn));
    XFillRectangle(app->display, app->surface, app->gc, start_x, start_y, start_w, start_h);

    XSetForeground(app->display, app->gc, desktop->theme.menu_border);
    XDrawRectangle(app->display, app->surface, app->gc, start_x, start_y, start_w, start_h);
    XDrawString(app->display, app->surface, app->gc, start_x + 18, start_y + 15, "Start", 5);

    XSetForeground(app->display, app->gc, desktop->theme.topbar_text);
    XDrawString(app->display, app->surface, app->gc, 102, 19, "Cinix Linux UI", 14);
}

static void cinix_draw_start_menu(CinixApp* app, const CinixDesktop* desktop) {
    const int menu_x = CINIX_START_MENU_X;
    const int menu_y = CINIX_START_MENU_Y;
    const int menu_w = CINIX_START_MENU_W;
    const int item_h = CINIX_START_MENU_ITEM_H;
    const char* items[CINIX_START_MENU_ITEMS] = { "About Cinix", "Settings", "X Programs >", "Tux", "Exit" };
    int i;

    XSetForeground(app->display, app->gc, desktop->theme.menu_bg);
    XFillRectangle(app->display, app->surface, app->gc, menu_x, menu_y, menu_w, item_h * CINIX_START_MENU_ITEMS + 4);
    XSetForeground(app->display, app->gc, desktop->theme.menu_border);
    XDrawRectangle(app->display, app->surface, app->gc, menu_x, menu_y, menu_w, item_h * CINIX_START_MENU_ITEMS + 4);

    for (i = 0; i < CINIX_START_MENU_ITEMS; ++i) {
        int iy = menu_y + 2 + i * item_h;
        int hover = point_in_rect(desktop->mouse_x, desktop->mouse_y, menu_x + 2, iy, menu_w - 4, item_h);
        if (hover) {
            XSetForeground(app->display, app->gc, desktop->theme.menu_item_hover);
            XFillRectangle(app->display, app->surface, app->gc, menu_x + 2, iy, menu_w - 4, item_h);
        }
        XSetForeground(app->display, app->gc, desktop->theme.text_main);
        XDrawString(app->display, app->surface, app->gc, menu_x + 12, iy + 20, items[i], (int)strlen(items[i]));
    }
}

static void cinix_draw_xprog_menu(CinixApp* app, const CinixDesktop* desktop) {
    const int menu_x = CINIX_START_MENU_X + CINIX_START_MENU_W + 4;
    const int menu_y = CINIX_START_MENU_Y + 2 + CINIX_START_MENU_ITEM_H * 2;
    const int menu_w = CINIX_XPROG_MENU_W;
    const int item_h = CINIX_START_MENU_ITEM_H;
    const char* items[CINIX_XPROG_MENU_ITEMS] = { "xclock", "xterm", "xeyes" };
    int i;

    XSetForeground(app->display, app->gc, desktop->theme.menu_bg);
    XFillRectangle(app->display, app->surface, app->gc, menu_x, menu_y, menu_w, item_h * CINIX_XPROG_MENU_ITEMS + 4);
    XSetForeground(app->display, app->gc, desktop->theme.menu_border);
    XDrawRectangle(app->display, app->surface, app->gc, menu_x, menu_y, menu_w, item_h * CINIX_XPROG_MENU_ITEMS + 4);

    for (i = 0; i < CINIX_XPROG_MENU_ITEMS; ++i) {
        int iy = menu_y + 2 + i * item_h;
        int hover = point_in_rect(desktop->mouse_x, desktop->mouse_y, menu_x + 2, iy, menu_w - 4, item_h);
        if (hover) {
            XSetForeground(app->display, app->gc, desktop->theme.menu_item_hover);
            XFillRectangle(app->display, app->surface, app->gc, menu_x + 2, iy, menu_w - 4, item_h);
        }
        XSetForeground(app->display, app->gc, desktop->theme.text_main);
        XDrawString(app->display, app->surface, app->gc, menu_x + 12, iy + 20, items[i], (int)strlen(items[i]));
    }
}

void cinix_handle_resize(CinixApp* app, int width, int height) {
    if (app == NULL || app->display == NULL) {
        return;
    }

    app->width = width;
    app->height = height;

    if (app->backbuffer != 0) {
        XFreePixmap(app->display, app->backbuffer);
        app->backbuffer = 0;
    }

    if (width > 0 && height > 0) {
        app->backbuffer = XCreatePixmap(
            app->display,
            app->host,
            (unsigned int)width,
            (unsigned int)height,
            (unsigned int)DefaultDepth(app->display, app->screen));
    }

    app->surface = (app->backbuffer != 0) ? app->backbuffer : app->host;
}

int cinix_init(CinixApp* app, int width, int height, const char* title) {
    if (app == NULL) {
        return 0;
    }

    memset(app, 0, sizeof(*app));

    app->display = XOpenDisplay(NULL);
    if (app->display == NULL) {
        fprintf(stderr, "Cinix: failed to open X display.\n");
        return 0;
    }

    app->screen = DefaultScreen(app->display);
    app->root = RootWindow(app->display, app->screen);
    app->width = width;
    app->height = height;

    app->host = XCreateSimpleWindow(
        app->display,
        app->root,
        80,
        80,
        (unsigned int)width,
        (unsigned int)height,
        1,
        BlackPixel(app->display, app->screen),
        WhitePixel(app->display, app->screen));

    XSelectInput(app->display, app->host, ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);

    if (title != NULL) {
        XStoreName(app->display, app->host, title);
    }

    app->wm_delete = XInternAtom(app->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(app->display, app->host, &app->wm_delete, 1);

    XMapWindow(app->display, app->host);

    app->gc = XCreateGC(app->display, app->host, 0, NULL);
    if (app->gc == 0) {
        XDestroyWindow(app->display, app->host);
        XCloseDisplay(app->display);
        return 0;
    }

    app->backbuffer = 0;
    app->surface = app->host;
    cinix_handle_resize(app, width, height);

    app->bg_color = cinix_rgb(app, 231, 235, 240);
    app->running = 1;
    return 1;
}

void cinix_shutdown(CinixApp* app) {
    if (app == NULL || app->display == NULL) {
        return;
    }

    if (app->backbuffer != 0) {
        XFreePixmap(app->display, app->backbuffer);
        app->backbuffer = 0;
    }

    XFreeGC(app->display, app->gc);
    XDestroyWindow(app->display, app->host);
    XCloseDisplay(app->display);

    app->display = NULL;
    app->running = 0;
}

void cinix_begin_frame(CinixApp* app) {
    XSetForeground(app->display, app->gc, app->bg_color);
    XFillRectangle(app->display, app->surface, app->gc, 0, 0, (unsigned int)app->width, (unsigned int)app->height);
}

void cinix_draw_window(CinixApp* app, const CinixWindow* w) {
    if (app == NULL || w == NULL) {
        return;
    }

    XSetForeground(app->display, app->gc, w->border_color);
    XFillRectangle(app->display, app->surface, app->gc, w->x - 1, w->y - 1, (unsigned int)(w->width + 2), (unsigned int)(w->height + 2));

    XSetForeground(app->display, app->gc, w->body_color);
    XFillRectangle(app->display, app->surface, app->gc, w->x, w->y, (unsigned int)w->width, (unsigned int)w->height);

    cinix_draw_title(app, w);
}

void cinix_present(CinixApp* app) {
    if (app->backbuffer != 0) {
        XCopyArea(
            app->display,
            app->backbuffer,
            app->host,
            app->gc,
            0,
            0,
            (unsigned int)app->width,
            (unsigned int)app->height,
            0,
            0);
    }
    XFlush(app->display);
}

void cinix_theme_default(CinixApp* app, CinixTheme* theme) {
    if (app == NULL || theme == NULL) {
        return;
    }

    theme->desktop_bg = cinix_rgb(app, 231, 235, 240);
    theme->window_body = cinix_rgb(app, 248, 249, 251);
    theme->window_title = cinix_rgb(app, 17, 64, 122);
    theme->window_border = cinix_rgb(app, 80, 93, 112);
    theme->panel_body = cinix_rgb(app, 242, 241, 236);
    theme->panel_title = cinix_rgb(app, 140, 72, 25);
    theme->panel_border = cinix_rgb(app, 99, 86, 68);
    theme->topbar_bg = cinix_rgb(app, 28, 40, 56);
    theme->topbar_text = cinix_rgb(app, 221, 227, 236);
    theme->start_btn = cinix_rgb(app, 204, 222, 246);
    theme->start_btn_hover = cinix_rgb(app, 182, 206, 241);
    theme->start_btn_pressed = cinix_rgb(app, 163, 191, 230);
    theme->menu_bg = cinix_rgb(app, 241, 244, 248);
    theme->menu_border = cinix_rgb(app, 25, 34, 48);
    theme->menu_item_hover = cinix_rgb(app, 212, 226, 244);
    theme->cursor_outline = cinix_rgb(app, 12, 12, 12);
    theme->cursor_fill = cinix_rgb(app, 246, 246, 246);
    theme->text_main = cinix_rgb(app, 19, 27, 38);
}

void cinix_window_apply_theme(CinixWindow* window, const CinixTheme* theme, int panel_style) {
    if (window == NULL || theme == NULL) {
        return;
    }

    if (panel_style) {
        window->body_color = theme->panel_body;
        window->title_color = theme->panel_title;
        window->border_color = theme->panel_border;
    } else {
        window->body_color = theme->window_body;
        window->title_color = theme->window_title;
        window->border_color = theme->window_border;
    }
}

void cinix_desktop_init(CinixApp* app, CinixDesktop* desktop, const CinixTheme* theme) {
    if (app == NULL || desktop == NULL) {
        return;
    }

    memset(desktop, 0, sizeof(*desktop));
    desktop->dragging_index = -1;

    if (theme != NULL) {
        desktop->theme = *theme;
    } else {
        cinix_theme_default(app, &desktop->theme);
    }

    app->bg_color = desktop->theme.desktop_bg;
    cinix_hide_system_cursor(app);
}

void cinix_desktop_handle_event(CinixApp* app, CinixDesktop* desktop, XEvent* event, CinixWindow* windows, int window_count) {
    int i;

    if (app == NULL || desktop == NULL || event == NULL) {
        return;
    }

    if (event->type == ConfigureNotify) {
        cinix_handle_resize(app, event->xconfigure.width, event->xconfigure.height);
        for (i = 0; i < window_count; ++i) {
            clamp_window(app, &windows[i]);
        }
    }

    if (event->type == MotionNotify) {
        desktop->mouse_x = event->xmotion.x;
        desktop->mouse_y = event->xmotion.y;

        if (desktop->dragging_index >= 0 && desktop->dragging_index < window_count) {
            CinixWindow* w = &windows[desktop->dragging_index];
            w->x = desktop->mouse_x - desktop->drag_dx;
            w->y = desktop->mouse_y - desktop->drag_dy;
            clamp_window(app, w);
        }
    }

    if (event->type == ButtonPress && event->xbutton.button == Button1) {
        int mx = event->xbutton.x;
        int my = event->xbutton.y;
        int over_start_btn = point_in_rect(mx, my, 8, 4, 72, 22);
        int over_menu = point_in_rect(mx, my, CINIX_START_MENU_X, CINIX_START_MENU_Y, CINIX_START_MENU_W, CINIX_START_MENU_ITEM_H * CINIX_START_MENU_ITEMS + 4);
        int over_xprog = point_in_rect(mx, my, CINIX_START_MENU_X + CINIX_START_MENU_W + 4, CINIX_START_MENU_Y + 2 + CINIX_START_MENU_ITEM_H * 2, CINIX_XPROG_MENU_W, CINIX_START_MENU_ITEM_H * CINIX_XPROG_MENU_ITEMS + 4);

        desktop->mouse_x = mx;
        desktop->mouse_y = my;

        if (over_start_btn) {
            desktop->start_pressed = 1;
        } else if (!over_menu && !over_xprog) {
            desktop->start_open = 0;
            desktop->xprog_open = 0;
            for (i = window_count - 1; i >= 0; --i) {
                if (point_in_rect(mx, my, windows[i].x, windows[i].y, windows[i].width, CINIX_TITLE_H)) {
                    desktop->dragging_index = i;
                    desktop->drag_dx = mx - windows[i].x;
                    desktop->drag_dy = my - windows[i].y;
                    break;
                }
            }
        }
    }

    if (event->type == ButtonRelease && event->xbutton.button == Button1) {
        int mx = event->xbutton.x;
        int my = event->xbutton.y;
        int over_start_btn = point_in_rect(mx, my, 8, 4, 72, 22);
        int menu_x = CINIX_START_MENU_X;
        int menu_y = CINIX_START_MENU_Y;
        int item_h = CINIX_START_MENU_ITEM_H;
        int xmenu_x = CINIX_START_MENU_X + CINIX_START_MENU_W + 4;
        int xmenu_y = CINIX_START_MENU_Y + 2 + CINIX_START_MENU_ITEM_H * 2;

        desktop->mouse_x = mx;
        desktop->mouse_y = my;

        if (desktop->start_pressed && over_start_btn) {
            desktop->start_open = !desktop->start_open;
            if (!desktop->start_open) {
                desktop->xprog_open = 0;
            }
        } else if (desktop->start_open && point_in_rect(mx, my, menu_x + 2, menu_y + 2, 186, item_h)) {
            desktop->last_action = CINIX_START_ACTION_ABOUT;
            desktop->start_open = 0;
            desktop->xprog_open = 0;
        } else if (desktop->start_open && point_in_rect(mx, my, menu_x + 2, menu_y + 2 + item_h, 186, item_h)) {
            desktop->last_action = CINIX_START_ACTION_SETTINGS;
            desktop->start_open = 0;
            desktop->xprog_open = 0;
        } else if (desktop->start_open && point_in_rect(mx, my, menu_x + 2, menu_y + 2 + item_h * 2, 186, item_h)) {
            desktop->xprog_open = !desktop->xprog_open;
        } else if (desktop->start_open && point_in_rect(mx, my, menu_x + 2, menu_y + 2 + item_h * 3, 186, item_h)) {
            desktop->last_action = CINIX_START_ACTION_TUX;
            desktop->start_open = 0;
            desktop->xprog_open = 0;
        } else if (desktop->start_open && point_in_rect(mx, my, menu_x + 2, menu_y + 2 + item_h * 4, 186, item_h)) {
            desktop->last_action = CINIX_START_ACTION_EXIT;
            desktop->start_open = 0;
            desktop->xprog_open = 0;
        } else if (desktop->start_open && desktop->xprog_open && point_in_rect(mx, my, xmenu_x + 2, xmenu_y + 2, CINIX_XPROG_MENU_W - 4, item_h)) {
            desktop->last_action = CINIX_START_ACTION_XCLOCK;
            desktop->start_open = 0;
            desktop->xprog_open = 0;
        } else if (desktop->start_open && desktop->xprog_open && point_in_rect(mx, my, xmenu_x + 2, xmenu_y + 2 + item_h, CINIX_XPROG_MENU_W - 4, item_h)) {
            desktop->last_action = CINIX_START_ACTION_XTERM;
            desktop->start_open = 0;
            desktop->xprog_open = 0;
        } else if (desktop->start_open && desktop->xprog_open && point_in_rect(mx, my, xmenu_x + 2, xmenu_y + 2 + item_h * 2, CINIX_XPROG_MENU_W - 4, item_h)) {
            desktop->last_action = CINIX_START_ACTION_XEYES;
            desktop->start_open = 0;
            desktop->xprog_open = 0;
        } else if (!over_start_btn &&
                   !point_in_rect(mx, my, CINIX_START_MENU_X, CINIX_START_MENU_Y, CINIX_START_MENU_W, CINIX_START_MENU_ITEM_H * CINIX_START_MENU_ITEMS + 4) &&
                   !point_in_rect(mx, my, xmenu_x, xmenu_y, CINIX_XPROG_MENU_W, CINIX_START_MENU_ITEM_H * CINIX_XPROG_MENU_ITEMS + 4)) {
            desktop->start_open = 0;
            desktop->xprog_open = 0;
        }

        desktop->start_pressed = 0;
        desktop->dragging_index = -1;
    }

    if (event->type == ClientMessage && (Atom)event->xclient.data.l[0] == app->wm_delete) {
        app->running = 0;
    }

    if (event->type == KeyPress) {
        app->running = 0;
    }
}

void cinix_desktop_draw_chrome(CinixApp* app, const CinixDesktop* desktop) {
    if (app == NULL || desktop == NULL) {
        return;
    }

    cinix_draw_topbar(app, desktop);
    if (desktop->start_open) {
        cinix_draw_start_menu(app, desktop);
        if (desktop->xprog_open) {
            cinix_draw_xprog_menu(app, desktop);
        }
    }
    cinix_draw_cursor(app, desktop);
}

int cinix_desktop_consume_action(CinixDesktop* desktop) {
    int action;

    if (desktop == NULL) {
        return CINIX_START_ACTION_NONE;
    }

    action = desktop->last_action;
    desktop->last_action = CINIX_START_ACTION_NONE;
    return action;
}

unsigned long cinix_rgb(CinixApp* app, unsigned char r, unsigned char g, unsigned char b) {
    Visual* visual;

    if (app == NULL || app->display == NULL) {
        return 0;
    }

    visual = DefaultVisual(app->display, app->screen);
    return channel_to_visual(r, visual->red_mask) |
           channel_to_visual(g, visual->green_mask) |
           channel_to_visual(b, visual->blue_mask);
}
