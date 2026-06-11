#include "cinix_wm.h"

#include <stdlib.h>
#include <string.h>

int cinix_wm_find_client(CinixWM* wm, Window window) {
    int i;
    for (i = 0; i < wm->client_count; ++i) {
        if (wm->clients[i].client == window || wm->clients[i].frame == window) return i;
    }
    return -1;
}

void cinix_wm_refresh_title(CinixWM* wm, CinixClient* client) {
    char* name = NULL;
    strncpy(client->title, "X11 Window", sizeof(client->title) - 1);
    client->title[sizeof(client->title) - 1] = '\0';
    if (XFetchName(wm->display, client->client, &name) && name != NULL) {
        strncpy(client->title, name, sizeof(client->title) - 1);
        client->title[sizeof(client->title) - 1] = '\0';
        XFree(name);
    }
}

void cinix_wm_manage(CinixWM* wm, Window window) {
    XWindowAttributes attrs;
    CinixClient* client;

    if (wm->client_count >= CINIX_WM_MAX_CLIENTS || cinix_wm_find_client(wm, window) >= 0) return;
    if (!XGetWindowAttributes(wm->display, window, &attrs) || attrs.override_redirect) return;

    client = &wm->clients[wm->client_count++];
    memset(client, 0, sizeof(*client));
    client->client = window;
    client->x = attrs.x;
    client->y = attrs.y < CINIX_WM_TOPBAR_H ? CINIX_WM_TOPBAR_H + 12 : attrs.y;
    client->width = attrs.width < 120 ? 120 : attrs.width;
    client->height = attrs.height < 60 ? 60 : attrs.height;
    client->saved_x = client->x;
    client->saved_y = client->y;
    client->saved_width = client->width;
    client->saved_height = client->height;
    cinix_wm_refresh_title(wm, client);

    client->frame = XCreateSimpleWindow(wm->display, wm->root, client->x, client->y, (unsigned int)(client->width + 2), (unsigned int)(client->height + CINIX_WM_TITLE_H + 2), 0, wm->theme.border, wm->theme.menu_bg);
    XSelectInput(wm->display, client->frame, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | SubstructureNotifyMask);
    XSelectInput(wm->display, client->client, PropertyChangeMask | StructureNotifyMask);
    XAddToSaveSet(wm->display, client->client);
    XReparentWindow(wm->display, client->client, client->frame, 1, CINIX_WM_TITLE_H + 1);
    XMapWindow(wm->display, client->frame);
    XMapWindow(wm->display, client->client);
    cinix_wm_raise_shell(wm);
    cinix_wm_draw_frame(wm, client);
}

void cinix_wm_minimize_client(CinixWM* wm, CinixClient* client) {
    if (client == NULL) return;
    client->minimized = !client->minimized;
    if (client->minimized) {
        XMoveResizeWindow(wm->display, client->client, 1, CINIX_WM_TITLE_H + 1, 1, 1);
        XResizeWindow(wm->display, client->frame, (unsigned int)(client->width + 2), CINIX_WM_TITLE_H + 2);
    } else {
        XResizeWindow(wm->display, client->frame, (unsigned int)(client->width + 2), (unsigned int)(client->height + CINIX_WM_TITLE_H + 2));
        XMoveResizeWindow(wm->display, client->client, 1, CINIX_WM_TITLE_H + 1, (unsigned int)client->width, (unsigned int)client->height);
    }
    cinix_wm_draw_frame(wm, client);
}

void cinix_wm_maximize_client(CinixWM* wm, CinixClient* client) {
    if (client == NULL) return;
    if (!client->maximized) {
        client->saved_x = client->x;
        client->saved_y = client->y;
        client->saved_width = client->width;
        client->saved_height = client->height;
        client->x = 8;
        client->y = CINIX_WM_TOPBAR_H + 8;
        client->width = wm->root_width - 16;
        client->height = wm->root_height - CINIX_WM_TOPBAR_H - CINIX_WM_DOCK_H - 36;
        client->maximized = 1;
        client->minimized = 0;
        XMapWindow(wm->display, client->client);
    } else {
        client->x = client->saved_x;
        client->y = client->saved_y;
        client->width = client->saved_width;
        client->height = client->saved_height;
        client->maximized = 0;
    }
    if (client->height < 80) client->height = 80;
    XMoveResizeWindow(wm->display, client->frame, client->x, client->y, (unsigned int)(client->width + 2), (unsigned int)(client->height + CINIX_WM_TITLE_H + 2));
    XMoveResizeWindow(wm->display, client->client, 1, CINIX_WM_TITLE_H + 1, (unsigned int)client->width, (unsigned int)client->height);
    cinix_wm_draw_frame(wm, client);
}

void cinix_wm_resize_client(CinixWM* wm, CinixClient* client, int width, int height) {
    if (client == NULL || client->minimized) return;
    client->width = width < 160 ? 160 : width;
    client->height = height < 80 ? 80 : height;
    client->maximized = 0;
    XMoveResizeWindow(wm->display, client->frame, client->x, client->y, (unsigned int)(client->width + 2), (unsigned int)(client->height + CINIX_WM_TITLE_H + 2));
    XMoveResizeWindow(wm->display, client->client, 1, CINIX_WM_TITLE_H + 1, (unsigned int)client->width, (unsigned int)client->height);
    cinix_wm_draw_frame(wm, client);
}

void cinix_wm_close_client(CinixWM* wm, CinixClient* client) {
    Atom wm_delete = XInternAtom(wm->display, "WM_DELETE_WINDOW", False);
    Atom wm_protocols = XInternAtom(wm->display, "WM_PROTOCOLS", False);
    Atom* protocols = NULL;
    int protocol_count = 0;
    int supports_delete = 0;
    int i;

    if (XGetWMProtocols(wm->display, client->client, &protocols, &protocol_count)) {
        for (i = 0; i < protocol_count; ++i) {
            if (protocols[i] == wm_delete) supports_delete = 1;
        }
        XFree(protocols);
    }

    if (supports_delete) {
        XEvent message;
        memset(&message, 0, sizeof(message));
        message.xclient.type = ClientMessage;
        message.xclient.window = client->client;
        message.xclient.message_type = wm_protocols;
        message.xclient.format = 32;
        message.xclient.data.l[0] = (long)wm_delete;
        message.xclient.data.l[1] = CurrentTime;
        XSendEvent(wm->display, client->client, False, NoEventMask, &message);
    } else {
        XKillClient(wm->display, client->client);
    }
}

void cinix_wm_unmanage(CinixWM* wm, Window window) {
    int index = cinix_wm_find_client(wm, window);
    if (index < 0) return;

    XReparentWindow(wm->display, wm->clients[index].client, wm->root, wm->clients[index].x, wm->clients[index].y);
    XDestroyWindow(wm->display, wm->clients[index].frame);
    if (index < wm->client_count - 1) {
        memmove(&wm->clients[index], &wm->clients[index + 1], (size_t)(wm->client_count - index - 1) * sizeof(wm->clients[0]));
    }
    wm->client_count--;
    cinix_wm_draw_desktop(wm);
    cinix_wm_raise_shell(wm);
}

void cinix_wm_handle_configure(CinixWM* wm, XConfigureRequestEvent* request) {
    XWindowChanges changes;
    int index = cinix_wm_find_client(wm, request->window);
    changes.x = request->x;
    changes.y = request->y;
    changes.width = request->width;
    changes.height = request->height;
    changes.border_width = request->border_width;
    changes.sibling = request->above;
    changes.stack_mode = request->detail;

    if (index < 0) {
        XConfigureWindow(wm->display, request->window, (unsigned int)request->value_mask, &changes);
        return;
    }

    if (request->value_mask & CWX) wm->clients[index].x = request->x;
    if (request->value_mask & CWY) wm->clients[index].y = request->y;
    if (request->value_mask & CWWidth) wm->clients[index].width = request->width < 120 ? 120 : request->width;
    if (request->value_mask & CWHeight) wm->clients[index].height = request->height < 60 ? 60 : request->height;
    wm->clients[index].minimized = 0;
    XMoveResizeWindow(wm->display, wm->clients[index].frame, wm->clients[index].x, wm->clients[index].y, (unsigned int)(wm->clients[index].width + 2), (unsigned int)(wm->clients[index].height + CINIX_WM_TITLE_H + 2));
    XMoveResizeWindow(wm->display, wm->clients[index].client, 1, CINIX_WM_TITLE_H + 1, (unsigned int)wm->clients[index].width, (unsigned int)wm->clients[index].height);
}
