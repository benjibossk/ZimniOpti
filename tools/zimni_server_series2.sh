#!/bin/bash
# Série client/serveur : variantes "label:points:mods:cap" (mods = GUIDs séparés par des virgules, cap=1 -> plafond console simulé)
# ex: zimni_server_series.sh s_w0:war0.txt:: s_w8:war8.txt:: s_w8phys:war8.txt:64F0FE5157B83EF0: s_w8cap:war8.txt::1
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
PS="powershell -NoProfile -ExecutionPolicy Bypass -File"
while tasklist | grep -qi ArmaReforgerSteamDiag; do sleep 2; done
r=$($PS 'C:\Users\benbo\tools\PakTools\zimni_pack.ps1' | grep EXIT); echo "pack $r"; echo "$r" | grep -q EXIT=0 || exit 1
for v in "$@"; do
  IFS=: read lab pts mods cap vc ping sa <<< "$v"
  rm -f "$P/forcecap" "$P/engset" "$P/viewdist"; [ "$cap" = 1 ] && touch "$P/forcecap"; [ -n "$ENGSET" ] && echo "$ENGSET" | tr "|" "\n" > "$P/engset"
  echo "== $lab ($pts mods=$mods cap=$cap)"
  # AFF=<masque> : bride les coeurs du client (approche console)
  [ -n "$AFF" ] && $PS 'C:\Users\benbo\tools\PakTools\cpu_limit.ps1' -Mask "$AFF" > /dev/null 2>&1 &
  # VRAM du client (compteur Windows) -> ZimniPerf/<label>_vram.txt ; le sampler s'arrête seul après le client
  $PS 'C:\Users\benbo\tools\PakTools\vram_sampler.ps1' -Out "$(cygpath -w "$P/${lab}_vram.txt")" > /dev/null 2>&1 &
  $PS 'C:\Users\benbo\tools\PakTools\zimni_server_bench.ps1' -Label "$lab" -Points "$pts" -Mods "$mods" -Vcons "${vc:-0}" -PingMs "${ping:-0}" -SrvArgs "$sa" -TimeoutMin "${TMIN:-25}" | grep -E "KICK|FIN|KO|INCOMPATIBLE|TIMEOUT|MORT|TELECH"
  while tasklist | grep -qiE "ArmaReforgerSteamDiag|ArmaReforgerServer"; do sleep 2; done
  awk '$2+0>0{v=$2+0; s+=v; n++; if(v>m)m=v; if($3+0>r)r=$3+0; w=$3+0} END{if(n) printf "VRAM %s : moyenne %d Mo, pic %d Mo ; RAM privee pic %d Mo, fin %d Mo\n", L, s/n, m, r, w}' L="$lab" "$P/${lab}_vram.txt" 2>/dev/null
done
rm -f "$P/forcecap"; echo FINI
