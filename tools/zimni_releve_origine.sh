#!/bin/bash
# F24 étape 1-2 : relevé en jeu avec les calques d'ORIGINE (streaming off), puis remise en streaming on.
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
M="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti"; W="$M/World/Zimnitrita"
B=/c/Users/benbo/tools/PakTools/BACKUP_layers_20261109/Zimnitrita; O=/c/Users/benbo/tools/PakTools/staging/releve_origine
mkdir -p "$O/ancien"; cp "$P/streamdump.bin" "$P/streamdump_prefabs.txt" "$O/ancien/" 2>/dev/null
bash zimni_stream_deploy.sh off | tail -1
rm -rf "$W/Zimnitrita_Layers" && cp -r "$B/Zimnitrita_Layers" "$B/default.layer" "$W/" && echo "calques origine : $(du -sh "$W" | cut -f1)"
rm -f "$P/streamdump.bin"; touch "$P/streamdump"; : > "$P/streamr"
TMIN=15 bash zimni_server_series.sh "releve:p1.txt:::::" 2>&1 | tail -2
rm -f "$P/streamdump"
L=$(ls -dt "/c/Users/benbo/Documents/My Games/ZimniBench/logs"/logs_*/ | head -1)
grep -h "STREAM releve" "$L/console.log" | cut -c1-160
cp "$P/streamdump.bin" "$P/streamdump_prefabs.txt" "$O/" && ls -la "$O/streamdump.bin"
echo 1300 > "$P/streamr"; bash zimni_stream_deploy.sh on | tail -1
echo RELEVE_FINI
