#!/bin/bash
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
Z="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti"
cd /c/Users/benbo/tools/PakTools
: > "$P/cull"; : > "$P/cullmode"; echo 4 > "$P/streambudget"; touch "$P/streamconsole"; echo 1300 > "$P/streamr"; rm -f "$P/streamfps" "$P/inradius" "$P/capval"
run(){ TMIN=30 bash zimni_server_series.sh "$1:$2::1" 2>&1 | grep -E "pack|VRAM|KICK"
  L=$(ls -dt "/c/Users/benbo/Documents/My Games/ZimniBench/logs"/logs_*/ | head -1); grep -ch "Terrain.*\(E\)\|edds.*(E)" "$L/console.log"
  awk -F';' -v L=$1 'NR>2{n++; f+=$5; if($6>50)s++; if($7>mm)mm=$7} END{printf "%s n=%d fps=%.1f >50ms=%d memJeu=%d\n",L,n,f/n,s,mm}' "$P/$1.csv"; }
mkdir -p "$Z/World/Zimnitrita/Terrain" && cp -r staging/terrain256/World/Zimnitrita/Terrain/.Data "$Z/World/Zimnitrita/Terrain/" && echo "tuiles 256 copiees"
run ter256_sol vol_sol.txt
run ter256_vol vol.txt
rm -rf "$Z/World/Zimnitrita/Terrain" && echo "tuiles retirees"
run ter_ref_sol vol_sol.txt
run ter_ref_vol vol.txt
rm -f "$P/streamconsole"; echo TER_FINI
