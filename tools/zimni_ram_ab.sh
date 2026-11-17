#!/bin/bash
# A/B mémoire RÉELLE (commit Windows) : chunks off vs chunkall, point T1
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
: > "$P/cull"; : > "$P/cullmode"
TMIN=30 bash zimni_server_series.sh "ram_off:p1.txt:::::"
echo 1600 > "$P/cull"; echo chunkall > "$P/cullmode"
TMIN=30 bash zimni_server_series.sh "ram_all:p1.txt:::::"
: > "$P/cull"; : > "$P/cullmode"
for l in ram_off ram_all; do echo "== $l"; awk -F';' 'NR>1{print $1, $5, $7}' "$P/$l.csv" 2>/dev/null; tail -3 "$P/${l}_vram.txt"; done
