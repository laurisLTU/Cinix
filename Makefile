CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11 -Iinclude
LDFLAGS ?= -lX11 -lpng

DEMO_SRC = src/cinix.c src/cinix_buttons.c src/cinix_png.c \
      examples/main.c examples/controls.c examples/tux.c examples/about.c
WM_SRC = src/cinix_wm_main.c src/cinix_wm_draw.c src/cinix_wm_image.c src/cinix_wm_clients.c

all: cinix_demo cinixwm

cinix_demo: $(DEMO_SRC)
	$(CC) $(CFLAGS) $(DEMO_SRC) -o cinix_demo $(LDFLAGS)

cinixwm: $(WM_SRC)
	$(CC) $(CFLAGS) $(WM_SRC) -o cinixwm -lX11 -lpng

clean:
	rm -f cinix_demo cinixwm

install-real-wbar:
	bash scripts/install-real-wbar-makepkg.sh

.PHONY: all clean install-real-wbar
