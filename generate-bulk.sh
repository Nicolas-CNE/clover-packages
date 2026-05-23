#!/bin/bash

paquetes=(
    "neofetch|https://github.com/dylanaraps/neofetch|make"
    "htop|https://github.com/htop-dev/htop|script"
    "fetch|https://github.com/jschicht/fetch|make"
    "cmatrix|https://github.com/bisqwit/cmatrix|make"
)

echo "Starting Recipe generation."

for item in "${paquetes[@]}"; do
    IFS="|" read -r name repo build <<< "$item"
    dir="packages/$name"
    
    echo " -> Procesando: $name"
    mkdir -p "$dir"
    
    cat << EOR > "$dir/recipe.txt"
NAME="$name"
REPO="$repo"
BUILD_SYSTEM="$build"
DEPENDENCIES="gcc,make"
EOR
done

echo "Recipes loaded locally."
