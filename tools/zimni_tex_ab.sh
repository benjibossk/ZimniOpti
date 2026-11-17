#!/bin/bash
# A/B textures réduites : tex_on (ZimniOpti tel quel) puis tex_off (dossier Assets sorti temporairement). Chunks désactivés.
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
A="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti/Assets"
T="/c/Users/benbo/tools/PakTools/staging/Assets_tex_off"
cd /c/Users/benbo/tools/PakTools
powershell -NoProfile -ExecutionPolicy Bypass -File vram_sampler.ps1 -Out "C:\Users\benbo\tools\PakTools\analyses\vram_tex.txt" &
SP=$!
: > "$P/cull"; : > "$P/cullmode"
date +"debut tex_on %H:%M:%S"; TMIN=30 bash zimni_server_series.sh "tex_on:villes.txt:::::" 2>&1 | grep -E "FIN|KO|TIMEOUT|pack"
[ -d "$T" ] && { echo "staging deja present, arret"; exit 1; }
mv "$A" "$T" && echo "Assets sortis"
date +"debut tex_off %H:%M:%S"; TMIN=30 bash zimni_server_series.sh "tex_off:villes.txt:::::" 2>&1 | grep -E "FIN|KO|TIMEOUT|pack"
mv "$T" "$A" && echo "Assets remis"
ls "$A" >/dev/null && echo "verif Assets OK: $(find "$A" -name '*.edds' | wc -l) edds"
for f in tex_on tex_off; do echo "== $f"; awk -F';' 'NR>2{print $1, $5, $7}' "$P/$f.csv" | awk '{m[$1]=m[$1]" "$3; f[$1]+=$2; n[$1]++} END{for(k in m) printf "%s fps=%.0f mem=%s\n",k,f[k]/n[k],m[k]}'; ls -la --time-style=+%H:%M "$P/$f.csv"; done
kill $SP 2>/dev/null
