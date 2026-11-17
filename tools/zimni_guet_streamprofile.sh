#!/bin/bash
# Guetteur : quand la phase B (ln_str) de zimni_nuit_longue.sh est en place, fournit les tuiles par le profil (client + serveur)
M="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti"
B="/c/Users/benbo/Documents/My Games/ZimniBench"
T=/c/Users/benbo/tools/PakTools/staging/Stream_data
until [ -d "$M/Stream" ]; do sleep 10; done
sleep 5
for p in "$B/profile/ZimniPerf" "$B/srvprofile/profile/ZimniPerf"; do
  rm -rf "$p/stream"; mkdir -p "$p/stream"
  for f in "$T"/*.json; do cp "$f" "$p/stream/"; done
  touch "$p/streamprofile"
  echo "$p : $(ls "$p/stream" | wc -l) fichiers, streamprofile pose"
done
# retrait des streamprofile une fois la phase B finie (Stream retiré du mod)
while [ -d "$M/Stream" ]; do sleep 10; done
rm -f "$B/profile/ZimniPerf/streamprofile" "$B/srvprofile/profile/ZimniPerf/streamprofile"; echo "streamprofile retire"
