# Usage: python -I zimni_prefab_lod.py <extract> <dest_addon> <scale>
# Copie les prefabs ciblés dans dest (même chemin + .meta au GUID d'origine) en ajoutant PixelSizeScale au MeshObject.
import sys,os,re
ext,dst,scale=sys.argv[1],sys.argv[2],sys.argv[3]
T={"8E11620964565875":"Prefabs/Structures/Rails/RailDRoit19m01.et",
   "3BB3477130C8CB5D":"Prefabs/Structures/Rails/RailDroit09m1.et",
   "7C8780D6B345D856":"Prefabs/Props/construction/set01/Palette_Parpaing_01.et",
   "A6C692CB8E181EAD":"Prefabs/Props/construction/set01/Road_PlasticBlock_01.et",
   "5065DCC9B9245271":"Prefabs/Props/Civilian/RoadBlock/RoadBlockEnd2m.et"}
for g,p in T.items():
    src=open(os.path.join(ext,p),'rb').read().decode('utf-8')
    nl='\r\n' if '\r\n' in src else '\n'
    m=re.search(r'( *)Object "\{[0-9A-F]{16}\}[^"]+\.xob"',src)
    assert m and 'PixelSizeScale' not in src,p
    i=src.find(nl,m.end())
    out=src[:i]+nl+m.group(1)+'PixelSizeScale '+scale+src[i:]
    d=os.path.join(dst,p);os.makedirs(os.path.dirname(d),exist_ok=True)
    open(d,'wb').write(out.encode('utf-8'))
    cfg=''.join(f' EntityTemplateResourceClass {k} : PC {{}}{nl}' for k in['XBOX_ONE','XBOX_SERIES','PS4','PS5','HEADLESS'])
    open(d+'.meta','wb').write(f'MetaFileClass {{{nl} Name "{{{g}}}{p}"{nl} Configurations {{{nl} EntityTemplateResourceClass PC {{}}{nl}{cfg} }}{nl}}}{nl}'.encode())
    print('OK',p)
