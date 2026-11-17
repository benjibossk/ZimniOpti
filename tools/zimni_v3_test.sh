#!/bin/bash
M="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti"; W="$M/World/Zimnitrita"; S="$M/Scripts/Game/ZimniOpti"; T=staging
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
cd /c/Users/benbo/tools/PakTools
rm -rf "$W/Zimnitrita_Layers" "$M/Stream" && cp -r "$T/World_stream_v3/Zimnitrita_Layers" "$T/World_stream_v3/default.layer" "$W/" && mkdir -p "$M/Stream" && cp "$T/Stream_conf_v3/"*.conf "$T/Stream_conf_v3/"*.conf.meta "$M/Stream/" && cp "$T/Stream_conf_v3/ZO_StreamIndex.c" "$S/ZO_StreamIndex.c" && echo "v3 en place"
: > "$P/cull"; : > "$P/cullmode"; echo 4 > "$P/streambudget"; touch "$P/streamconsole"; echo 1300 > "$P/streamr"
for v in v3_sol:vol_sol.txt v3_vol:vol.txt; do
  TMIN=30 bash zimni_server_series.sh "$v::1" 2>&1 | grep -E "pack|VRAM|KICK"
  L=$(ls -dt "/c/Users/benbo/Documents/My Games/ZimniBench/logs"/logs_*/ | head -1)
  echo "runtime destructibles: $(grep -c "SCR_DestructibleEntity'.*spawned at runtime" "$L/console.log") murs, $(grep -c "'Tree'.*spawned at runtime" "$L/console.log") arbres ; erreurs ZO: $(grep -c 'SCRIPT *(E).*ZO' "$L/console.log")"
  lab=${v%%:*}; awk -F';' -v L=$lab 'NR>2{n++; f+=$5; if($6>50)s++; if($7>mm)mm=$7} END{printf "%s n=%d fps=%.1f >50ms=%d memJeu=%d\n",L,n,f/n,s,mm}' "$P/$lab.csv"
done
rm -f "$P/streamconsole"
bash zimni_stream_deploy.sh on && echo "version publiable remise"; echo V3_FINI
