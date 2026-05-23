#/bin/bash

# Array con el formato: "Nombre_Paquete|URL_GitHub|Sistema_Build"
# ¡Acá podés meter todas las líneas que quieras de un tirón!
paquetes=(
    "neofetch|https://github.com/dylanaraps/neofetch|make"
    "htop|https://github.com/htop-dev/htop|script"
    "fetch|https://github.com/jschicht/fetch|make"
    "cmatrix|https://github.com/bisqwit/cmatrix|make"
)

echo "Starting Recipe generation."

for item in "${paquetes[@]}"; do
    # Parseamos los datos usando la barra vertical como separador
    IFS="|" read -r name repo build <<< "$item"
    
    dir="packages/$name"
    
    echo " -> Procesando: $name"
    mkdir -p "$dir"
    
    # Creamos el archivo de receta limpio
    cat << EOR > "$dir/recipe.txt"
NAME="$name"
REPO="$repo"
BUILD_SYSTEM="$build"
DEPENDENCIES="gcc,make"
EOR
done

echo "Recipes loaded locally."
