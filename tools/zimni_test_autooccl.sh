#!/bin/bash
# Bench AutoOccluders 0 puis 1 sur le terrain du fork ZimniOpti, puis remet l'original.
L="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti/World/Zimnitrita/Zimnitrita_Layers/default.layer"
cp "$L" "$L.orig"
for v in 0 1; do
  cp "$L.orig" "$L"
  sed -i "s/^ LayerPreset \"Terrain\"$/ LayerPreset \"Terrain\"\n AutoOccluders $v/" "$L"
  grep -c "AutoOccluders $v" "$L"
  powershell -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\benbo\tools\PakTools\zimni_pack.ps1' | grep EXIT
  powershell -NoProfile -ExecutionPolicy Bypass -File 'C:\Users\benbo\tools\PakTools\zimni_bench_run.ps1' -TimeoutMin 20 -Label autoocc$v
done
cp "$L.orig" "$L" && rm "$L.orig"
