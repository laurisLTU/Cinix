#ifndef CINIX_WM_H
#define CINIX_WM_H

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <stdint.h>

#define CINIX_WM_MAX_CLIENTS 128
#define CINIX_WM_TOPBAR_H 30
#define CINIX_WM_TITLE_H 26
#define CINIX_WM_MENU_W 210
#define CINIX_WM_MENU_ITEM_H 30
#define CINIX_WM_DOCK_W 720
#define CINIX_WM_DOCK_H 76
#define CINIX_WM_DOCK_APPS 8
#define CINIX_WM_GUI_APPS 4

typedef struct {
    Window client;
    Window frame;
    int x;
    int y;
    int width;
    int height;
    int saved_x;
    int saved_y;
    int saved_width;
    int saved_height;
    int minimized;
    int maximized;
    char title[128];
} CinixClient;

typedef struct {
    char label[32];
    char command[128];
    int icon;
} CinixDockApp;

typedef struct {
    unsigned long desktop_top;
    unsigned long desktop_bottom;
    unsigned long topbar_top;
    unsigned long topbar_bottom;
    unsigned long topbar_text;
    unsigned long glass_light;
    unsigned long glass_mid;
    unsigned long glass_dark;
    unsigned long menu_bg;
    unsigned long menu_hover;
    unsigned long border;
    unsigned long title_text;
    unsigned long close_red;
    unsigned long min_yellow;
    unsigned long zoom_green;
    unsigned long dock_bg;
    unsigned long dock_edge;
    unsigned long shadow;
} CinixWMTheme;

typedef struct {
    Display* display;
    int screen;
    Window root;
    Window topbar;
    Window menu;
    Window gui_menu;
    Window settings;
    Window dock;
    GC gc;
    int running;
    int menu_open;
    int gui_menu_open;
    int settings_open;
    int drag_index;
    int drag_dx;
    int drag_dy;
    int resize_index;
    int resize_dx;
    int resize_dy;
    int root_width;
    int root_height;
    int dock_hover;
    int dock_pressed;
    double dock_anim[CINIX_WM_DOCK_APPS];
    double dock_launch[CINIX_WM_DOCK_APPS];
    int dock_app_count;
    CinixDockApp dock_apps[CINIX_WM_DOCK_APPS];
    CinixWMTheme theme;
    uint32_t* tux_pixels;
    int tux_width;
    int tux_height;
    CinixClient clients[CINIX_WM_MAX_CLIENTS];
    int client_count;
} CinixWM;

unsigned long cinix_wm_rgb(CinixWM* wm, unsigned char r, unsigned char g, unsigned char b);
unsigned long cinix_wm_color(CinixWM* wm, const char* name, unsigned long fallback);
void cinix_wm_theme_init(CinixWM* wm);

int cinix_wm_find_client(CinixWM* wm, Window window);
void cinix_wm_manage(CinixWM* wm, Window window);
void cinix_wm_unmanage(CinixWM* wm, Window window);
void cinix_wm_close_client(CinixWM* wm, CinixClient* client);
void cinix_wm_minimize_client(CinixWM* wm, CinixClient* client);
void cinix_wm_maximize_client(CinixWM* wm, CinixClient* client);
void cinix_wm_resize_client(CinixWM* wm, CinixClient* client, int width, int height);
void cinix_wm_handle_configure(CinixWM* wm, XConfigureRequestEvent* request);
void cinix_wm_refresh_title(CinixWM* wm, CinixClient* client);

int cinix_wm_load_png(const char* path, uint32_t** out_pixels, int* out_w, int* out_h);
void cinix_wm_free_image(uint32_t** pixels);

void cinix_wm_draw_desktop(CinixWM* wm);
void cinix_wm_draw_topbar(CinixWM* wm);
void cinix_wm_draw_menu(CinixWM* wm, int mouse_y);
void cinix_wm_draw_gui_menu(CinixWM* wm, int mouse_y);
void cinix_wm_draw_settings(CinixWM* wm, int mouse_y);
void cinix_wm_draw_dock(CinixWM* wm);
void cinix_wm_draw_frame(CinixWM* wm, CinixClient* client);
void cinix_wm_raise_shell(CinixWM* wm);
int cinix_wm_update_animations(CinixWM* wm);

#endif
