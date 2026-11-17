#!/bin/bash
# A/B « console bridée + combat » : carte actuelle (chunkall 1300) vs apparition à la demande (rayon 1300)
# client : plafond de vue Series S (forcecap), CPU bridé (AFF, défaut 63 = 6 threads) ; serveur : 8 joueurs virtuels ; #war 8 escouades, 4 véhicules, explosions/20 s
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
M="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti"
W="$M/World/Zimnitrita"; T=/c/Users/benbo/tools/PakTools/staging
export AFF=${AFF:-63} TMIN=40
logs() { L=$(ls -dt "/c/Users/benbo/Documents/My Games/ZimniBench/logs"/logs_*/ | head -1); S=$(ls -dt "/c/Users/benbo/Documents/My Games/ZimniBench/srvprofile/logs"/logs_*/ | head -1)
  echo "-- client:"; grep -hE "\[ZO\] (STREAM|CHUNK purge)|VM Exception" "$L/console.log" | head -6 | cut -c1-200
  echo "-- serveur:"; grep -hE "\[ZO\] STREAM|VM Exception" "$S/console.log" | head -4 | cut -c1-200; }
res() { for l in "$@"; do echo "== $l"; awk -F';' 'NR>1{print $1,$5,$6,$7}' "$P/$l.csv"; done; }
# --- A : carte actuelle
echo 1300 > "$P/cull"; echo chunkall > "$P/cullmode"; rm -f "$P/streamr"
bash zimni_server_series.sh "sb_ref1:villes_war.txt::1:8::" 2>&1 | grep -E "VRAM|pack|FIN"; logs
bash zimni_server_series.sh "sb_ref2:villes_war.txt::1:8::" 2>&1 | grep -E "VRAM|FIN"
res sb_ref1 sb_ref2
# --- B : apparition à la demande (données lues DANS le mod)
: > "$P/cull"; : > "$P/cullmode"; echo 1300 > "$P/streamr"; rm -f "$P/streamprofile"
rm -rf "$W/Zimnitrita_Layers" && cp -r "$T/World_stream/Zimnitrita_Layers" "$T/World_stream/default.layer" "$W/" && mkdir -p "$M/Stream" && cp "$T/Stream_data/"* "$M/Stream/"
echo "B en place : calques $(du -sh "$W" | cut -f1), tuiles $(ls "$M/Stream" | wc -l)"
bash zimni_server_series.sh "sb_str1:villes_war.txt::1:8::" 2>&1 | grep -E "VRAM|pack|FIN"; logs
bash zimni_server_series.sh "sb_str2:villes_war.txt::1:8::" 2>&1 | grep -E "VRAM|FIN"
res sb_str1 sb_str2
# --- remise en état : carte complète, pas de données dans le mod
rm -rf "$W/Zimnitrita_Layers" "$M/Stream" && cp -r "$T/World_avant_stream/Zimnitrita_Layers" "$T/World_avant_stream/default.layer" "$W/"
rm -f "$P/streamr"; : > "$P/cull"; : > "$P/cullmode"
echo "remis : calques $(du -sh "$W" | cut -f1), Stream absent: $([ -d "$M/Stream" ] && echo NON || echo oui)"
