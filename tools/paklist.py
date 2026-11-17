import struct, sys, json, collections, os
def chunks(f):
    assert f.read(4)==b'FORM'; total=struct.unpack('>I',f.read(4))[0]; typ=f.read(4); print('type',typ,'form',total,file=sys.stderr)
    out={}
    while True:
        h=f.read(8)
        if len(h)<8: break
        n,s=h[:4],struct.unpack('>I',h[4:])[0]; out[n]=(f.tell(),s); f.seek(s,1)
    return out
def parse(b):
    p=0; res=[]
    def ent(path):
        nonlocal p
        t=b[p]; l=b[p+1]; name=b[p+2:p+2+l].decode('utf8','replace'); p+=2+l
        if t==0:
            c=struct.unpack_from('<I',b,p)[0]; p+=4
            for _ in range(c): ent(path+name+'/')
        elif t==1:
            off,cs,us=struct.unpack_from('<III',b,p); comp=b[p+18]; p+=24
            res.append((path+name,off,cs,us,comp))
        else: raise ValueError(f'bad type {t} at {p}')
    ent('')
    return res
allf=[]
for pak in sys.argv[2:]:
    with open(pak,'rb') as f:
        c=chunks(f); print({k.decode():v for k,v in c.items()},file=sys.stderr)
        o,s=c[b'FILE']; f.seek(o); b=f.read(s)
    r=parse(b); allf+= [(os.path.basename(pak),)+x for x in r]
json.dump(allf,open(sys.argv[1],'w'))
print('files',len(allf))
ext=collections.Counter(); cnt=collections.Counter()
for a in allf:
    e=a[1].rsplit('.',1)[-1].lower(); ext[e]+=a[4]; cnt[e]+=1
for e,v in ext.most_common(15): print(f'{e:10} {cnt[e]:7} {v/2**20:10.1f} MiB')
print('--top dirs'); d=collections.Counter()
for a in allf: d['/'.join(a[1].split('/')[:3])]+=a[4]
for k,v in d.most_common(20): print(f'{v/2**20:9.1f} MiB {k}')
print('--top files')
for a in sorted(allf,key=lambda x:-x[4])[:20]: print(f'{a[4]/2**20:8.1f} MiB {a[1]}')
