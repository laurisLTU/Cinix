#ifndef CINIX_CONTROLS_H
#define CINIX_CONTROLS_H

#include "cinix.h"
#include "cinix_buttons.h"

typedef struct {
    CinixButton launch;
    CinixButton quit;
    int launch_off_x;
    int launch_off_y;
    int quit_off_x;
    int quit_off_y;
} CinixControls;

void cinix_controls_init(CinixApp* app, CinixControls* controls);
void cinix_controls_update_positions(CinixControls* controls, const CinixWindow* tools_window);
void cinix_controls_handle_press(CinixControls* controls, int mouse_x, int mouse_y);
int cinix_controls_handle_release(CinixControls* controls, int mouse_x, int mouse_y);
void cinix_controls_draw(CinixApp* app, const CinixControls* controls, int mouse_x, int mouse_y);

#endif
