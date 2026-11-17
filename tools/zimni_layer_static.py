# Usage: python -I zimni_layer_static.py <World_dir> [--dry]
# Dans les calques : les murs/clôtures posés en SCR_DestructibleEntity -> StaticModelEntity
# (non destructibles, non répliqués). Portails/portes exclus. La classe du calque l'emporte sur le prefab.
import sys,os,re,collections
W=sys.argv[1]; dry='--dry' in sys.argv
pat=re.compile(r'^(\s*(?:\$grp\s+)?)SCR_DestructibleEntity(\s*:\s*"\{[0-9A-F]{16}\}([^"]*?(?:/Walls/|/walls/|WoodenWall|PoleFence|StoneWall|ConcreteWall|NetFence|BrickWall|PipeRailing)[^"]*\.et)")',re.M)
EXC=re.compile(r'Gate|Door|Wicket',re.I)
tot=collections.Counter()
def rep(m):
    if EXC.search(m.group(3)): return m.group(0)
    tot[m.group(3).split('/')[-1]]+=1
    return m.group(1)+'StaticModelEntity'+m.group(2)
for dp,_,fs in os.walk(W):
    for f in fs:
        if not f.endswith('.layer'): continue
        p=os.path.join(dp,f); s=open(p,'rb').read().decode('utf-8')
        o=pat.sub(rep,s)
        if o!=s and not dry: open(p,'wb').write(o.encode('utf-8'))
print('instances',sum(tot.values()),'prefabs',len(tot))
for k,v in tot.most_common(8): print(v,k)
