#!/bin/bash
# Variantes "nom:ao:overlay:vd" — ao=AutoOccluders (vide=original), overlay=dossier de staging copié dans ZimniOpti
# le temps du test (vide=aucun), vd=distance de vue forcée en m (vide=défaut, fichier ZimniPerf/viewdist).
# usage: zimni_series2.sh <points.txt> <timeoutMin> var1 var2 ...
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
A="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti"
L="$A/World/Zimnitrita/Zimnitrita_Layers/default.layer"
S=/c/Users/benbo/tools/PakTools
PS="powershell -NoProfile -ExecutionPolicy Bypass -File"
pts=$1; to=$2; shift 2
cp "$P/$pts" "$P/bench_points.txt"
ORIG=/c/Users/benbo/tools/PakInspector/zimnitrita_extract/World/Zimnitrita/Zimnitrita_Layers/default.layer
for v in "$@"; do
  IFS=: read nom ao ov vd es <<< "$v"
  cp "$ORIG" "$L"; rm -f "$P/viewdist" "$P/engset"
  [ -n "$es" ] && cp "$S/staging/engset/$es.txt" "$P/engset"
  [ -n "$ao" ] && python -I $S/zimni_set_ao.py "$L" "$ao"
  [ -n "$vd" ] && echo "$vd" > "$P/viewdist"
  files=""; [ -n "$ov" ] && files=$(cd "$S/staging/$ov" && find . -type f) && cp -r "$S/staging/$ov/." "$A/"
  echo "== $nom ao='$ao' overlay='$ov' vd='$vd' es='$es'"
  while tasklist | grep -qi ArmaReforgerSteamDiag; do sleep 2; done
  r=$($PS 'C:\Users\benbo\tools\PakTools\zimni_pack.ps1' | grep EXIT); echo "pack $r"
  if echo "$r" | grep -q "EXIT=0"; then $PS "$S/zimni_bench_run.ps1" -TimeoutMin $to -Label "$nom" | grep EXIT; else echo "PACK ECHOUE $nom"; fi
  for f in $files; do rm -f "$A/$f"; done
done
cp "$ORIG" "$L"; rm -f "$P/viewdist" "$P/engset"
echo FINI
