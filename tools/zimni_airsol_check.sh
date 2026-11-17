#!/bin/bash
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
: > "$P/cull"; : > "$P/cullmode"; echo 4 > "$P/streambudget"; touch "$P/streamconsole"; echo 1300 > "$P/streamr"; rm -f "$P/streamfps" "$P/grass" "$P/lights"
for v in vol vol_sol; do
  TMIN=30 bash zimni_server_series.sh "as_$v:$v.txt::1" 2>&1 | grep -E "pack|VRAM|KICK"
  L=$(ls -dt "/c/Users/benbo/Documents/My Games/ZimniBench/logs"/logs_*/ | head -1); grep -h "STREAM rayon\|SCRIPT +(E).*ZO" "$L/console.log" | cut -c1-130 | head -8
  awk -F';' -v L=$v 'NR>2{n++; f+=$5; if($6>50)s++; if($6>w)w=$6; if($7>mm)mm=$7} END{printf "%s n=%d fps=%.1f pire=%s >50ms=%d memJeu=%d\n",L,n,f/n,w,s,mm}' "$P/as_$v.csv"
done
rm -f "$P/streamconsole"; echo AS_FINI
