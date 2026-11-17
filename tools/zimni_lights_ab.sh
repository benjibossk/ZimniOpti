#!/bin/bash
# A/B lumières sur le vol de caméra : normal / sans ombres / éteintes (console simulée, streaming on)
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
: > "$P/cull"; : > "$P/cullmode"; echo 1300 > "$P/streamr"; echo 4 > "$P/streambudget"; touch "$P/streamconsole"
for m in count noshadow off; do
  echo "$m" > "$P/lights"
  TMIN=25 bash zimni_server_series.sh "li_$m:vol.txt::1" 2>&1 | grep -E "VRAM|KICK"
  L=$(ls -dt "/c/Users/benbo/Documents/My Games/ZimniBench/logs"/logs_*/ | head -1)
  grep -h "LIGHTS total" "$L/console.log" | sed 's/.*LIGHTS/LIGHTS/' | sort | uniq -c | head -4
  awk -F';' -v L=$m 'NR>2{n++; f+=$5; if($6>50)s++; if($6>w)w=$6; if($7>mm)mm=$7} END{printf "%s n=%d fps=%.1f pire=%s >50ms=%d memJeu=%d\n",L,n,f/n,w,s,mm}' "$P/li_$m.csv"
done
rm -f "$P/lights" "$P/streamconsole"; echo LIGHTS_FINI
