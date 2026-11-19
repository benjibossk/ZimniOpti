#!/bin/bash
# Phase 0 : étalonnage Everon / Arland / Zimnitrita d'origine / Zimnitrita+ZimniOpti, survol + combat, vue 1200 m partout.
T=/c/Users/benbo/tools/PakTools; M="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti"
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"; S="$T/staging/publish_hors_mod"
PS="powershell -NoProfile -ExecutionPolicy Bypass -File"; OUT="$T/analyses/etalonnage_$(date +%Y%m%d_%H%M).txt"
cp "$S"/ZO_Benchmark*_Eden.conf* "$S"/ZO_Benchmark*_Arland.conf* "$M/Missions/" 2>/dev/null
mv "$P/bench_points.txt" "$P/bench_points.txt.off" 2>/dev/null
rm -f "$P/forcecap" "$P/engset" "$P/streamconsole"
run() { # label scenario
  r=$($PS "$T/zimni_pack.ps1" | grep EXIT); echo "pack $r"; echo "$r" | grep -q EXIT=0 || return
  echo 1200 > "$P/viewdist"
  $PS "$T/vram_sampler.ps1" -Out "$(cygpath -w "$P/$1_vram.txt")" > /dev/null 2>&1 &
  $PS "$T/zimni_server_bench.ps1" -Scenario "$2" -Label "$1" -TimeoutMin 18 | grep -E "FIN|KO|TIMEOUT|MORT"
  while tasklist | grep -qiE "ArmaReforgerSteamDiag"; do sleep 5; done; sleep 10
  L=$(ls -dt "/c/Users/benbo/Documents/My Games/ZimniBench/logs/"*/ | head -1)
  { echo "== $1"; grep -aE "BENCH ville|FPS moyens|Pire image|Saccades|Memoire" "$L/console.log" | sed 's/^.*SCRIPT *: //';
    awk '$2+0>0{v=$2+0; s+=v; n++; if(v>m)m=v; if($3+0>r)r=$3+0} END{if(n) printf "VRAM moy %d Mo pic %d Mo ; RAM privee pic %d Mo\n", s/n, m, r}' "$P/$1_vram.txt"; } >> "$OUT"
}
touch "$P/streamconsole"
run zopti_combat  "{7A3C5E91B24D06F8}Missions/ZO_BenchmarkCombat.conf"
rm -f "$P/streamconsole"
rm -f "$P/streamconsole"
bash "$T/zimni_stream_deploy.sh" off > /dev/null
run zorig_vol     "{DCC383B3E28EF8EA}Missions/ZO_Benchmark.conf"
run zorig_combat  "{7A3C5E91B24D06F8}Missions/ZO_BenchmarkCombat.conf"
bash "$T/zimni_stream_deploy.sh" on > /dev/null
rm -f "$P/viewdist" "$M"/Missions/ZO_Benchmark*_Eden.conf* "$M"/Missions/ZO_Benchmark*_Arland.conf*
mv "$P/bench_points.txt.off" "$P/bench_points.txt" 2>/dev/null
echo "FINI -> $OUT"
