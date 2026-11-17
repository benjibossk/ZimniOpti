#!/bin/bash
# Attend la fin du contrôle air/sol, puis tests à pied : vue 1200/900, intérieurs 60/30
cd /c/Users/benbo/tools/PakTools
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
: > "$P/cull"; : > "$P/cullmode"; echo 4 > "$P/streambudget"; touch "$P/streamconsole"; echo 1300 > "$P/streamr"; rm -f "$P/streamfps" "$P/grass" "$P/lights" "$P/inradius"
run(){ TMIN=30 bash zimni_server_series.sh "$1:vol_sol.txt::1" 2>&1 | grep -E "pack|VRAM|KICK"
  L=$(ls -dt "/c/Users/benbo/Documents/My Games/ZimniBench/logs"/logs_*/ | head -1); grep -h "SCRIPT +(E).*ZO\|CULL mode\|vue=" "$L/console.log" | cut -c1-140 | head -4
  awk -F';' -v L=$1 'NR>2{n++; f+=$5; if($6>50)s++; if($6>w)w=$6; if($7>mm)mm=$7} END{printf "%s n=%d fps=%.1f pire=%s >50ms=%d memJeu=%d\n",L,n,f/n,w,s,mm}' "$P/$1.csv"; }
echo 1200 > "$P/capval"; run pied_ref
echo 900 > "$P/capval"; run pied_vue900
echo 1200 > "$P/capval"; echo 30 > "$P/inradius"; run pied_int30
rm -f "$P/inradius" "$P/streamconsole" "$P/capval"; echo PIED_FINI
