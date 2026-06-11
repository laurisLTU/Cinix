# Cinix (Latest Version: Alpha 1.0)
<img width="1280" height="797" alt="image" src="https://github.com/user-attachments/assets/383aa4f8-4914-47eb-933b-4ee9aed9ad0d" />

Cinix is a C/X11 GUI and window manager project.

## Builds

```bash
make
```

This creates:
- `cinix_demo`: the normal Cinix demo app that runs inside any X11 desktop
- `cinixwm`: the Cinix Aqua-style X11 window manager

## Run The Demo

```bash
./cinix_demo
```

## Run Cinix As The X11 Window Manager

Use it from a clean X session where no other window manager is running:

```bash
startx ./cinixwm
```

Inside `cinixwm`:
- Use the Start button for the GUI Applications folder, Dock Settings, Xterm, and Exit.
- Use the GUI Applications folder for app launchers with icons.
- Use Cinix Dock Settings to add GUI apps into the dock.
- The dock starts with Xterm only and uses a drawn terminal-style icon.
- Drag real X11 app windows by their Cinix title bar.
- Use the Aqua-style red/yellow/green buttons to close, minimize/restore, and maximize/restore windows.
- Resize windows by dragging the bottom-right resize grip.
- The desktop draws `images/Tux.png` over a glossy blue desktop background.
- WM code is split across `cinix_wm_main.c`, `cinix_wm_draw.c`, `cinix_wm_clients.c`, and `cinix_wm_image.c`.

## Dependencies

On Debian/Ubuntu:

```bash
sudo apt-get install -y build-essential libx11-dev libpng-dev xinit xterm x11-apps
```

On Arch Linux:

```bash
sudo pacman -S base-devel libx11 libpng xorg-xinit xterm xorg-xclock xorg-xeyes xorg-server-xephyr
```
