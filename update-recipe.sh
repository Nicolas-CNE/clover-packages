#!/usr/bin/env bash
set -e

# Colores para output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

get_github_latest() {
    local repo="$1"
    curl -s "https://api.github.com/repos/${repo}/releases/latest" | grep '"tag_name":' | sed -E 's/.*"v?([^"]+)".*/\1/'
}

get_gnu_latest() {
    local pkg="$1"
    curl -s "https://ftp.gnu.org/gnu/${pkg}/" | grep -oP "${pkg}-\d+\.\d+(\.\d+)?\.tar\.(gz|xz|bz2)" | tail -1 | sed -E "s/${pkg}-(.*)\.tar\.(gz|xz|bz2)/\1/"
}

update_recipe() {
    local pkg="$1"
    local new_ver="$2"
    local recipe_file="packages/${pkg}/${pkg}.recipe"

    [ ! -f "$recipe_file" ] && recipe_file="${pkg}/${pkg}.recipe"
    [ ! -f "$recipe_file" ] && return

    local current_ver=$(grep '^VERSION=' "$recipe_file" | cut -d'=' -f2 | tr -d '"')

    if [ "$current_ver" != "$new_ver" ] && [ -n "$new_ver" ]; then
        echo -e "${BLUE}[UPDATE]${NC} $pkg: $current_ver -> $new_ver"
        
        # Reemplazar VERSION y actualizar las URLs que tengan la versión vieja hardcodeada
        sed -i "s/^VERSION=.*/VERSION=$new_ver/" "$recipe_file"
        sed -i "s/${current_ver}/${new_ver}/g" "$recipe_file"
    else
        echo -e "${GREEN}[OK]${NC} $pkg ($current_ver) está al día."
    fi
}

echo "=== Actualizando recetas de clover-packages ==="

# GitHub Releases
update_recipe "fastfetch" $(get_github_latest "fastfetch-cli/fastfetch")
update_recipe "htop"      $(get_github_latest "htop-dev/htop")
update_recipe "cmatrix"   $(get_github_latest "abishekvashok/cmatrix")
update_recipe "kmod"      $(get_github_latest "kmod-project/kmod")

# GNU Mirrors
update_recipe "bash"      $(get_gnu_latest "bash")
update_recipe "coreutils" $(get_gnu_latest "coreutils")
update_recipe "make"      $(get_gnu_latest "make")
update_recipe "nano"      $(get_gnu_latest "nano")
update_recipe "gcc"       $(get_gnu_latest "gcc")

# BusyBox
BUSYBOX_VER=$(curl -s https://busybox.net/downloads/ | grep -oP 'busybox-\d+\.\d+\.\d+\.tar\.bz2' | tail -1 | sed -E 's/busybox-(.*)\.tar\.bz2/\1/')
update_recipe "busybox" "$BUSYBOX_VER"
