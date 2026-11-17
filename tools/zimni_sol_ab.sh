#!/bin/bash
# Au sol (2 m, 15 m/s) : rayon fixe 1300 vs 800, console simulée
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
: > "$P/cull"; : > "$P/cullmode"; echo 4 > "$P/streambudget"; touch "$P/streamconsole"; echo off > "$P/streamfps"; rm -f "$P/grass" "$P/lights"
for r in 1300 800; do
  echo $r > "$P/streamr"
  TMIN=30 bash zimni_server_series.sh "sol_$r:vol_sol.txt::1" 2>&1 | grep -E "VRAM|KICK"
  awk -F';' -v L=$r 'NR>2{n++; f+=$5; if($6>50)s++; if($6>w)w=$6; if($7>mm)mm=$7} END{printf "sol rayon %s n=%d fps=%.1f pire=%s >50ms=%d memJeu=%d\n",L,n,f/n,w,s,mm}' "$P/sol_$r.csv"
done
echo 1300 > "$P/streamr"; rm -f "$P/streamfps" "$P/streamconsole"; echo SOL_FINI
