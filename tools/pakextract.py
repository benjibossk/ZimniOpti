"""Extracteur .pak Arma Reforger (PAC1) en lecture seule, sans dependance.
Usage: python -I pakextract.py <sortie> <data.pak> [data001.pak ...] [--only ext1,ext2]"""
import struct,sys,zlib,os
def files(pak):
    f=open(pak,'rb'); assert f.read(4)==b'FORM'; f.read(4); f.read(4); c={}
    while True:
        h=f.read(8)
        if len(h)<8: break
        c[h[:4]]=(f.tell(),struct.unpack('>I',h[4:])[0]); f.seek(c[h[:4]][1],1)
    o,s=c[b'FILE']; f.seek(o); b=f.read(s); p=0; res=[]
    def ent(path):
        nonlocal p
        t=b[p]; l=b[p+1]; n=b[p+2:p+2+l].decode('utf8','replace'); p+=2+l
        if t==0:
            k=struct.unpack_from('<I',b,p)[0]; p+=4
            for _ in range(k): ent(path+n+'/')
        else:
            off,cs,us=struct.unpack_from('<III',b,p); res.append((path+n,off,cs,us,b[p+18])); p+=24
    ent(''); return f,res
args=sys.argv[1:]; only=None
if '--only' in args: i=args.index('--only'); only=tuple('.'+e for e in args[i+1].split(',')); del args[i:i+2]
out=args[0]; n=0; sz=0
for pak in args[1:]:
    f,res=files(pak)
    for path,off,cs,us,comp in res:
        if only and not path.lower().endswith(only): continue
        f.seek(off); d=f.read(cs)
        if comp: d=zlib.decompress(d)
        dst=os.path.join(out,path.lstrip('/')); os.makedirs(os.path.dirname(dst),exist_ok=True)
        open(dst,'wb').write(d); n+=1; sz+=len(d)
print(f'{n} fichiers, {sz/2**20:.1f} MiB -> {out}')
