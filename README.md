# Cinix

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
- Use the Start button for the GUI Applications folder, Restart wbar, Xterm, and Exit.
- Use the scrollable GUI Applications folder for installed `.desktop` GUI apps.
- Cinix starts real external `wbar` as a Tiny Core-style bottom-center dock.
- The Start menu Restart wbar item rewrites `~/.wbar` and relaunches real `wbar`.
- Cinix writes a default `~/.wbar` from installed GUI apps that have real PNG/XPM icons.
- Xterm stays in the Start menu, but it is not forced into `wbar`, so it cannot appear as a fake dock background.
- Cinix draws liquid-glass Aqua bars, menus, and window frames.
- Cinix sets X11 opacity hints for menus, frames, and app windows; real alpha transparency needs a compositor such as `picom`.
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

## Real wbar

Cinix uses real installed `wbar`.

Build Cinix:

```bash
make
```

Arch dependencies:

```bash
sudo pacman -S --needed base-devel git libx11 libpng xterm xorg-server-xephyr
```

Install real `wbar` from GitHub using a generated local `PKGBUILD`:

```bash
make install-real-wbar
```

The installer builds [SpartanJ/wbar](https://github.com/SpartanJ/wbar) with `makepkg -si`.

On current Arch, old GTK2 configuration dependencies are unavailable, so the installer builds only the real `wbar` dock.
