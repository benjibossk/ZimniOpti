#!/bin/bash
# attend la fin de l'A/B (ligne "remis :"), puis la longue session console
cd /c/Users/benbo/tools/PakTools
until grep -q "^remis :" analyses/stream_ab2.txt 2>/dev/null; do sleep 30; done
bash zimni_nuit_longue.sh > analyses/nuit_longue.txt 2>&1
echo NUIT_FINIE >> analyses/nuit_longue.txt
