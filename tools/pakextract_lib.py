import struct
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
