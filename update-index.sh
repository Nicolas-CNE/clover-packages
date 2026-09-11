#!/usr/bin/env bash
set -euo pipefail

shopt -s globstar

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGES_DIR="${REPO_DIR}/packages"
INDEX_FILE="${REPO_DIR}/INDEX"
TMP_INDEX="$(mktemp)"

echo -e "\033[34m[INFO]\033[0m Escaneando recetas en ${PACKAGES_DIR}..."

count=0

for recipe_path in "$PACKAGES_DIR"/**/*.recipe; do
    [ -f "$recipe_path" ] || continue

    # Extraer variables con sed/awk simple evitando subshells complejas
    pkg_name=$(sed -n 's/^NAME=['\''"]\?\([^'\''"]*\)['\''"]\?/\1/p' "$recipe_path" | head -n1)
    pkg_version=$(sed -n 's/^VERSION=['\''"]\?\([^'\''"]*\)['\''"]\?/\1/p' "$recipe_path" | head -n1)
    pkg_desc=$(sed -n 's/^DESCRIPTION=['\''"]\?\([^'\''"]*\)['\''"]\?/\1/p' "$recipe_path" | head -n1)

    if [ -n "$pkg_name" ]; then
        rel_path="${recipe_path#$REPO_DIR/}"
        echo "${pkg_name}|${pkg_version:-1.0.0}|${pkg_desc:-Sin descripcion}|${rel_path}" >> "$TMP_INDEX"
        echo -e "  \033[32m+\033[0m Registrado: \033[1m${pkg_name}\033[0m (${rel_path})"
        count=$((count + 1))
    fi
done

if [ -s "$TMP_INDEX" ]; then
    sort -t'|' -k1,1 "$TMP_INDEX" > "$INDEX_FILE"
else
    > "$INDEX_FILE"
fi
rm -f "$TMP_INDEX"

echo -e "\033[32m[OK]\033[0m INDEX actualizado con $count receta(s) en: $INDEX_FILE"
