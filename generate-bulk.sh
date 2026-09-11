#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGES_DIR="${REPO_DIR}/packages"

echo -e "\033[34m[INFO]\033[0m Limpiando paquetes antiguos (preservando fortune)..."
find "$PACKAGES_DIR" -mindepth 1 -maxdepth 1 -type d ! -name 'fortune' -exec rm -rf {} +

create_recipe() {
    local name="$1"
    local version="$2"
    local url="$3"
    local build_type="$4"
    local deps="${5:-""}"
    local desc="${6:-"Paquete $name"}"
    local is_lib="${7:-false}"

    local target_dir="${PACKAGES_DIR}/$name"
    if [ "$is_lib" = "true" ]; then
        target_dir="${PACKAGES_DIR}/lib/$name"
    fi

    mkdir -p "$target_dir"
    local recipe_file="$target_dir/$name.recipe"
    local build_steps=""

    case "$build_type" in
        "autotools"|"c")
            build_steps='./configure --prefix=/usr
make -j$(nproc)
make DESTDIR="$DESTDIR" install'
            ;;
        "cmake")
            build_steps='cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
DESTDIR="$DESTDIR" cmake --install build'
            ;;
        "meson")
            build_steps='meson setup build --prefix=/usr
ninja -C build
DESTDIR="$DESTDIR" ninja -C build install'
            ;;
        "makefile"|"make")
            build_steps='make -j$(nproc)
make DESTDIR="$DESTDIR" install'
            ;;
        "meta")
            build_steps='mkdir -p "$DESTDIR/usr/share/doc/'"$name"'"
echo "Meta-package '"$name"' v'"$version"' installed" > "$DESTDIR/usr/share/doc/'"$name"'/README"'
            ;;
        "script")
            build_steps='mkdir -p "$DESTDIR/usr/bin"
if [ -f Makefile ]; then
    make PREFIX="$DESTDIR/usr" install || make DESTDIR="$DESTDIR" install
else
    cp -f '"$name"' "$DESTDIR/usr/bin/" 2>/dev/null || true
fi'
            ;;
        *)
            build_steps="$build_type"
            ;;
    esac

    cat <<EOF > "$recipe_file"
NAME="$name"
VERSION="$version"
URL="$url"
DESCRIPTION="$desc"
DEPENDENCIES="$deps"
BUILD_STEPS='$build_steps'
EOF

    echo -e "\033[32m[OK]\033[0m Creada receta: \033[1m${recipe_file#$REPO_DIR/}\033[0m"
}

echo -e "\033[34m[INFO]\033[0m Generando todas las recetas limpias..."

# --- BASE Y SISTEMA ---
create_recipe "base" "1.0.0" "" "meta" "bash coreutils busybox" "Clover Linux Base System metapackage"
create_recipe "build-essentials" "1.0.0" "" "meta" "gcc make" "Essential tools for compiling software"
create_recipe "bash" "5.2.21" "https://ftp.gnu.org/gnu/bash/bash-5.2.21.tar.gz" "autotools" "" "The GNU Bourne Again Shell"
create_recipe "busybox" "1.36.1" "https://busybox.net/downloads/busybox-1.36.1.tar.bz2" "make" "" "The Swiss Army Knife of Embedded Linux"
create_recipe "coreutils" "9.5" "https://ftp.gnu.org/gnu/coreutils/coreutils-9.5.tar.xz" "autotools" "" "GNU core utilities"
create_recipe "e2fsprogs" "1.47.0" "https://kernel.org/pub/linux/kernel/people/tytso/e2fsprogs/v1.47.0/e2fsprogs-1.47.0.tar.gz" "autotools" "" "Ext2/3/4 filesystem utilities"
create_recipe "gcc" "13.2.0" "https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz" "autotools" "make" "GNU Compiler Collection"
create_recipe "kmod" "32" "https://kernel.org/pub/linux/utils/kernel/kmod/kmod-32.tar.xz" "autotools" "" "Kernel module utilities"
create_recipe "make" "4.4.1" "https://ftp.gnu.org/gnu/make/make-4.4.1.tar.gz" "autotools" "" "GNU Make build tool"
create_recipe "systemd" "255" "https://github.com/systemd/systemd/archive/refs/tags/v255.tar.gz" "meson" "dbus coreutils" "System and Service Manager"

# --- APLICACIONES Y NAVEGADORES ---
create_recipe "cmatrix" "2.0" "https://github.com/abishekvashok/cmatrix/archive/refs/heads/master.tar.gz" "autotools" "ncurses" "Matrix terminal effect"
create_recipe "cowsay" "3.04" "https://github.com/tnalpgm/cowsay/archive/refs/tags/cowsay-3.04.tar.gz" "script" "" "Configurable speaking cow"
create_recipe "fastfetch" "2.21.0" "https://github.com/fastfetch-cli/fastfetch/archive/refs/tags/2.21.0.tar.gz" "cmake" "" "System information tool"
create_recipe "htop" "3.3.0" "https://github.com/htop-dev/htop/archive/refs/tags/3.3.0.tar.gz" "autotools" "ncurses" "Interactive process viewer"
create_recipe "nano" "8.0" "https://github.com/nano-editor/nano/archive/refs/tags/v8.0.tar.gz" "autotools" "ncurses" "GNU nano text editor"
create_recipe "firefox" "124.0.2" "https://ftp.mozilla.org/pub/firefox/releases/124.0.2/source/firefox-124.0.2.source.tar.xz" "autotools" "gtk3 nss" "Mozilla Firefox Web Browser"
create_recipe "chromium" "123.0.6312.105" "https://commondatastorage.googleapis.com/chromium-browser-official/chromium-123.0.6312.105.tar.xz" "autotools" "gtk3 alsa" "Chromium Web Browser"
create_recipe "brave" "1.64.113" "https://github.com/brave/brave-browser/archive/refs/tags/v1.64.113.tar.gz" "script" "gtk3 nss" "Brave Web Browser"

# --- LIBRERÍAS ---
create_recipe "zlib" "1.3.1" "https://zlib.net/zlib-1.3.1.tar.gz" "autotools" "" "Compression library" "true"
create_recipe "ncurses" "6.4" "https://ftp.gnu.org/gnu/ncurses/ncurses-6.4.tar.gz" "autotools" "" "Terminal display library" "true"
create_recipe "openssl" "3.2.1" "https://www.openssl.org/source/openssl-3.2.1.tar.gz" "autotools" "" "TLS/SSL cryptography library" "true"

echo -e "\033[32m[ÉXITO]\033[0m Recetas re-generadas correctamente con sintaxis limpia."
