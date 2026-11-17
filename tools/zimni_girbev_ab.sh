#!/bin/bash
# Parcours de Girbev (Ungru -> Ajvan -> Ungru -> Ajvan), RAM + VRAM par étape.
#  gb_base : sans nos optimisations (chunks OFF, textures d'origine = dossier Assets sorti)
#  gb_opti : chunks végétation ON (1600 m, purge au chargement) + textures réduites
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
A="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti/Assets"
T="/c/Users/benbo/tools/PakTools/staging/Assets_tex_off"
L="/c/Users/benbo/Documents/My Games/ZimniBench/logs"
cd /c/Users/benbo/tools/PakTools
par_etape() { # $1 label : VRAM moyenne/max entre deux repères BENCH POINT du log client
  log="$(ls -dt "$L"/logs_*/ | head -1)console.log"
  grep -E "\[ZO\] BENCH (POINT|TERMINE)" "$log" | awk '{split($1,t,"."); n=$NF; if($0~/TERMINE/) n="FIN"; print t[1], n}' > /tmp/gb_marks.txt
  awk 'NR==FNR{mt[NR]=$1; mn[NR]=$2; k=NR; next} $2+0>0{for(i=1;i<k;i++) if($1>=mt[i] && $1<mt[i+1]){s[i]+=$2; c[i]++; if($2>m[i])m[i]=$2}} END{for(i=1;i<k;i++) printf "  %-7s VRAM moy=%d max=%d (n=%d)\n", mn[i], (c[i]?s[i]/c[i]:0), m[i], c[i]}' /tmp/gb_marks.txt "$P/$1_vram.txt"
  awk -F';' 'NR>2{s[$1]+=$7; n[$1]++; if($7>m[$1])m[$1]=$7} END{for(k in s) printf "  %-7s RAM moy=%d max=%d\n", k, s[k]/n[k], m[k]}' "$P/$1.csv" | sort
}
[ -d "$T" ] && { echo "staging deja present, arret"; exit 1; }
: > "$P/cull"; : > "$P/cullmode"
mv "$A" "$T" && echo "Assets sortis (textures d'origine)"
TMIN=30 bash zimni_server_series.sh "gb_base:girbev.txt:::::" 2>&1 | grep -E "FIN|KO|TIMEOUT|pack|VRAM"
par_etape gb_base > /tmp/gb_base.txt
mv "$T" "$A" && echo "Assets remis: $(find "$A" -name '*.edds' | wc -l) edds"
echo 1600 > "$P/cull"; echo chunktrees > "$P/cullmode"
TMIN=40 bash zimni_server_series.sh "gb_opti:girbev.txt:::::" 2>&1 | grep -E "FIN|KO|TIMEOUT|pack|VRAM"
par_etape gb_opti > /tmp/gb_opti.txt
: > "$P/cull"; : > "$P/cullmode"
echo "== gb_base"; cat /tmp/gb_base.txt; echo "== gb_opti"; cat /tmp/gb_opti.txt
cat /tmp/gb_base.txt /tmp/gb_opti.txt > analyses/girbev_ab.txt
