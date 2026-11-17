#!/bin/bash
# Série de benchs : pour chaque variante "nom:valeurAutoOccluders" (vide = original), pack + bench sur <fichier points>.
# usage: zimni_series.sh <points.txt dans ZimniPerf> <timeoutMin> nom1:ao nom2:ao ...
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
L="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti/World/Zimnitrita/Zimnitrita_Layers/default.layer"
PS="powershell -NoProfile -ExecutionPolicy Bypass -File"
pts=$1; to=$2; shift 2
cp "$P/$pts" "$P/bench_points.txt"
cp /c/Users/benbo/tools/PakInspector/zimnitrita_extract/World/Zimnitrita/Zimnitrita_Layers/default.layer "$L.orig"   # toujours partir de l'original
for v in "$@"; do
  nom=${v%%:*}; ao=${v#*:}
  cp "$L.orig" "$L"
  [ -n "$ao" ] && python -I /c/Users/benbo/tools/PakTools/zimni_set_ao.py "$L" "$ao"
  echo "== $nom (AutoOccluders='$ao', lignes=$(grep -c AutoOccluders "$L"))"
  while tasklist | grep -qi ArmaReforgerSteamDiag; do sleep 2; done   # pak verrouille tant que le jeu tourne
  r=$($PS 'C:\Users\benbo\tools\PakTools\zimni_pack.ps1' | grep EXIT); echo "pack $r"
  echo "$r" | grep -q "EXIT=0" || { echo "PACK ECHOUE -> arret"; break; }
  $PS 'C:\Users\benbo\tools\PakTools\zimni_bench_run.ps1' -TimeoutMin $to -Label "$nom" | grep EXIT
done
cp "$L.orig" "$L" && rm "$L.orig"
echo FINI
