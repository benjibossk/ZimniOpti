#!/bin/bash
# Met le streaming (plan B .conf) dans le mod, ou l'en retire.
#   on  : calques allégés + Stream/*.conf + ZO_StreamIndex.c complet
#   off : calques complets + pas de Stream/ + ZO_StreamIndex.c vide (streaming inactif)
M="/c/Users/benbo/Documents/My Games/ArmaReforgerWorkbench/addons/ZimniOpti"
W="$M/World/Zimnitrita"; S="$M/Scripts/Game/ZimniOpti"; T=/c/Users/benbo/tools/PakTools/staging
case "$1" in
on)
  rm -rf "$W/Zimnitrita_Layers" "$M/Stream" && cp -r "$T/World_stream/Zimnitrita_Layers" "$T/World_stream/default.layer" "$W/"
  mkdir -p "$M/Stream" && cp "$T/Stream_conf/"*.conf "$T/Stream_conf/"*.conf.meta "$M/Stream/"
  cp "$T/Stream_conf/ZO_StreamIndex.c" "$S/ZO_StreamIndex.c" ;;
off)
  rm -rf "$W/Zimnitrita_Layers" "$M/Stream" && cp -r "$T/World_avant_stream/Zimnitrita_Layers" "$T/World_avant_stream/default.layer" "$W/"
  git -C "$M" show HEAD:Scripts/Game/ZimniOpti/ZO_StreamIndex.c > "$S/ZO_StreamIndex.c" 2>/dev/null || cp "$T/ZO_StreamIndex_vide.c" "$S/ZO_StreamIndex.c" ;;
*) echo "usage: $0 on|off"; exit 1 ;;
esac
echo "streaming $1 : calques $(du -sh "$W" | cut -f1), tuiles $(ls "$M/Stream" 2>/dev/null | grep -c '\.conf$'), index $(grep -c 'tiles.Set' "$S/ZO_StreamIndex.c") tuiles"
