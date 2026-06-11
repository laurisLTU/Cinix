#include "cinix_wm.h"

#include <math.h>
#include <string.h>

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
    if (mask == 0) return 0;
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

    if (mask == 0) return 0;
    bits = bits_in_mask(mask);
    shift = mask_shift(mask);
    max_value = (1UL << bits) - 1UL;
    scaled = (unsigned long)((value * max_value + 127UL) / 255UL);
    return (scaled << shift) & mask;
}

unsigned long cinix_wm_rgb(CinixWM* wm, unsigned char r, unsigned char g, unsigned char b) {
    Visual* visual = DefaultVisual(wm->display, wm->screen);
    return channel_to_visual(r, visual->red_mask) |
           channel_to_visual(g, visual->green_mask) |
           channel_to_visual(b, visual->blue_mask);
}

unsigned long cinix_wm_color(CinixWM* wm, const char* name, unsigned long fallback) {
    XColor color;
    XColor exact;
    Colormap colormap = DefaultColormap(wm->display, wm->screen);
    if (XAllocNamedColor(wm->display, colormap, name, &color, &exact)) return color.pixel;
    return fallback;
}

void cinix_wm_theme_init(CinixWM* wm) {
    wm->theme.desktop_top = cinix_wm_color(wm, "#5cbfff", WhitePixel(wm->display, wm->screen));
    wm->theme.desktop_bottom = cinix_wm_color(wm, "#e9fbff", WhitePixel(wm->display, wm->screen));
    wm->theme.topbar_top = cinix_wm_color(wm, "#ffffff", WhitePixel(wm->display, wm->screen));
    wm->theme.topbar_bottom = cinix_wm_color(wm, "#8fd7ff", WhitePixel(wm->display, wm->screen));
    wm->theme.topbar_text = cinix_wm_color(wm, "#07345b", BlackPixel(wm->display, wm->screen));
    wm->theme.glass_light = cinix_wm_color(wm, "#f7ffff", WhitePixel(wm->display, wm->screen));
    wm->theme.glass_mid = cinix_wm_color(wm, "#76d9ff", WhitePixel(wm->display, wm->screen));
    wm->theme.glass_dark = cinix_wm_color(wm, "#1274c5", BlackPixel(wm->display, wm->screen));
    wm->theme.menu_bg = cinix_wm_color(wm, "#def7ff", WhitePixel(wm->display, wm->screen));
    wm->theme.menu_hover = cinix_wm_color(wm, "#f3ffff", WhitePixel(wm->display, wm->screen));
    wm->theme.border = cinix_wm_color(wm, "#2a80b8", BlackPixel(wm->display, wm->screen));
    wm->theme.title_text = cinix_wm_color(wm, "#032943", BlackPixel(wm->display, wm->screen));
    wm->theme.close_red = cinix_wm_color(wm, "#ff6159", WhitePixel(wm->display, wm->screen));
    wm->theme.min_yellow = cinix_wm_color(wm, "#ffbd2e", WhitePixel(wm->display, wm->screen));
    wm->theme.zoom_green = cinix_wm_color(wm, "#28c840", WhitePixel(wm->display, wm->screen));
    wm->theme.dock_bg = cinix_wm_color(wm, "#d8edf9", WhitePixel(wm->display, wm->screen));
    wm->theme.dock_edge = cinix_wm_color(wm, "#ffffff", WhitePixel(wm->display, wm->screen));
    wm->theme.shadow = cinix_wm_color(wm, "#36546b", BlackPixel(wm->display, wm->screen));
}

static void fill_vgradient(CinixWM* wm, Drawable d, int x, int y, int w, int h, unsigned long top, unsigned long bottom) {
    int i;
    XColor c1;
    XColor c2;
    Colormap colormap = DefaultColormap(wm->display, wm->screen);
    c1.pixel = top;
    c2.pixel = bottom;
    XQueryColor(wm->display, colormap, &c1);
    XQueryColor(wm->display, colormap, &c2);

    for (i = 0; i < h; ++i) {
        unsigned char r = (unsigned char)(((c1.red >> 8) * (h - i) + (c2.red >> 8) * i) / h);
        unsigned char g = (unsigned char)(((c1.green >> 8) * (h - i) + (c2.green >> 8) * i) / h);
        unsigned char b = (unsigned char)(((c1.blue >> 8) * (h - i) + (c2.blue >> 8) * i) / h);
        XSetForeground(wm->display, wm->gc, cinix_wm_rgb(wm, r, g, b));
        XDrawLine(wm->display, d, wm->gc, x, y + i, x + w, y + i);
    }
}

static void draw_glass_panel(CinixWM* wm, Drawable d, int x, int y, int w, int h, int shine_h) {
    fill_vgradient(wm, d, x, y, w, h, wm->theme.glass_light, wm->theme.glass_mid);
    fill_vgradient(wm, d, x, y + h / 2, w, h - h / 2, wm->theme.glass_mid, wm->theme.topbar_bottom);
    XSetForeground(wm->display, wm->gc, wm->theme.dock_edge);
    XFillRectangle(wm->display, d, wm->gc, x + 2, y + 2, w - 4, shine_h);
    XSetForeground(wm->display, wm->gc, wm->theme.glass_light);
    XDrawLine(wm->display, d, wm->gc, x + 3, y + shine_h + 3, x + w - 4, y + 4);
    XSetForeground(wm->display, wm->gc, wm->theme.border);
    XDrawRectangle(wm->display, d, wm->gc, x, y, w - 1, h - 1);
}

static void draw_liquid_bubbles(CinixWM* wm, Drawable d, int w, int h) {
    XSetForeground(wm->display, wm->gc, cinix_wm_rgb(wm, 238, 255, 255));
    XFillArc(wm->display, d, wm->gc, w - 92, 5, 52, h + 8, 0, 360 * 64);
    XFillArc(wm->display, d, wm->gc, w - 42, 2, 34, h + 12, 0, 360 * 64);
    XSetForeground(wm->display, wm->gc, wm->theme.glass_mid);
    XDrawArc(wm->display, d, wm->gc, w - 92, 5, 52, h + 8, 0, 360 * 64);
    XDrawArc(wm->display, d, wm->gc, w - 42, 2, 34, h + 12, 0, 360 * 64);
}

static void draw_tux(CinixWM* wm) {
    int max_dim = 170;
    int dst_w;
    int dst_h;
    int dst_x;
    int dst_y;
    int x;
    int y;

    if (wm->tux_pixels == NULL || wm->tux_width <= 0 || wm->tux_height <= 0) return;
    if (wm->tux_width >= wm->tux_height) {
        dst_w = max_dim;
        dst_h = (wm->tux_height * max_dim) / wm->tux_width;
    } else {
        dst_h = max_dim;
        dst_w = (wm->tux_width * max_dim) / wm->tux_height;
    }
    dst_x = (wm->root_width - dst_w) / 2;
    dst_y = (wm->root_height - dst_h) / 2 - 24;
    if (dst_y < CINIX_WM_TOPBAR_H + 20) dst_y = CINIX_WM_TOPBAR_H + 20;

    for (y = 0; y < dst_h; ++y) {
        int sy = (y * wm->tux_height) / dst_h;
        for (x = 0; x < dst_w; ++x) {
            int sx = (x * wm->tux_width) / dst_w;
            uint32_t px = wm->tux_pixels[sy * wm->tux_width + sx];
            unsigned char a = (unsigned char)((px >> 24) & 0xFF);
            if (a > 0) {
                unsigned char r = (unsigned char)((px >> 16) & 0xFF);
                unsigned char g = (unsigned char)((px >> 8) & 0xFF);
                unsigned char b = (unsigned char)(px & 0xFF);
                XSetForeground(wm->display, wm->gc, cinix_wm_rgb(wm, r, g, b));
                XDrawPoint(wm->display, wm->root, wm->gc, dst_x + x, dst_y + y);
            }
        }
    }
}

void cinix_wm_draw_desktop(CinixWM* wm) {
    int i;
    fill_vgradient(wm, wm->root, 0, CINIX_WM_TOPBAR_H, wm->root_width, wm->root_height - CINIX_WM_TOPBAR_H, wm->theme.desktop_top, wm->theme.desktop_bottom);
    XSetForeground(wm->display, wm->gc, cinix_wm_rgb(wm, 255, 255, 255));
    for (i = 0; i < 7; ++i) {
        int x = 44 + i * 138;
        int y = CINIX_WM_TOPBAR_H + 38 + (i % 3) * 78;
        XDrawArc(wm->display, wm->root, wm->gc, x, y, 72, 32, 0, 360 * 64);
    }
    draw_tux(wm);
}

void cinix_wm_draw_topbar(CinixWM* wm) {
    draw_glass_panel(wm, wm->topbar, 0, 0, wm->root_width, CINIX_WM_TOPBAR_H, 11);
    draw_liquid_bubbles(wm, wm->topbar, wm->root_width, CINIX_WM_TOPBAR_H);
    XSetForeground(wm->display, wm->gc, wm->theme.glass_light);
    XFillArc(wm->display, wm->topbar, wm->gc, 8, 4, 76, 22, 0, 360 * 64);
    XSetForeground(wm->display, wm->gc, wm->theme.dock_edge);
    XFillArc(wm->display, wm->topbar, wm->gc, 14, 6, 64, 8, 0, 360 * 64);
    XSetForeground(wm->display, wm->gc, wm->theme.border);
    XDrawArc(wm->display, wm->topbar, wm->gc, 8, 4, 76, 22, 0, 360 * 64);
    XSetForeground(wm->display, wm->gc, wm->theme.topbar_text);
    XDrawString(wm->display, wm->topbar, wm->gc, 27, 19, "Start", 5);
    XDrawString(wm->display, wm->topbar, wm->gc, 112, 19, "Cinix Liquid Glass WM", 21);
}

void cinix_wm_draw_menu(CinixWM* wm, int mouse_y) {
    const char* items[] = { "GUI Applications >", "Restart wbar", "Xterm", "Exit Cinix" };
    int i;
    draw_glass_panel(wm, wm->menu, 0, 0, CINIX_WM_MENU_W, CINIX_WM_MENU_ITEM_H * 4, 24);
    XSetForeground(wm->display, wm->gc, wm->theme.border);
    XDrawRectangle(wm->display, wm->menu, wm->gc, 0, 0, CINIX_WM_MENU_W - 1, CINIX_WM_MENU_ITEM_H * 4 - 1);
    for (i = 0; i < 4; ++i) {
        int y = i * CINIX_WM_MENU_ITEM_H;
        if (mouse_y >= y && mouse_y < y + CINIX_WM_MENU_ITEM_H) {
            XSetForeground(wm->display, wm->gc, wm->theme.menu_hover);
            XFillArc(wm->display, wm->menu, wm->gc, 7, y + 4, CINIX_WM_MENU_W - 14, CINIX_WM_MENU_ITEM_H - 8, 0, 360 * 64);
            XSetForeground(wm->display, wm->gc, wm->theme.glass_mid);
            XDrawArc(wm->display, wm->menu, wm->gc, 7, y + 4, CINIX_WM_MENU_W - 14, CINIX_WM_MENU_ITEM_H - 8, 0, 360 * 64);
        }
        XSetForeground(wm->display, wm->gc, wm->theme.title_text);
        XDrawString(wm->display, wm->menu, wm->gc, 12, y + 20, items[i], (int)strlen(items[i]));
    }
}

void cinix_wm_draw_gui_menu(CinixWM* wm, int mouse_y) {
    int i;
    int max_visible = (wm->root_height - CINIX_WM_TOPBAR_H - 16) / CINIX_WM_MENU_ITEM_H;
    int items;
    int width = 230;
    if (max_visible < 1) max_visible = 1;
    if (max_visible > 14) max_visible = 14;
    items = wm->gui_app_count > 0 ? wm->gui_app_count : 1;
    if (items > max_visible) items = max_visible;
    draw_glass_panel(wm, wm->gui_menu, 0, 0, width, CINIX_WM_MENU_ITEM_H * items, 24);
    XSetForeground(wm->display, wm->gc, wm->theme.border);
    XDrawRectangle(wm->display, wm->gui_menu, wm->gc, 0, 0, width - 1, CINIX_WM_MENU_ITEM_H * items - 1);
    if (wm->gui_app_count <= 0) {
        XSetForeground(wm->display, wm->gc, wm->theme.title_text);
        XDrawString(wm->display, wm->gui_menu, wm->gc, 38, 20, "No GUI apps found", 17);
        return;
    }
    for (i = 0; i < items; ++i) {
        int app_index = wm->gui_menu_scroll + i;
        int y = i * CINIX_WM_MENU_ITEM_H;
        if (app_index >= wm->gui_app_count) break;
        if (mouse_y >= y && mouse_y < y + CINIX_WM_MENU_ITEM_H) {
            XSetForeground(wm->display, wm->gc, wm->theme.menu_hover);
            XFillArc(wm->display, wm->gui_menu, wm->gc, 6, y + 4, width - 12, CINIX_WM_MENU_ITEM_H - 8, 0, 360 * 64);
            XSetForeground(wm->display, wm->gc, wm->theme.glass_mid);
            XDrawArc(wm->display, wm->gui_menu, wm->gc, 6, y + 4, width - 12, CINIX_WM_MENU_ITEM_H - 8, 0, 360 * 64);
        }
        XSetForeground(wm->display, wm->gc, wm->theme.glass_mid);
        XFillArc(wm->display, wm->gui_menu, wm->gc, 12, y + 7, 16, 16, 0, 360 * 64);
        XSetForeground(wm->display, wm->gc, wm->theme.title_text);
        XDrawString(wm->display, wm->gui_menu, wm->gc, 38, y + 20, wm->gui_apps[app_index].label, (int)strlen(wm->gui_apps[app_index].label));
    }
    if (wm->gui_app_count > items) {
        XSetForeground(wm->display, wm->gc, wm->theme.title_text);
        if (wm->gui_menu_scroll > 0) {
            XDrawString(wm->display, wm->gui_menu, wm->gc, width - 20, 18, "^", 1);
        }
        if (wm->gui_menu_scroll + items < wm->gui_app_count) {
            XDrawString(wm->display, wm->gui_menu, wm->gc, width - 20, CINIX_WM_MENU_ITEM_H * items - 8, "v", 1);
        }
    }
}

static void draw_dock_icon(CinixWM* wm, const CinixDockApp* app, int cx, int base_y, int size) {
    int x = cx - size / 2;
    int y = base_y - size;
    unsigned long colors[] = { wm->theme.glass_mid, wm->theme.min_yellow, wm->theme.zoom_green, wm->theme.close_red };
    unsigned long color = colors[app->icon % 4];

    XSetForeground(wm->display, wm->gc, wm->theme.shadow);
    XFillArc(wm->display, wm->dock, wm->gc, x + 4, y + 6, size, size, 0, 360 * 64);
    XSetForeground(wm->display, wm->gc, color);
    XFillArc(wm->display, wm->dock, wm->gc, x, y, size, size, 0, 360 * 64);
    XSetForeground(wm->display, wm->gc, wm->theme.dock_edge);
    XFillArc(wm->display, wm->dock, wm->gc, x + size / 5, y + size / 7, size / 2, size / 4, 20 * 64, 160 * 64);
    XSetForeground(wm->display, wm->gc, wm->theme.border);
    XDrawArc(wm->display, wm->dock, wm->gc, x, y, size, size, 0, 360 * 64);
    if (app->icon == 0) {
        int sx = x + size / 4;
        int sy = y + size / 3;
        int sw = size / 2;
        int sh = size / 3;
        XSetForeground(wm->display, wm->gc, wm->theme.title_text);
        XFillRectangle(wm->display, wm->dock, wm->gc, sx, sy, sw, sh);
        XSetForeground(wm->display, wm->gc, wm->theme.zoom_green);
        XDrawString(wm->display, wm->dock, wm->gc, sx + 4, sy + sh - 5, ">", 1);
    }
    XSetForeground(wm->display, wm->gc, wm->theme.title_text);
    XDrawString(wm->display, wm->dock, wm->gc, cx - 20, CINIX_WM_DOCK_H - 9, app->label, (int)strlen(app->label));
}

void cinix_wm_draw_dock(CinixWM* wm) {
    int i;
    XSetForeground(wm->display, wm->gc, cinix_wm_rgb(wm, 0, 0, 0));
    XFillRectangle(wm->display, wm->dock, wm->gc, 0, 0, CINIX_WM_DOCK_W, CINIX_WM_DOCK_H);
    fill_vgradient(wm, wm->dock, 0, 12, CINIX_WM_DOCK_W, CINIX_WM_DOCK_H - 12, wm->theme.dock_edge, wm->theme.dock_bg);
    XSetForeground(wm->display, wm->gc, wm->theme.border);
    XDrawArc(wm->display, wm->dock, wm->gc, 0, -2, CINIX_WM_DOCK_W - 1, CINIX_WM_DOCK_H + 20, 0, 360 * 64);
    for (i = 0; i < wm->dock_app_count; ++i) {
        int pulse = (int)(8.0 * wm->dock_launch[i]);
        int size = 42 + (int)(16.0 * wm->dock_anim[i]) + pulse;
        int y = CINIX_WM_DOCK_H - 17 - pulse / 2;
        int center = 52 + i * 82;
        draw_dock_icon(wm, &wm->dock_apps[i], center, y, size);
    }
}

void cinix_wm_draw_frame(CinixWM* wm, CinixClient* client) {
    XSetForeground(wm->display, wm->gc, cinix_wm_rgb(wm, 70, 115, 150));
    XDrawRectangle(wm->display, client->frame, wm->gc, 3, 3, client->width + 1, client->height + CINIX_WM_TITLE_H + 1);
    XSetForeground(wm->display, wm->gc, wm->theme.menu_bg);
    XFillRectangle(wm->display, client->frame, wm->gc, 1, CINIX_WM_TITLE_H, client->width, client->height + 1);
    XSetForeground(wm->display, wm->gc, wm->theme.border);
    XDrawRectangle(wm->display, client->frame, wm->gc, 0, 0, client->width + 1, client->height + CINIX_WM_TITLE_H + 1);
    draw_glass_panel(wm, client->frame, 1, 1, client->width, CINIX_WM_TITLE_H, 10);
    draw_liquid_bubbles(wm, client->frame, client->width, CINIX_WM_TITLE_H);
    XSetForeground(wm->display, wm->gc, wm->theme.close_red);
    XFillArc(wm->display, client->frame, wm->gc, 10, 6, 14, 14, 0, 360 * 64);
    XSetForeground(wm->display, wm->gc, wm->theme.min_yellow);
    XFillArc(wm->display, client->frame, wm->gc, 30, 6, 14, 14, 0, 360 * 64);
    XSetForeground(wm->display, wm->gc, wm->theme.zoom_green);
    XFillArc(wm->display, client->frame, wm->gc, 50, 6, 14, 14, 0, 360 * 64);
    XSetForeground(wm->display, wm->gc, wm->theme.title_text);
    XDrawString(wm->display, client->frame, wm->gc, 76, 18, client->title, (int)strlen(client->title));
    XDrawLine(wm->display, client->frame, wm->gc, client->width - 15, client->height + CINIX_WM_TITLE_H - 4, client->width - 4, client->height + CINIX_WM_TITLE_H - 15);
    XDrawLine(wm->display, client->frame, wm->gc, client->width - 10, client->height + CINIX_WM_TITLE_H - 4, client->width - 4, client->height + CINIX_WM_TITLE_H - 10);
}

void cinix_wm_raise_shell(CinixWM* wm) {
    if (wm->dock != 0) {
        XRaiseWindow(wm->display, wm->dock);
    }
    XRaiseWindow(wm->display, wm->topbar);
}

int cinix_wm_update_animations(CinixWM* wm) {
    int i;
    int dirty = 0;
    for (i = 0; i < wm->dock_app_count; ++i) {
        double target = (wm->dock_hover == i) ? 1.0 : 0.0;
        double old_anim = wm->dock_anim[i];
        double old_launch = wm->dock_launch[i];
        wm->dock_anim[i] += (target - wm->dock_anim[i]) * 0.22;
        if (wm->dock_launch[i] > 0.0) {
            wm->dock_launch[i] -= 0.055;
            if (wm->dock_launch[i] < 0.0) wm->dock_launch[i] = 0.0;
        }
        if ((old_anim - wm->dock_anim[i] > 0.002) || (wm->dock_anim[i] - old_anim > 0.002) ||
            (old_launch - wm->dock_launch[i] > 0.002) || (wm->dock_launch[i] - old_launch > 0.002)) {
            dirty = 1;
        }
    }
    return dirty;
}
