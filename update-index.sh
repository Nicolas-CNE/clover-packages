#!/usr/bin/env bash
set -euo pipefail

# Obtener la ruta absoluta de la raíz del repositorio
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGES_DIR="${REPO_DIR}/packages"
INDEX_FILE="${REPO_DIR}/INDEX"
TMP_INDEX="$(mktemp)"

echo -e "\033[34m[INFO]\033[0m Escaneando recetas en ${PACKAGES_DIR}..."

if [ ! -d "$PACKAGES_DIR" ]; then
    echo -e "\033[31m[ERROR]\033[0m No existe el directorio ${PACKAGES_DIR}" >&2
    rm -f "$TMP_INDEX"
    exit 1
fi

count=0

# Buscar todos los archivos .recipe dentro de subcarpetas de packages/
while IFS= read -r -d '' recipe_path; do
    pkg_name=""
    pkg_version=""
    pkg_desc=""

    # Leer las variables dentro del .recipe
    while IFS='=' read -r key val || [ -n "$key" ]; do
        # Omitir comentarios o líneas vacías
        [[ "$key" =~ ^[[:space:]]*# ]] && continue
        [[ -z "$key" ]] && continue

        # Limpiar espacios y comillas del valor
        key="$(echo "$key" | xargs)"
        val="$(echo "$val" | sed -e 's/^"//' -e 's/"$//' -e "s/^'//" -e "s/'$//")"

        case "$key" in
            NAME) pkg_name="$val" ;;
            VERSION) pkg_version="$val" ;;
            DESCRIPTION) pkg_desc="$val" ;;
        esac
    done < "$recipe_path"

    # Si se encontró un NAME válido, agregar al índice
    if [ -n "$pkg_name" ]; then
        # Calcular la ruta relativa desde la raíz del repo (ej: packages/zlib/zlib.recipe)
        rel_path="${recipe_path#$REPO_DIR/}"
        
        # Formato de salida: NOMBRE|VERSIÓN|DESCRIPCIÓN|RUTA
        echo "${pkg_name}|${pkg_version:-1.0.0}|${pkg_desc:-Sin descripcion}|${rel_path}" >> "$TMP_INDEX"
        echo -e "  \033[32m+\033[0m Registrado: \033[1m${pkg_name}\033[0m (v${pkg_version:-1.0.0})"
        ((count++))
    fi
done < <(find "$PACKAGES_DIR" -type f -name "*.recipe" -print0)

# Reemplazar el INDEX final de forma segura
mv "$TMP_INDEX" "$INDEX_FILE"

echo -e "\033[32m[OK]\033[0m INDEX actualizado con $count receta(s) en: $INDEX_FILE"
