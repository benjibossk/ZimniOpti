#!/bin/bash
# A/B mémoire RÉELLE : layers d'origine (sauvegarde 2026-11-09) vs layers allégés (buissons 1/3, rochers 1/2), chunks off
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
W="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti/World/Zimnitrita"
S="/c/Users/benbo/tools/PakTools/staging/World_allege"
B="/c/Users/benbo/tools/PakTools/BACKUP_layers_20261109/Zimnitrita"
: > "$P/cull"; : > "$P/cullmode"
[ -d "$S" ] && { echo "staging deja present, stop"; exit 1; }
mkdir -p "$S"
# on ne déplace que les .layer (le reste du dossier World reste)
(cd "$W" && find . -name "*.layer" | while read f; do mkdir -p "$S/$(dirname "$f")"; mv "$f" "$S/$f"; done)
(cd "$B" && find . -name "*.layer" | while read f; do cp "$f" "$W/$f"; done)
echo "layers d'origine en place: $(find "$W" -name '*.layer' | wc -l)"
TMIN=30 bash zimni_server_series.sh "lay_orig:p1.txt:::::"
(cd "$S" && find . -name "*.layer" | while read f; do mv -f "$f" "$W/$f"; done)
left=$(find "$S" -name "*.layer" | wc -l); echo "layers alleges remis, reste en staging: $left"
[ "$left" = 0 ] && rm -rf "$S"
TMIN=30 bash zimni_server_series.sh "lay_alleg:p1.txt:::::"
for l in lay_orig lay_alleg; do echo "== $l"; awk -F';' 'NR>1{print $1, $5, $7}' "$P/$l.csv"; awk 'NF>=3' "$P/${l}_vram.txt" | tail -4; done
