# Usage: python -I zimni_furn_static.py <furn.json> <dest_addon>
# Mobilier des maisons : surcharge (même GUID) sans RplComponent ni SCR_DestructionMultiPhaseComponent
# -> plus répliqué, plus destructible. Exclus : actions, inventaire, lumière, hiérarchie, slots.
import sys,os,re,json
R='C:/Users/benbo/tools/PakInspector/'
EXT=[R+'base_et_conf_extract',R+'minusbuildingpack_extract',R+'zimnitrita_extract']
furn=json.load(open(sys.argv[1])); dst=sys.argv[2]
files={};guids={}
ref=re.compile(r'"\{([0-9A-F]{16})\}((?:Prefabs|PrefabLibrary)/[^"]+?\.et)"')
for e in EXT:
    for dp,_,fs in os.walk(e):
        for f in fs:
            p=os.path.join(dp,f).replace(os.sep,'/')
            if f.endswith('.et'):
                rel=p.split(e+'/',1)[1]
                for k in('Prefabs/','PrefabLibrary/'):
                    if k in rel: rel=rel[rel.find(k):];break
                files[rel]=p
                for g,q in ref.findall(open(p,encoding='utf-8',errors='ignore').read()): guids.setdefault(q,g)
            if f.endswith('.et.meta'):
                m=re.search(r'Name "\{([0-9A-F]{16})\}([^"]+\.et)"',open(p,encoding='utf-8',errors='ignore').read())
                if m: guids[m.group(2)]=m.group(1)
BAD=re.compile(r'^\s*\w*(?:ActionsManager|Inventory|Light|Hierarchy|SlotManager|Spawner|Door|Sound|Compartment)\w*\s+"\{',re.M)
DROP=('RplComponent','SCR_DestructionMultiPhaseComponent')
todo=set(furn)
# ajouter les parents (chaîne) qui portent eux aussi Rpl/Destruction
for rel in list(todo):
    r=rel
    for _ in range(15):
        if r not in files: break
        m=re.match(r'\s*\w+\s*:\s*"\{[0-9A-F]{16}\}([^"]+\.et)"',open(files[r],encoding='utf-8',errors='ignore').read())
        if not m: break
        r=m.group(1)
        if 'Prefabs/Props/' in r or 'Furniture' in r: todo.add(r)
n=0;skip=0
for rel in sorted(todo):
    if rel not in files or rel not in guids: skip+=1; continue
    s=open(files[rel],'rb').read().decode('utf-8','ignore')
    if BAD.search(s): skip+=1; continue
    lines=s.split('\n');res=[];d=0;cut=None
    for l in lines:
        t=l.strip()
        if cut is None and d==2 and t.split(' ')[0] in DROP and t.endswith('{'):
            cut=d
        if cut is not None:
            d+=t.count('{')-t.count('}')
            if d<=cut: cut=None
            continue
        if d==2 and t.split(' ')[0] in DROP and t.endswith('}'):  # bloc sur une ligne
            continue
        d+=t.count('{')-t.count('}'); res.append(l)
    o='\n'.join(res)
    if o==s: continue
    nl='\r\n' if '\r\n' in s else '\n'
    p=os.path.join(dst,rel);os.makedirs(os.path.dirname(p),exist_ok=True)
    open(p,'wb').write(o.encode('utf-8'))
    cfg=''.join(f' EntityTemplateResourceClass {k} : PC {{}}{nl}' for k in['XBOX_ONE','XBOX_SERIES','PS4','PS5','HEADLESS'])
    open(p+'.meta','wb').write(f'MetaFileClass {{{nl} Name "{{{guids[rel]}}}{rel}"{nl} Configurations {{{nl} EntityTemplateResourceClass PC {{}}{nl}{cfg} }}{nl}}}{nl}'.encode())
    n+=1
print('ecrits',n,'ignores',skip)
