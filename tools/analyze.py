"""Analyse de paks Arma Reforger : inventaire, cout VRAM des textures (en-tetes .edds), entites des .layer.
Usage: python -I analyze.py <nom> <sortie_dir> [--no-layers] <pak> [pak...]
Ecrit <nom>_files.json, <nom>_textures.csv, <nom>_entities.csv, <nom>_resume.txt"""
import struct,sys,zlib,os,re,collections,csv
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
from pakextract_lib import files as pakfiles
DX={71:4,72:4,74:8,75:8,77:8,78:8,80:4,81:4,83:8,84:8,95:8,96:8,98:8,99:8,28:32,29:32,87:32,88:32,91:32,93:32,61:8,62:8,49:16,56:16,10:64,2:128,24:32,41:32,42:32,43:32,34:16,35:16,54:16,57:16,11:64,16:64,33:32,26:32}
FOURCC={'DXT1':4,'DXT3':8,'DXT5':8,'ATI1':4,'BC4U':4,'ATI2':8,'BC5U':8}
args=sys.argv[1:]; name,out=args[0],args[1]; args=args[2:]; layers=True
if '--no-layers' in args: args.remove('--no-layers'); layers=False
ents=[]
for pak in args:
    f,res=pakfiles(pak); ents+=[(f,pak)+r for r in res]
def rd(e,limit=0):
    f=e[0]; f.seek(e[3]); b=f.read(e[4] if not limit else min(e[4],65536))
    if e[6]: return zlib.decompressobj().decompress(b,limit) if limit else zlib.decompress(b)
    return b[:limit] if limit else b
L=[]
ext=collections.Counter();cnt=collections.Counter()
for e in ents:
    x=e[2].rsplit('.',1)[-1].lower(); ext[x]+=e[5]; cnt[x]+=1
tex=[];unk=collections.Counter()
for e in ents:
    if not e[2].lower().endswith('.edds'): continue
    try:
        b=rd(e,160); h,w,m=struct.unpack_from('<III',b,12); m=struct.unpack_from('<I',b,28)[0]
        cc=b[84:88].decode('latin1')
        if cc=='DX10': d=struct.unpack_from('<I',b,128)[0]; fmt=f'DXGI{d}'; bpp=DX.get(d)
        elif cc.strip('\0'): fmt=cc; bpp=FOURCC.get(cc)
        else: bpp=struct.unpack_from('<I',b,88)[0]; fmt=f'RGB{bpp}'
        if not bpp: unk[fmt]+=1; bpp=8
        v=0;x,y=w,h
        for i in range(max(m,1)): v+=max(x,4)*max(y,4)*bpp/8; x=max(x//2,1); y=max(y//2,1)
        tex.append((e[2],w,h,max(m,1),fmt,round(v),e[5]))
    except Exception as ex: unk['ERR']+=1
with open(f'{out}/{name}_textures.csv','w',newline='',encoding='utf8') as fo:
    c=csv.writer(fo); c.writerow(['path','w','h','mips','format','vram_bytes','raw_bytes']); c.writerows(sorted(tex,key=lambda t:-t[5]))
inst=[]
if layers:
    for e in ents:
        if not e[2].endswith('.layer'): continue
        t=rd(e).decode('utf8','replace'); depth=0; grp=False; pref=cls=None; cur=None; idepth=None
        for line in t.split('\n'):
            s=line.strip()
            if depth==0 and s.endswith('{'):
                grp=s.startswith('$grp'); hm=re.match(r'(?:\$grp\s+)?(\w+)[^"]*(?:"\{[0-9A-F]+\}([^"]+)")?',s)
                cls=hm.group(1); pref=hm.group(2) or '(inline)'
                if not grp: cur=[cls,pref,None,e[2]]; inst.append(cur); idepth=1
            elif grp and depth==1 and s.startswith('{'):
                cur=[cls,pref,None,e[2]]; inst.append(cur); idepth=2
            elif cur is not None and cur[2] is None and depth==idepth and s.startswith('coords '):
                try: cur[2]=tuple(float(v) for v in s.split()[1:4])
                except: pass
            depth+=line.count('{')-line.count('}')
    with open(f'{out}/{name}_entities.csv','w',newline='',encoding='utf8') as fo:
        c=csv.writer(fo); c.writerow(['class','prefab','x','y','z','layer'])
        for i in inst: c.writerow([i[0],i[1]]+(list(i[2]) if i[2] else ['','',''])+[i[3]])
def cat(p,c):
    p=p.lower()
    for k,v in (('vegetation','vegetation'),('tree','vegetation'),('bush','vegetation'),('rock','rochers'),('generator','generateurs'),('decal','decals'),('light','lumieres'),('sound','sons'),('house','batiments'),('structure','batiments'),('building','batiments'),('wall','murs/clotures'),('fence','murs/clotures'),('prop','props')):
        if k in p: return v
    return 'autre:'+c
R=[]
R.append(f'== {name} : {len(ents)} fichiers ==')
for x,v in ext.most_common(12): R.append(f'{x:8} {cnt[x]:7} {v/2**20:9.1f} MiB (decompresse)')
tv=sum(t[5] for t in tex); R.append(f'\nTextures: {len(tex)} ; VRAM si tout resident {tv/2**30:.2f} GiB ; formats inconnus {dict(unk)}')
rs=collections.Counter();rc=collections.Counter()
for t in tex: k=max(t[1],t[2]); rs[k]+=t[5]; rc[k]+=1
for k in sorted(rs,reverse=True)[:6]: R.append(f'  {k:6}px x{rc[k]:5} = {rs[k]/2**20:8.1f} MiB')
dd=collections.Counter()
for t in tex: dd['/'.join(t[0].split('/')[:4])]+=t[5]
R.append('  Par dossier:'); R+= [f'  {v/2**20:8.1f} MiB {k}' for k,v in dd.most_common(12)]
if inst:
    R.append(f'\nEntites (instances top-niveau): {len(inst)} ; sans coords: {sum(1 for i in inst if not i[2])}')
    cc=collections.Counter(cat(i[1],i[0]) for i in inst); R+= [f'  {v:7} {k}' for k,v in cc.most_common(12)]
    R.append('  Prefabs les plus poses:'); R+=[f'  {v:7} {k}' for k,v in collections.Counter(i[1] for i in inst).most_common(15)]
    g=collections.Counter((int(i[2][0]//1000),int(i[2][2]//1000)) for i in inst if i[2])
    R.append(f'  Densite par carre de 1 km ({len(g)} carres, moyenne {sum(g.values())/max(len(g),1):.0f}) - top 12 :')
    for (gx,gz),v in g.most_common(12):
        sub=collections.Counter(cat(i[1],i[0]) for i in inst if i[2] and int(i[2][0]//1000)==gx and int(i[2][2]//1000)==gz).most_common(3)
        R.append(f'  X{gx}km Z{gz}km : {v:6}  {sub}')
open(f'{out}/{name}_resume.txt','w',encoding='utf8').write('\n'.join(R)); print('\n'.join(R))
