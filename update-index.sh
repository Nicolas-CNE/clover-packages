#!/bin/sh
# update-index.sh: Genera PACKINDEX.txt leyendo los .recipe de packages/

OUTPUT="PACKINDEX.txt"
echo "# Fortune Package Index - Autogenerado" > "$OUTPUT"

for recipe in packages/*/*.recipe; do
    [ -f "$recipe" ] || continue
    
    pkgname=""
    version=""
    deps=""
    description=""

    # Parsea los campos principales
    pkgname=$(grep -E '^PKGNAME=' "$recipe" | cut -d'=' -f2 | tr -d '"' | tr -d "'")
    version=$(grep -E '^VERSION=' "$recipe" | cut -d'=' -f2 | tr -d '"' | tr -d "'")
    deps=$(grep -E '^DEPS=' "$recipe" | cut -d'=' -f2 | tr -d '"' | tr -d "'")
    description=$(grep -E '^DESCRIPTION=' "$recipe" | cut -d'=' -f2 | tr -d '"' | tr -d "'")

    if [ -n "$pkgname" ]; then
        echo "${pkgname}|${version}|${deps}|${description}" >> "$OUTPUT"
    fi
done

echo "[OK] PACKINDEX.txt generado con éxito."
