#!/bin/bash
P="/c/Users/benbo/Documents/My Games/ZimniBench/profile/ZimniPerf"
Z="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti"
B=staging/tex_backup_avant1024; N=staging/tex_1024
cd /c/Users/benbo/tools/PakTools
: > "$P/cull"; : > "$P/cullmode"; echo 4 > "$P/streambudget"; touch "$P/streamconsole"; echo 1300 > "$P/streamr"; rm -f "$P/streamfps" "$P/inradius" "$P/capval"
run(){ powershell -NoProfile -ExecutionPolicy Bypass -File vram_sampler.ps1 -Out "C:\Users\benbo\tools\PakTools\analyses\vram_$1.txt" & SP=$!
  TMIN=30 bash zimni_server_series.sh "$1:$2::1" 2>&1 | grep -E "pack|VRAM|KICK"; kill $SP 2>/dev/null
  awk -F';' -v L=$1 'NR>2{n++; f+=$5; if($7>mm)mm=$7} END{printf "%s n=%d fps=%.1f memJeu=%d\n",L,n,f/n,mm}' "$P/$1.csv"; }
run t1024_sites textures.txt
run t1024_sol vol_sol.txt
[ -d $N ] && { echo "staging tex_1024 deja present, arret"; exit 1; }
mkdir -p $N && mv "$Z/Assets" $N/ && cp "$Z/guids.json" $N/ && cp -r $B/Assets "$Z/" && cp $B/guids.json "$Z/" && echo "textures d'avant remises"
run tcur_sites textures.txt
run tcur_sol vol_sol.txt
rm -rf "$Z/Assets" && mv $N/Assets "$Z/" && cp $N/guids.json "$Z/" && rmdir $N 2>/dev/null; rm -f $N/guids.json; rmdir $N 2>/dev/null
echo "version 1024 remise: $(du -sh "$Z/Assets" | cut -f1)"; rm -f "$P/streamconsole"; echo TEX_FINI
