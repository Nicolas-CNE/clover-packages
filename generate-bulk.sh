#!/bin/bash

# Función para crear la receta
create_recipe() {
    local name=$1
    local url=$2
    local type=$3 # 'c' para compilar, 'script' para copiar directo

    mkdir -p packages/$name
    cat <<EOF > packages/$name/$name.txt
NAME=$name
SOURCE_URL=$url
TYPE=$type
EOF
    echo "Receta para $name generada (Tipo: $type)"
}

# --- ADDED PACKAGES ---
create_recipe "neofetch" "https://github.com/dylanaraps/neofetch/archive/refs/tags/7.1.0.tar.gz" "script"
create_recipe "cmatrix" "https://github.com/abishekvashok/cmatrix/archive/refs/heads/master.tar.gz" "c"
# ---------------------------------------------
create_recipe "nano" "https://github.com/nano-editor/nano/archive/refs/tags/v8.0.tar.gz" "c"
create_recipe "htop" "https://github.com/htop-dev/htop/archive/refs/tags/3.3.0.tar.gz" "c"
