#define _POSIX_C_SOURCE 200809L

#include "cinix_wm.h"

#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int g_other_wm_detected = 0;
static int g_wbar_position_ticks = 0;

static const char* liquid_xterm_command(void) {
    return "xterm -bg '#001b2d' -fg '#e7fbff' -cr '#ffffff' -fa Monospace -fs 11";
}

static void set_shell_opacity(CinixWM* wm, Window window, double opacity) {
    Atom opacity_atom = XInternAtom(wm->display, "_NET_WM_WINDOW_OPACITY", False);
    unsigned long value;
    if (opacity < 0.15) opacity = 0.15;
    if (opacity > 1.0) opacity = 1.0;
    value = (unsigned long)(0xffffffffUL * opacity);
    XChangeProperty(wm->display, window, opacity_atom, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)&value, 1);
}

static int cinix_wm_error_handler(Display* display, XErrorEvent* error) {
    (void)display;
    if (error->error_code == BadAccess) g_other_wm_detected = 1;
    return 0;
}

static void copy_text(char* dest, size_t dest_size, const char* src) {
    if (dest_size == 0) return;
    if (src == NULL) src = "";
    snprintf(dest, dest_size, "%.*s", (int)dest_size - 1, src);
}

static void clean_desktop_exec(char* text) {
    char cleaned[256];
    size_t i = 0;
    size_t o = 0;
    while (text[i] != '\0' && o + 1 < sizeof(cleaned)) {
        if (text[i] == '%') {
            i++;
            if (text[i] != '\0') i++;
            continue;
        }
        cleaned[o++] = text[i++];
    }
    cleaned[o] = '\0';
    while (o > 0 && (cleaned[o - 1] == ' ' || cleaned[o - 1] == '\t')) {
        cleaned[--o] = '\0';
    }
    copy_text(text, 256, cleaned);
}

static int resolve_icon_path(const char* icon_name, char* out, size_t out_size) {
    const char* dirs[] = {
        "/usr/share/pixmaps",
        "/usr/share/icons/hicolor/48x48/apps",
        "/usr/share/icons/hicolor/64x64/apps",
        "/usr/share/icons/hicolor/32x32/apps",
        "/usr/share/icons/Adwaita/48x48/legacy",
        "/usr/share/icons/Adwaita/48x48/apps",
        NULL
    };
    const char* exts[] = { "", ".png", ".xpm", NULL };
    int d;
    int e;
    char candidate[512];

    if (out_size == 0) return 0;
    out[0] = '\0';
    if (icon_name == NULL || icon_name[0] == '\0') return 0;

    if (strchr(icon_name, '/') != NULL) {
        if (access(icon_name, R_OK) == 0) {
            copy_text(out, out_size, icon_name);
            return 1;
        }
        return 0;
    }

    for (d = 0; dirs[d] != NULL; ++d) {
        for (e = 0; exts[e] != NULL; ++e) {
            snprintf(candidate, sizeof(candidate), "%s/%s%s", dirs[d], icon_name, exts[e]);
            if (access(candidate, R_OK) == 0) {
                copy_text(out, out_size, candidate);
                return 1;
            }
        }
    }
    return 0;
}

static int add_gui_app(CinixWM* wm, const char* label, const char* command, const char* icon) {
    int i;
    if (wm->gui_app_count >= CINIX_WM_MAX_GUI_APPS) return 0;
    if (label == NULL || label[0] == '\0' || command == NULL || command[0] == '\0') return 0;
    for (i = 0; i < wm->gui_app_count; ++i) {
        if (strcmp(wm->gui_apps[i].label, label) == 0) return 0;
    }
    copy_text(wm->gui_apps[wm->gui_app_count].label, sizeof(wm->gui_apps[wm->gui_app_count].label), label);
    copy_text(wm->gui_apps[wm->gui_app_count].command, sizeof(wm->gui_apps[wm->gui_app_count].command), command);
    copy_text(wm->gui_apps[wm->gui_app_count].icon, sizeof(wm->gui_apps[wm->gui_app_count].icon), icon);
    wm->gui_app_count++;
    return 1;
}

static void load_gui_apps(CinixWM* wm) {
    FILE* pipe;
    char line[768];
    wm->gui_app_count = 0;

    {
        char xterm_icon[256];
        resolve_icon_path("xterm-color_48x48", xterm_icon, sizeof(xterm_icon));
        add_gui_app(wm, "Xterm", liquid_xterm_command(), xterm_icon);
    }

    pipe = popen("find /usr/share/applications \"$HOME/.local/share/applications\" -name '*.desktop' -type f 2>/dev/null | sort", "r");
    if (pipe == NULL) {
        return;
    }

    while (fgets(line, sizeof(line), pipe) != NULL && wm->gui_app_count < CINIX_WM_MAX_GUI_APPS) {
        char path[512];
        char name[128] = "";
        char exec_cmd[256] = "";
        char icon_name[256] = "";
        char icon_path[256] = "";
        int nodisplay = 0;
        int hidden = 0;
        int terminal = 0;
        FILE* fp;

        line[strcspn(line, "\r\n")] = '\0';
        copy_text(path, sizeof(path), line);
        fp = fopen(path, "r");
        if (fp == NULL) continue;

        while (fgets(line, sizeof(line), fp) != NULL) {
            line[strcspn(line, "\r\n")] = '\0';
            if (strncmp(line, "Name=", 5) == 0 && name[0] == '\0') {
                copy_text(name, sizeof(name), line + 5);
            } else if (strncmp(line, "Exec=", 5) == 0 && exec_cmd[0] == '\0') {
                copy_text(exec_cmd, sizeof(exec_cmd), line + 5);
                clean_desktop_exec(exec_cmd);
            } else if (strncmp(line, "Icon=", 5) == 0 && icon_name[0] == '\0') {
                copy_text(icon_name, sizeof(icon_name), line + 5);
            } else if (strcmp(line, "NoDisplay=true") == 0) {
                nodisplay = 1;
            } else if (strcmp(line, "Hidden=true") == 0) {
                hidden = 1;
            } else if (strcmp(line, "Terminal=true") == 0) {
                terminal = 1;
            }
        }
        fclose(fp);

        resolve_icon_path(icon_name, icon_path, sizeof(icon_path));
        if (!nodisplay && !hidden && !terminal) {
            add_gui_app(wm, name, exec_cmd, icon_path);
        }
    }
    pclose(pipe);
}

static void launch_app(CinixWM* wm, int app) {
    char cmdline[384];
    if (app < 0 || app >= wm->dock_app_count) return;
    snprintf(cmdline, sizeof(cmdline), "%s >/dev/null 2>&1 &", wm->dock_apps[app].command);
    wm->dock_launch[app] = 1.0;
    (void)system(cmdline);
}

static void write_wbar_entry(FILE* fp, const char* icon, const char* command, const char* title) {
    fprintf(fp, "i: %s\n", icon);
    fprintf(fp, "c: %s\n", command);
    fprintf(fp, "t: %s\n\n", title);
}

static void write_default_wbar_config(CinixWM* wm) {
    const char* home = getenv("HOME");
    char path[512];
    FILE* fp;
    int i;
    int written = 0;

    if (home == NULL || home[0] == '\0') return;
    snprintf(path, sizeof(path), "%s/.wbar", home);
    fp = fopen(path, "w");
    if (fp == NULL) return;

    for (i = 0; i < wm->gui_app_count && written < 10; ++i) {
        if (strcmp(wm->gui_apps[i].label, "Xterm") == 0) continue;
        if (wm->gui_apps[i].icon[0] == '\0') continue;
        write_wbar_entry(fp, wm->gui_apps[i].icon, wm->gui_apps[i].command, wm->gui_apps[i].label);
        written++;
    }
    fclose(fp);
}

static int text_has_wbar(const char* text) {
    return text != NULL && (strstr(text, "wbar") != NULL || strstr(text, "Wbar") != NULL || strstr(text, "WBar") != NULL);
}

static int is_wbar_window(CinixWM* wm, Window window) {
    char* name = NULL;
    XClassHint class_hint;
    int matched = 0;

    memset(&class_hint, 0, sizeof(class_hint));
    if (XFetchName(wm->display, window, &name) && name != NULL) {
        matched = text_has_wbar(name);
        XFree(name);
    }

    if (!matched && XGetClassHint(wm->display, window, &class_hint)) {
        matched = text_has_wbar(class_hint.res_name) || text_has_wbar(class_hint.res_class);
        if (class_hint.res_name != NULL) XFree(class_hint.res_name);
        if (class_hint.res_class != NULL) XFree(class_hint.res_class);
    }

    return matched;
}

static int looks_like_wbar_window(CinixWM* wm, Window window, const XWindowAttributes* attrs) {
    if (window == wm->topbar || window == wm->menu || window == wm->gui_menu) return 0;
    if (!attrs->override_redirect) return 0;
    if (attrs->width < 70 || attrs->width > 520) return 0;
    if (attrs->height < 24 || attrs->height > 130) return 0;
    return 1;
}

static int position_real_wbar(CinixWM* wm) {
    Window root;
    Window parent;
    Window* children = NULL;
    unsigned int child_count = 0;
    unsigned int i;
    int positioned = 0;

    if (!XQueryTree(wm->display, wm->root, &root, &parent, &children, &child_count)) return 0;

    for (i = 0; i < child_count; ++i) {
        XWindowAttributes attrs;
        if (!XGetWindowAttributes(wm->display, children[i], &attrs)) continue;
        if (attrs.map_state != IsViewable) continue;
        if (!is_wbar_window(wm, children[i]) && !looks_like_wbar_window(wm, children[i], &attrs)) continue;

        {
            int x = (wm->root_width - attrs.width) / 2;
            int y = wm->root_height - attrs.height - 8;
            if (x < 0) x = 0;
            if (y < CINIX_WM_TOPBAR_H) y = CINIX_WM_TOPBAR_H;
            XMoveWindow(wm->display, children[i], x, y);
            XRaiseWindow(wm->display, children[i]);
            positioned = 1;
        }
    }

    if (children != NULL) XFree(children);
    return positioned;
}

static void start_real_wbar(CinixWM* wm) {
    write_default_wbar_config(wm);
    (void)system("pkill -x wbar >/dev/null 2>&1; if command -v wbar >/dev/null 2>&1; then wbar -above-desk -pos bottom -offset 0 -isize 40 -idist 56 -zoomf 1.6 -jumpf 1.0 -balfa 0 -falfa 100 >/dev/null 2>&1 & else xmessage 'Install real wbar: make install-real-wbar' >/dev/null 2>&1 & fi");
    g_wbar_position_ticks = 180;
}

static void close_menus(CinixWM* wm) {
    XUnmapWindow(wm->display, wm->menu);
    XUnmapWindow(wm->display, wm->gui_menu);
    wm->menu_open = 0;
    wm->gui_menu_open = 0;
}

static void launch_gui_app(CinixWM* wm, int item) {
    char cmdline[384];
    if (item < 0 || item >= wm->gui_app_count) return;
    snprintf(cmdline, sizeof(cmdline), "%s >/dev/null 2>&1 &", wm->gui_apps[item].command);
    (void)system(cmdline);
}

static int gui_menu_visible_items(CinixWM* wm) {
    int max_visible = (wm->root_height - CINIX_WM_TOPBAR_H - 16) / CINIX_WM_MENU_ITEM_H;
    if (max_visible < 1) max_visible = 1;
    if (max_visible > 14) max_visible = 14;
    if (wm->gui_app_count > 0 && max_visible > wm->gui_app_count) max_visible = wm->gui_app_count;
    return max_visible;
}

static int init_wm(CinixWM* wm) {
    XSetWindowAttributes attrs;

    memset(wm, 0, sizeof(*wm));
    wm->drag_index = -1;
    wm->resize_index = -1;
    wm->dock_hover = -1;
    wm->dock_pressed = -1;
    wm->gui_menu_scroll = 0;
    wm->dock_app_count = 1;
    snprintf(wm->dock_apps[0].label, sizeof(wm->dock_apps[0].label), "Xterm");
    snprintf(wm->dock_apps[0].command, sizeof(wm->dock_apps[0].command), "%s", liquid_xterm_command());
    wm->dock_apps[0].icon = 0;
    load_gui_apps(wm);
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
    set_shell_opacity(wm, wm->topbar, 0.78);
    XSelectInput(wm->display, wm->topbar, ExposureMask | ButtonPressMask);

    attrs.background_pixel = wm->theme.menu_bg;
    wm->menu = XCreateWindow(wm->display, wm->root, 8, CINIX_WM_TOPBAR_H, CINIX_WM_MENU_W, CINIX_WM_MENU_ITEM_H * 4, 0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect | CWBackPixel, &attrs);
    set_shell_opacity(wm, wm->menu, 0.84);
    XSelectInput(wm->display, wm->menu, ExposureMask | ButtonPressMask | PointerMotionMask);

    wm->gui_menu = XCreateWindow(wm->display, wm->root, 8 + CINIX_WM_MENU_W + 5, CINIX_WM_TOPBAR_H, 230, CINIX_WM_MENU_ITEM_H * gui_menu_visible_items(wm), 0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect | CWBackPixel, &attrs);
    set_shell_opacity(wm, wm->gui_menu, 0.84);
    XSelectInput(wm->display, wm->gui_menu, ExposureMask | ButtonPressMask | PointerMotionMask);

    wm->gc = XCreateGC(wm->display, wm->root, 0, NULL);
    cinix_wm_draw_desktop(wm);
    XMapRaised(wm->display, wm->topbar);
    cinix_wm_draw_topbar(wm);
    start_real_wbar(wm);
    wm->running = 1;
    return 1;
}

static void shutdown_wm(CinixWM* wm) {
    if (wm->display == NULL) return;
    XFreeGC(wm->display, wm->gc);
    XDestroyWindow(wm->display, wm->menu);
    XDestroyWindow(wm->display, wm->gui_menu);
    XDestroyWindow(wm->display, wm->topbar);
    cinix_wm_free_image(&wm->tux_pixels);
    (void)system("pkill -x wbar >/dev/null 2>&1");
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
                close_menus(wm);
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
            start_real_wbar(wm);
            close_menus(wm);
        } else if (item == 2) {
            launch_app(wm, 0);
            close_menus(wm);
        } else if (item == 3) {
            wm->running = 0;
        }
    } else if (button->window == wm->gui_menu && button->button == Button1) {
        int item = wm->gui_menu_scroll + button->y / CINIX_WM_MENU_ITEM_H;
        launch_gui_app(wm, item);
        close_menus(wm);
    } else if (button->window == wm->gui_menu && (button->button == Button4 || button->button == Button5)) {
        int visible = gui_menu_visible_items(wm);
        int max_scroll = wm->gui_app_count - visible;
        if (max_scroll < 0) max_scroll = 0;
        if (button->button == Button4 && wm->gui_menu_scroll > 0) wm->gui_menu_scroll--;
        if (button->button == Button5 && wm->gui_menu_scroll < max_scroll) wm->gui_menu_scroll++;
        cinix_wm_draw_gui_menu(wm, -1);
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
        if (g_wbar_position_ticks > 0) {
            (void)position_real_wbar(&wm);
            g_wbar_position_ticks--;
        }
        XFlush(wm.display);
        nanosleep(&delay, NULL);
    }

    shutdown_wm(&wm);
    return 0;
}
