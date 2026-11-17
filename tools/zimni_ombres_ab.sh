#!/bin/bash
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
: > "$P/cull"; : > "$P/cullmode"; echo 4 > "$P/streambudget"; touch "$P/streamconsole"; echo 1300 > "$P/streamr"; rm -f "$P/streamfps" "$P/grass" "$P/lights" "$P/inradius" "$P/capval"
run(){ ENG="$2" TMIN=30 bash zimni_server_series.sh "$1:vol_sol.txt::1" 2>&1 | grep -E "pack|VRAM|KICK"
  L=$(ls -dt "/c/Users/benbo/Documents/My Games/ZimniBench/logs"/logs_*/ | head -1); grep -h "REGLAGES MOTEUR\|SCRIPT +(E).*ZO" "$L/console.log" | cut -c1-160 | head -3
  awk -F';' -v L=$1 'NR>2{n++; f+=$5; if($6>50)s++; if($6>w)w=$6; if($7>mm)mm=$7} END{printf "%s n=%d fps=%.1f pire=%s >50ms=%d memJeu=%d\n",L,n,f/n,w,s,mm}' "$P/$1.csv"; }
run om_ref ""
run om_q0 "PipelineUserSettings ShadowQuality i 0"
run om_dist0 "VideoUserSettings DistantShadowsQuality i 0"
run om_ref2 ""
rm -f "$P/streamconsole"; echo OMBRES_FINI
