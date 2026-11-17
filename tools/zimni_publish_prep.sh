#!/bin/bash
# Prépare ZimniOpti pour une publication Workshop, puis remet les outils de test.
#   bash zimni_publish_prep.sh avant   -> sort le scénario de banc (ZO_Bench.conf) du mod
#   bash zimni_publish_prep.sh apres   -> le remet (pour refaire des tests)
# Le reste du code de test est inerte chez les joueurs (il ne s'active qu'avec des fichiers $profile:ZimniPerf/*).
M="C:/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti"
S="C:/Users/benbo/tools/PakTools/staging/publish_hors_mod"
case "$1" in
  avant)
    mkdir -p "$S"
    for f in Missions/ZO_Bench.conf Missions/ZO_Bench.conf.meta; do
      [ -f "$M/$f" ] && mv "$M/$f" "$S/" && echo "sorti : $f"
    done
    [ -d "$M/Scripts/WorkbenchGame" ] && echo "ATTENTION : Scripts/WorkbenchGame est revenu dans le mod (outil MCP) - a retirer"
    grep -rln "EMCP_WB_" "$M/Scripts" 2>/dev/null && echo "ATTENTION : handlers EnfusionMCP dans le mod"
    echo "Pret a publier. Apres publication : bash zimni_publish_prep.sh apres"
    ;;
  apres)
    for f in ZO_Bench.conf ZO_Bench.conf.meta; do
      [ -f "$S/$f" ] && mv "$S/$f" "$M/Missions/" && echo "remis : Missions/$f"
    done
    ;;
  *) echo "usage : $0 avant|apres" ;;
esac
