#!/bin/bash
# A/B éjections REPLICATION FLOODED : console bridée (6 threads, vue Series S), combat, téléportations ; serveur par défaut vs rafales limitées
cd /c/Users/benbo/tools/PakTools
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
export AFF=63 TMIN=40
echo 1300 > "$P/cull"; echo chunkall > "$P/cullmode"; rm -f "$P/streamr" "$P/streamprofile"
LB="/c/Users/benbo/Documents/My Games/ZimniBench/logs"
for v in "fl_def1:villes_war.txt::1:8::" "fl_lim1:villes_war.txt::1:8::-streamingBudget 200 -streamsDelta 20" \
         "fl_def2:villes_war.txt::1:8::" "fl_lim2:villes_war.txt::1:8::-streamingBudget 200 -streamsDelta 20"; do
  bash zimni_server_series.sh "$v" 2>&1 | grep -E "VRAM|FIN|KICK"
  L=$(ls -dt "$LB"/logs_*/ | head -1); l=${v%%:*}
  echo "   $l : mesures $(($(wc -l < "$P/$l.csv")-1)), points $(grep -c 'BENCH POINT' "$L/console.log"), flooded $(grep -c 'Connection is flooded' "$L/console.log")"
done
echo FLOOD_FINI
