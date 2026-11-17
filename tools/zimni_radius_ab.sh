#!/bin/bash
# Rayon de streaming console (vue plafonnée 1200 m) : 1300 / 1000 / 800, vol 12 km, console simulée
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
: > "$P/cull"; : > "$P/cullmode"; echo 4 > "$P/streambudget"; touch "$P/streamconsole"; rm -f "$P/lights"
for r in 1300 1000 800; do
  echo $r > "$P/streamr"
  TMIN=25 bash zimni_server_series.sh "ra_$r:vol.txt::1" 2>&1 | grep -E "VRAM|KICK"
  awk -F';' -v L=$r 'NR==1{print $0} NR>2{n++; f+=$5; if($6>50)s++; if($6>w)w=$6; if($7>mm)mm=$7} END{printf "rayon %s n=%d fps=%.1f pire=%s >50ms=%d memJeu=%d\n",L,n,f/n,w,s,mm}' "$P/ra_$r.csv"
done
echo 1300 > "$P/streamr"; rm -f "$P/streamconsole"; echo RAYON_FINI
