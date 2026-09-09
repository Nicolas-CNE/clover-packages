#!/usr/bin/env bash
set -e

GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m'

get_github_latest() {
    local repo="$1"
    curl -s "https://api.github.com/repos/${repo}/releases/latest" | grep '"tag_name":' | sed -E 's/.*"v?([^"]+)".*/\1/'
}

update_recipe() {
    local pkg="$1"
    local new_ver="$2"
    local recipe_file="packages/${pkg}/${pkg}.recipe"

    [ ! -f "$recipe_file" ] && recipe_file="${pkg}/${pkg}.recipe"
    if [ ! -f "$recipe_file" ]; then
        return
    fi

    local current_ver=$(grep -i '^VERSION' "$recipe_file" | cut -d'=' -f2- | tr -d '"' | tr -d "'" | xargs)

    if [ -n "$new_ver" ] && [ -n "$current_ver" ] && [ "$current_ver" != "$new_ver" ]; then
        echo -e "${BLUE}[UPDATE]${NC} $pkg: $current_ver -> $new_ver"
        sed -i "s/$current_ver/$new_ver/g" "$recipe_file"
    elif [ -n "$current_ver" ]; then
        echo -e "${GREEN}[OK]${NC} $pkg ($current_ver) está al día."
    fi
}

echo "=== Actualizando recetas estables ==="

# GitHub Repos
update_recipe "fastfetch" $(get_github_latest "fastfetch-cli/fastfetch")
update_recipe "htop"      $(get_github_latest "htop-dev/htop")
update_recipe "cmatrix"   $(get_github_latest "abishekvashok/cmatrix")

# Kernel.org / Kmod
KMOD_VER=$(curl -s https://mirrors.kernel.org/pub/linux/utils/kernel/kmod/ | grep -oP 'kmod-\d+\.tar\.xz' | sort -V | tail -1 | sed -E 's/kmod-(.*)\.tar\.xz/\1/')
update_recipe "kmod" "$KMOD_VER"

# BusyBox
BUSYBOX_VER=$(curl -s https://busybox.net/downloads/ | grep -oP 'busybox-\d+\.\d+\.\d+\.tar\.bz2' | sort -V | tail -1 | sed -E 's/busybox-(.*)\.tar\.bz2/\1/')
update_recipe "busybox" "$BUSYBOX_VER"
