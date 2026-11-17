#!/bin/bash
# Nuit, partie 2 (après la longue session) : occluders automatiques A/B (GPU chargé : résolution x2) + grille fine 250 m des zones rouges, réglages console
cd /c/Users/benbo/tools/PakTools
until grep -q "NUIT_FINIE" analyses/nuit_longue.txt 2>/dev/null; do sleep 30; done
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
M="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti"; S="$M/Scripts/Game/ZimniOpti"
export AFF=63 TMIN=60 ENGSET="VideoUserSettings ResolutionScale f 2"
: > "$P/cull"; : > "$P/cullmode"; rm -f "$P/streamr" "$P/occl"
# installation des occluders (code inerte sans le fichier occl)
cp -r staging/occl/Prefabs "$M/" && cp staging/occl/ZO_Occluders.c staging/occl/ZO_OcclTable.c "$S/"
python -I - "$S/ZO_PropCuller.c" <<'PYEOF'
import sys; p=sys.argv[1]; s=open(p,encoding='utf-8',newline='').read()
if 'ZO_Occluders.Frame' not in s:
    s=s.replace("\t\tZO_PropCuller.Frame(world, timeslice);\n\t}","\t\tZO_PropCuller.Frame(world, timeslice);\n\t\tZO_Occluders.Frame(world, timeslice);\n\t}")
    s=s.replace("\t\t\tZO_Streamer.Reset();\n","\t\t\tZO_Streamer.Reset();\n\t\t\tZO_Occluders.Reset();\n")
open(p,'w',encoding='utf-8',newline='').write(s); print('hook', s.count('ZO_Occluders'))
PYEOF
lg() { L=$(ls -dt "/c/Users/benbo/Documents/My Games/ZimniBench/logs"/logs_*/ | head -1); grep -hE "\[ZO\] OCCL|SCRIPT +\(E\).*ZO_|VM Exception" "$L/console.log" | head -6 | cut -c1-200; }
echo "=== OCCLUDERS (villes, vue Series S, resolution x2, 6 threads)"
bash zimni_server_series2.sh "oc_off:villes.txt::1:::" 2>&1 | grep -E "VRAM|pack|FIN"; lg
touch "$P/occl"
bash zimni_server_series2.sh "oc_on:villes.txt::1:::" 2>&1 | grep -E "VRAM|pack|FIN"; lg
rm -f "$P/occl"
for l in oc_off oc_on; do echo "== $l"; awk -F';' 'NR>1{print $1,$4,$5,$6,$7}' "$P/$l.csv"; done
echo "=== GRILLE FINE 250 m (156 points des zones rouges, réglages console)"
bash zimni_server_series2.sh "fin250:fin250.txt::1:::" 2>&1 | grep -E "VRAM|pack|FIN"
echo "points: $(awk 'END{print NR-1}' "$P/fin250.csv")"
# remise en état : occluders retirés du mod (gardés dans staging/occl)
rm -rf "$M/Prefabs" "$S/ZO_Occluders.c" "$S/ZO_OcclTable.c"
python -I - "$S/ZO_PropCuller.c" <<'PYEOF'
import sys; p=sys.argv[1]; s=open(p,encoding='utf-8',newline='').read()
s=s.replace("\t\tZO_Occluders.Frame(world, timeslice);\n","").replace("\t\t\tZO_Occluders.Reset();\n","")
open(p,'w',encoding='utf-8',newline='').write(s); print('hook retire', s.count('ZO_Occluders'))
PYEOF
echo NUIT2_FINIE
