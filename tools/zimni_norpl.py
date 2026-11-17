# Usage: python -I zimni_norpl.py <extract> <dest_addon> <rel.et>...
# Copie les prefabs de la carte dans dest (même chemin, .meta au GUID d'origine) SANS leur RplComponent
# (objets non destructibles : la réplication réseau ne sert à rien et charge serveur + clients).
import sys,os,re,glob
ext,dst,rels=sys.argv[1],sys.argv[2],sys.argv[3:]
guids={}
for f in glob.glob(os.path.join(ext,'World','**','*.layer'),recursive=True):
    for g,p in re.findall(r'"\{([0-9A-F]{16})\}(Prefabs/[^"]+\.et)"',open(f,encoding='utf-8',errors='ignore').read()):
        guids.setdefault(p,g)
for p in rels:
    if p not in guids: print('SANS GUID',p); continue
    src=open(os.path.join(ext,p),'rb').read().decode('utf-8')
    nl='\r\n' if '\r\n' in src else '\n'
    out,n=re.subn(r'\n *RplComponent "\{[0-9A-F]{16}\}" \{\s*\}','',src)
    if n!=1: print('IGNORE (Rpl non vide ou absent)',p); continue
    d=os.path.join(dst,p);os.makedirs(os.path.dirname(d),exist_ok=True)
    open(d,'wb').write(out.encode('utf-8'))
    cfg=''.join(f' EntityTemplateResourceClass {k} : PC {{}}{nl}' for k in['XBOX_ONE','XBOX_SERIES','PS4','PS5','HEADLESS'])
    open(d+'.meta','wb').write(f'MetaFileClass {{{nl} Name "{{{guids[p]}}}{p}"{nl} Configurations {{{nl} EntityTemplateResourceClass PC {{}}{nl}{cfg} }}{nl}}}{nl}'.encode())
    print('OK',p)
