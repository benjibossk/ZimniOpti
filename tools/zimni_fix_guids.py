"""Apres 'Register' dans le Workbench : remet le GUID d'origine (guids.json) dans chaque .edds.meta de ZimniOpti.
Usage: python -I zimni_fix_guids.py <ZimniOpti>   (Workbench FERME ou ressources fermees)"""
import sys,os,json,re
Z=sys.argv[1]; g=json.load(open(os.path.join(Z,'guids.json'))); ok=miss=0
for rel,guid in g.items():
    mp=os.path.join(Z,rel+'.meta')
    if not os.path.exists(mp): miss+=1; print('pas de meta:',rel); continue
    t=open(mp,encoding='utf8').read()
    t2=re.sub(r'Name "\{[0-9A-F]{16}\}',f'Name "{{{guid}}}',t,count=1)
    if t2!=t: open(mp,'w',encoding='utf8').write(t2)
    ok+=1
print(f'{ok} meta au bon GUID, {miss} sans meta')
