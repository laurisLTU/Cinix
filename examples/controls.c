#include "controls.h"

void cinix_controls_init(CinixApp* app, CinixControls* controls) {
    controls->launch_off_x = 35;
    controls->launch_off_y = 75;
    controls->quit_off_x = 35;
    controls->quit_off_y = 126;

    controls->launch.width = 160;
    controls->launch.height = 36;
    controls->launch.label = "Launch";
    controls->launch.pressed = 0;
    controls->launch.color_idle = cinix_rgb(app, 195, 217, 255);
    controls->launch.color_hover = cinix_rgb(app, 171, 201, 255);
    controls->launch.color_pressed = cinix_rgb(app, 142, 182, 252);

    controls->quit.width = 160;
    controls->quit.height = 36;
    controls->quit.label = "Quit";
    controls->quit.pressed = 0;
    controls->quit.color_idle = cinix_rgb(app, 251, 213, 199);
    controls->quit.color_hover = cinix_rgb(app, 247, 191, 170);
    controls->quit.color_pressed = cinix_rgb(app, 240, 167, 138);
}

void cinix_controls_update_positions(CinixControls* controls, const CinixWindow* tools_window) {
    controls->launch.x = tools_window->x + controls->launch_off_x;
    controls->launch.y = tools_window->y + controls->launch_off_y;
    controls->quit.x = tools_window->x + controls->quit_off_x;
    controls->quit.y = tools_window->y + controls->quit_off_y;
}

void cinix_controls_handle_press(CinixControls* controls, int mouse_x, int mouse_y) {
    if (cinix_button_hit(&controls->launch, mouse_x, mouse_y)) {
        controls->launch.pressed = 1;
    }
    if (cinix_button_hit(&controls->quit, mouse_x, mouse_y)) {
        controls->quit.pressed = 1;
    }
}

int cinix_controls_handle_release(CinixControls* controls, int mouse_x, int mouse_y) {
    int action = 0;

    if (controls->launch.pressed && cinix_button_hit(&controls->launch, mouse_x, mouse_y)) {
        action = 1;
    }
    if (controls->quit.pressed && cinix_button_hit(&controls->quit, mouse_x, mouse_y)) {
        action = 2;
    }

    controls->launch.pressed = 0;
    controls->quit.pressed = 0;
    return action;
}

void cinix_controls_draw(CinixApp* app, const CinixControls* controls, int mouse_x, int mouse_y) {
    cinix_button_draw(app, &controls->launch, mouse_x, mouse_y);
    cinix_button_draw(app, &controls->quit, mouse_x, mouse_y);
}
