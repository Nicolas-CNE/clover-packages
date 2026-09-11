#!/bin/sh
OUTPUT="PACKINDEX.txt"
echo "# Fortune Package Index - Autogenerado" > "$OUTPUT"

for recipe in packages/*/*.recipe packages/*/*.txt; do
    [ -f "$recipe" ] || continue
    
    # Ignorar si está dentro de la carpeta de código fuente de fortune
    case "$recipe" in
        packages/fortune/*) continue ;;
    esac

    pkg_name=""
    version=""
    deps=""
    description=""

    pkg_name=$(grep -E "^PKGNAME=" "$recipe" | cut -d'=' -f2 | tr -d '"' | tr -d "'")
    version=$(grep -E "^VERSION=" "$recipe" | cut -d'=' -f2 | tr -d '"' | tr -d "'")
    deps=$(grep -E "^DEPS=" "$recipe" | cut -d'=' -f2 | tr -d '"' | tr -d "'")
    description=$(grep -E "^DESCRIPTION=" "$recipe" | cut -d'=' -f2 | tr -d '"' | tr -d "'")

    if [ -n "$pkg_name" ]; then
        echo "$pkg_name|$version|$deps|$description" >> "$OUTPUT"
    fi
done

echo "[OK] PACKINDEX.txt generado con éxito."
