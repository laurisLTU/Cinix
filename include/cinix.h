#ifndef CINIX_H
#define CINIX_H

#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int x;
    int y;
    int width;
    int height;
    const char* title;
    unsigned long body_color;
    unsigned long title_color;
    unsigned long border_color;
} CinixWindow;

typedef struct {
    Display* display;
    int screen;
    Window root;
    Window host;
    Drawable surface;
    Pixmap backbuffer;
    GC gc;
    int running;
    int width;
    int height;
    unsigned long bg_color;
    Atom wm_delete;
} CinixApp;

typedef struct {
    unsigned long desktop_bg;
    unsigned long window_body;
    unsigned long window_title;
    unsigned long window_border;
    unsigned long panel_body;
    unsigned long panel_title;
    unsigned long panel_border;
    unsigned long topbar_bg;
    unsigned long topbar_text;
    unsigned long start_btn;
    unsigned long start_btn_hover;
    unsigned long start_btn_pressed;
    unsigned long menu_bg;
    unsigned long menu_border;
    unsigned long menu_item_hover;
    unsigned long cursor_outline;
    unsigned long cursor_fill;
    unsigned long text_main;
} CinixTheme;

typedef struct {
    int mouse_x;
    int mouse_y;
    int start_open;
    int xprog_open;
    int start_pressed;
    int dragging_index;
    int drag_dx;
    int drag_dy;
    int last_action;
    CinixTheme theme;
} CinixDesktop;

#define CINIX_TITLE_H 24
#define CINIX_TOPBAR_H 30

#define CINIX_START_ACTION_NONE 0
#define CINIX_START_ACTION_ABOUT 1
#define CINIX_START_ACTION_SETTINGS 2
#define CINIX_START_ACTION_TUX 3
#define CINIX_START_ACTION_EXIT 4
#define CINIX_START_ACTION_XCLOCK 5
#define CINIX_START_ACTION_XTERM 6
#define CINIX_START_ACTION_XEYES 7

int cinix_init(CinixApp* app, int width, int height, const char* title);
void cinix_shutdown(CinixApp* app);
void cinix_begin_frame(CinixApp* app);
void cinix_draw_window(CinixApp* app, const CinixWindow* w);
void cinix_present(CinixApp* app);
void cinix_handle_resize(CinixApp* app, int width, int height);
void cinix_theme_default(CinixApp* app, CinixTheme* theme);
void cinix_window_apply_theme(CinixWindow* window, const CinixTheme* theme, int panel_style);
void cinix_desktop_init(CinixApp* app, CinixDesktop* desktop, const CinixTheme* theme);
void cinix_desktop_handle_event(CinixApp* app, CinixDesktop* desktop, XEvent* event, CinixWindow* windows, int window_count);
void cinix_desktop_draw_chrome(CinixApp* app, const CinixDesktop* desktop);
int cinix_desktop_consume_action(CinixDesktop* desktop);

unsigned long cinix_rgb(CinixApp* app, unsigned char r, unsigned char g, unsigned char b);

#ifdef __cplusplus
}
#endif

#endif
