#!/usr/bin/env bash
set -euo pipefail

WORKDIR="${WORKDIR:-$HOME/cinix-makepkg/wbar}"
WBAR_REPO="${WBAR_REPO:-https://github.com/SpartanJ/wbar.git}"

if [ "$(id -u)" -eq 0 ]; then
  echo "Do not run this as root. makepkg must run as a normal user." >&2
  exit 1
fi

command -v git >/dev/null 2>&1 || { echo "Missing git. Install: sudo pacman -S git" >&2; exit 1; }
command -v makepkg >/dev/null 2>&1 || { echo "Missing makepkg. Install: sudo pacman -S base-devel" >&2; exit 1; }

mkdir -p "$WORKDIR"
cd "$WORKDIR"

cat > PKGBUILD <<EOF
pkgname=wbar-git
pkgver=git
pkgrel=1
pkgdesc="Wbar quick launch bar built from GitHub"
arch=('x86_64')
url='https://github.com/SpartanJ/wbar'
license=('GPL3')
depends=('imlib2' 'libx11')
makedepends=('git' 'autoconf' 'automake' 'libtool' 'intltool' 'pkgconf')
provides=('wbar')
conflicts=('wbar')
source=("\$pkgname::git+$WBAR_REPO")
sha256sums=('SKIP')

pkgver() {
  cd "\$pkgname"
  git rev-list --count HEAD
}

prepare() {
  cd "\$pkgname"
  autoreconf -fi
}

build() {
  cd "\$pkgname"
  ./configure --prefix=/usr --exec_prefix=/usr --sysconfdir=/etc --disable-wbar-config
  make
}

package() {
  cd "\$pkgname"
  make DESTDIR="\$pkgdir" install
}
EOF

echo "[Cinix] Building real wbar from $WBAR_REPO"
makepkg -si

echo "[Cinix] Installed:"
command -v wbar || true
echo "[Cinix] Built real wbar only; the old GTK2 config GUI is not installed."
