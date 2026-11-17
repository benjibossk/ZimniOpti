#!/bin/bash
# A/B herbe et petits objets, vol 12 km, console simulée (rayon adaptatif, cible 40)
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
echo 4 > "$P/streambudget"; touch "$P/streamconsole"; echo 1300 > "$P/streamr"; rm -f "$P/lights" "$P/streamfps"
run() { # label grass cull cullmode
  rm -f "$P/grass"; [ -n "$2" ] && echo "$2" > "$P/grass"
  echo "$3" > "$P/cull"; echo "$4" > "$P/cullmode"
  TMIN=25 bash zimni_server_series.sh "$1:vol.txt::1" 2>&1 | grep -E "VRAM|KICK"
  L=$(ls -dt "/c/Users/benbo/Documents/My Games/ZimniBench/logs"/logs_*/ | head -1); grep -h "HERBE\|PROPCULL\|CULL" "$L/console.log" | head -3 | cut -c1-150
  awk -F';' -v L=$1 'NR==1{print} NR>2{n++; f+=$5; if($6>50)s++; if($6>w)w=$6; if($7>mm)mm=$7} END{printf "%s n=%d fps=%.1f pire=%s >50ms=%d memJeu=%d\n",L,n,f/n,w,s,mm}' "$P/$1.csv"
}
run gr_base "" "" ""
run gr_50 50 "" ""
run gr_10 10 "" ""
run pt_250 "" 250 ""
rm -f "$P/grass" "$P/streamconsole"; : > "$P/cull"; : > "$P/cullmode"; echo HERBE_FINI
