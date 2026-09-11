#!/usr/bin/env bash
set -euo pipefail

create_recipe() {
    local name="$1"
    local version="$2"
    local url="$3"
    local build_type="$4"
    local deps="${5:-""}"
    local desc="${6:-"Paquete $name"}"

    mkdir -p "packages/$name"
    local recipe_file="packages/$name/$name.recipe"

    local build_steps=""

    case "$build_type" in
        "autotools"|"c")
            build_steps="./configure --prefix=/usr
make -j\$(nproc)
make DESTDIR=\"\$DESTDIR\" install"
            ;;
        "cmake")
            build_steps="cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
DESTDIR=\"\$DESTDIR\" cmake --install build"
            ;;
        "makefile"|"make")
            build_steps="make -j\$(nproc)
make DESTDIR=\"\$DESTDIR\" install"
            ;;
        "meta")
            build_steps="mkdir -p \"\$DESTDIR/usr/share/doc/$name\"
echo \"Meta-package $name v$version installed\" > \"\$DESTDIR/usr/share/doc/$name/README\""
            ;;
        "script")
            build_steps="mkdir -p \"\$DESTDIR/usr/bin\"
if [ -f Makefile ]; then
    make PREFIX=\"\$DESTDIR/usr\" install || make DESTDIR=\"\$DESTDIR\" install
else
    cp -f $name \"\$DESTDIR/usr/bin/\" 2>/dev/null || true
fi"
            ;;
        *)
            build_steps="$build_type"
            ;;
    esac

    cat <<EOF > "$recipe_file"
NAME="$name"
VERSION="$version"
SOURCE_URL="$url"
DESCRIPTION="$desc"
DEPENDENCIES="$deps"
BUILD_STEPS="$build_steps"
EOF

    echo -e "\033[32m[OK]\033[0m Creada receta: \033[1m$recipe_file\033[0m"
}

echo -e "\033[34m[INFO]\033[0m Generando recetas estandarizadas..."

# --- PAQUETES BASE Y SISTEMA ---
create_recipe "base" "1.0.0" "" "meta" "bash coreutils busybox" "Clover Linux Base System metapackage"
create_recipe "build-essentials" "1.0.0" "" "meta" "gcc make" "Essential tools for compiling software"
create_recipe "bash" "5.2.21" "https://ftp.gnu.org/gnu/bash/bash-5.2.21.tar.gz" "autotools" "" "The GNU Bourne Again Shell"
create_recipe "busybox" "1.36.1" "https://busybox.net/downloads/busybox-1.36.1.tar.bz2" "make" "" "The Swiss Army Knife of Embedded Linux"
create_recipe "coreutils" "9.5" "https://ftp.gnu.org/gnu/coreutils/coreutils-9.5.tar.xz" "autotools" "" "GNU core utilities (ls, cp, mv, etc)"
create_recipe "e2fsprogs" "1.47.0" "https://kernel.org/pub/linux/kernel/people/tytso/e2fsprogs/v1.47.0/e2fsprogs-1.47.0.tar.gz" "autotools" "" "Ext2/3/4 filesystem utilities"
create_recipe "gcc" "13.2.0" "https://ftp.gnu.org/gnu/gcc/gcc-13.2.0/gcc-13.2.0.tar.xz" "autotools" "make" "GNU Compiler Collection"
create_recipe "kmod" "32" "https://kernel.org/pub/linux/utils/kernel/kmod/kmod-32.tar.xz" "autotools" "" "Tools for loading and managing Linux kernel modules"
create_recipe "make" "4.4.1" "https://ftp.gnu.org/gnu/make/make-4.4.1.tar.gz" "autotools" "" "GNU Make build tool"

# --- HERRAMIENTAS Y APLICACIONES ---
create_recipe "cmatrix" "2.0" "https://github.com/abishekvashok/cmatrix/archive/refs/heads/master.tar.gz" "autotools" "ncurses" "Matrix terminal effect"
create_recipe "cowsay" "3.04" "https://github.com/tnalpgm/cowsay/archive/refs/tags/cowsay-3.04.tar.gz" "script" "" "Configurable speaking cow"
create_recipe "fastfetch" "2.21.0" "https://github.com/fastfetch-cli/fastfetch/archive/refs/tags/2.21.0.tar.gz" "cmake" "" "Like neofetch, but faster"
create_recipe "htop" "3.3.0" "https://github.com/htop-dev/htop/archive/refs/tags/3.3.0.tar.gz" "autotools" "ncurses" "Interactive process viewer"
create_recipe "nano" "8.0" "https://github.com/nano-editor/nano/archive/refs/tags/v8.0.tar.gz" "autotools" "ncurses" "GNU nano text editor"

echo -e "\033[32m[ÉXITO]\033[0m Todas las recetas fueron regeneradas."
