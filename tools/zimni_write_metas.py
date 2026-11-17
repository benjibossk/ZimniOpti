"""Ecrit un .meta EDDSResourceClass au GUID d'origine pour chaque .edds liste dans guids.json.
Usage: python -I zimni_write_metas.py <ZimniOpti>"""
import sys,os,json
Z=sys.argv[1]; g=json.load(open(os.path.join(Z,'guids.json'))); n=0
P=['PC','XBOX_ONE','XBOX_SERIES','PS4','PS5','HEADLESS']
for rel,guid in g.items():
    if not os.path.exists(os.path.join(Z,rel)): continue
    c='\n'.join(f'  EDDSResourceClass {p}{"" if p=="PC" else " : PC"} {{\n  }}' for p in P)
    open(os.path.join(Z,rel+'.meta'),'w',encoding='utf8',newline='\n').write(f'MetaFileClass {{\n Name "{{{guid}}}{rel}"\n Configurations {{\n{c}\n }}\n}}\n'); n+=1
print(n,'meta ecrits')
