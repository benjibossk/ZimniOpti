#!/bin/bash
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
PS="powershell -NoProfile -ExecutionPolicy Bypass -File"
r=$($PS 'C:\Users\benbo\tools\PakTools\zimni_pack.ps1' | grep EXIT); echo "pack $r"; echo "$r" | grep -q EXIT=0 || exit 1
rm -f "$P/engset" "$P/viewdist"; touch "$P/forcecap"
$PS 'C:\Users\benbo\tools\PakTools\vram_sampler.ps1' -Out "$(cygpath -w "$P/combat_vram.txt")" > /dev/null 2>&1 &
$PS 'C:\Users\benbo\tools\PakTools\zimni_server_bench.ps1' -Scenario "{7A3C5E91B24D06F8}Missions/ZO_BenchmarkCombat.conf" -Label combat -TimeoutMin 18 | grep -E "KICK|FIN|KO|INCOMPATIBLE|TIMEOUT|MORT"
while tasklist | grep -qiE "ArmaReforgerSteamDiag|ArmaReforgerServer.*ZimniBench"; do sleep 2; done
rm -f "$P/forcecap"; echo FINI
