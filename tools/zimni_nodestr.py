# Usage: python -I zimni_nodestr.py <dest_addon> [--list]
# Murs/clôtures : surcharge (même GUID) de tous les prefabs SCR_DestructibleEntity qui descendent de Wall_Base,
# remis en StaticModelEntity (classe d'origine de Wall_Base) -> plus destructibles, plus répliqués.
# On saute portails/portes (actions) et générateurs.
import sys,os,re
R='C:/Users/benbo/tools/PakInspector/'
EXT=[R+'base_et_conf_extract',R+'minusbuildingpack_extract',R+'zimnitrita_extract']
dst=sys.argv[1]; only_list='--list' in sys.argv
ref=re.compile(r'"\{([0-9A-F]{16})\}((?:Prefabs|PrefabLibrary)/[^"]+?\.et)"')
files={}; guids={}
for e in EXT:
    for dp,_,fs in os.walk(e):
        for f in fs:
            p=os.path.join(dp,f).replace(os.sep,'/')
            if f.endswith('.et'):
                rel=p.split(e+'/',1)[1]
                for k in ('Prefabs/','PrefabLibrary/'):
                    if k in rel: rel=rel[rel.find(k):]; break
                files[rel]=p
            if f.endswith(('.et','.layer','.conf','.ent')):
                for g,q in ref.findall(open(p,encoding='utf-8',errors='ignore').read()): guids.setdefault(q,g)
            if f.endswith('.et.meta'):
                m=re.search(r'Name "\{([0-9A-F]{16})\}([^"]+\.et)"',open(p,encoding='utf-8',errors='ignore').read())
                if m: guids[m.group(2)]=m.group(1)
head=re.compile(r'^\s*(\w+)(?:\s*:\s*"\{[0-9A-F]{16}\}([^"]+)")?\s*\{')
def info(rel):
    s=open(files[rel],'rb').read().decode('utf-8','ignore')
    m=head.match(s); return (m.group(1),m.group(2),s) if m else (None,None,s)
memo={}
def walls(rel,d=0):
    if rel in memo: return memo[rel]
    if rel not in files or d>20: return False
    c,par,_=info(rel)
    r= rel=='Prefabs/Structures/Core/Wall_Base.et' or (par is not None and walls(par,d+1))
    memo[rel]=r; return r
KEEP=('ID','components','placement','Flags','coords','angles','scale')
out=0
for rel in sorted(files):
    if rel=='Prefabs/Structures/Core/Wall_Base.et' or not walls(rel): continue
    c,par,s=info(rel)
    if c!='SCR_DestructibleEntity': continue
    if re.search(r'ActionsManager|Door|Gate|RplComponent|SlotManager|Hierarchy',s) or '/WG_' in rel: continue
    if rel not in guids: print('SANS GUID',rel); continue
    out+=1
    if only_list: print(rel); continue
    nl='\r\n' if '\r\n' in s else '\n'
    lines=s.split('\n'); res=[]; depth=0; skip=False
    for i,l in enumerate(lines):
        t=l.strip()
        if i==0: l=l.replace('SCR_DestructibleEntity','StaticModelEntity',1)
        elif depth==1 and not skip and t and t!='}':
            key=t.split()[0]
            if key not in KEEP: skip=(t.endswith('{') and t.count('{')>t.count('}')); sd=depth;
            if key not in KEEP:
                depth+=t.count('{')-t.count('}'); continue
        elif skip:
            depth+=t.count('{')-t.count('}')
            if depth<=1: skip=False
            continue
        depth+=t.count('{')-t.count('}'); res.append(l)
    d=os.path.join(dst,rel); os.makedirs(os.path.dirname(d),exist_ok=True)
    open(d,'wb').write('\n'.join(res).encode('utf-8'))
    cfg=''.join(f' EntityTemplateResourceClass {k} : PC {{}}{nl}' for k in['XBOX_ONE','XBOX_SERIES','PS4','PS5','HEADLESS'])
    open(d+'.meta','wb').write(f'MetaFileClass {{{nl} Name "{{{guids[rel]}}}{rel}"{nl} Configurations {{{nl} EntityTemplateResourceClass PC {{}}{nl}{cfg} }}{nl}}}{nl}'.encode())
print('PREFABS',out)
