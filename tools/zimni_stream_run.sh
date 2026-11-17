#!/bin/bash
# banc streaming : rayon $1, budget $2 ms, label $3
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
: > "$P/cull"; : > "$P/cullmode"; echo "$1" > "$P/streamr"; echo "$2" > "$P/streambudget"
TMIN=25 bash zimni_server_series.sh "$3:p1.txt:::::" 2>&1 | tail -3
awk -F';' 'NR>1{print $1,$5,$6,$7}' "$P/$3.csv"
awk 'NF>=3' "$P/$3_vram.txt" | awk 'NR%3==1'
L=$(ls -dt "/c/Users/benbo/Documents/My Games/ZimniBench/logs"/logs_*/ | head -1)
grep -hE "\[ZO\] STREAM|SCRIPT +\(E\)|VM Exception" "$L/console.log" | head -14 | cut -c1-220
