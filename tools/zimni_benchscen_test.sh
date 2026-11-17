#!/bin/bash
# Test du scénario joueurs « ZimniOpti - Benchmark » (parcours intégré, pas de bench_points.txt)
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
PS="powershell -NoProfile -ExecutionPolicy Bypass -File"
r=$($PS 'C:\Users\benbo\tools\PakTools\zimni_pack.ps1' | grep EXIT); echo "pack $r"
rm -f "$P/bench_points.txt" "$P/forcecap"; touch "$P/streamconsole"
$PS 'C:\Users\benbo\tools\PakTools\zimni_server_bench.ps1' -Scenario "{DCC383B3E28EF8EA}Missions/ZO_Benchmark.conf" -Label benchscen -TimeoutMin 15 | grep -E "FIN|KICK" | cut -c1-150
rm -f "$P/streamconsole"
L=$(ls -dt "/c/Users/benbo/Documents/My Games/ZimniBench/logs"/logs_*/ | head -1)
grep -hE "BENCH (VOL|TERMINE|RESULTAT)|ZO_Bench|SCRIPT +\(E\).*ZO" "$L/console.log" | cut -c1-300 | head
