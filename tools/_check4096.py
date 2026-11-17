import sys,struct,os,re,glob
X=sys.argv[1]; ok=bad=0; big=[]
for root,_,fs in os.walk(X):
    for f in fs:
        if not f.endswith('.edds'): continue
        p=os.path.join(root,f); b=open(p,'rb').read(260)
        h,w=struct.unpack_from('<II',b,12); m=struct.unpack_from('<I',b,28)[0]
        if max(w,h)<4096: continue
        n=0;q=148 if b[84:88]==b'DX10' else 128
        while b[q:q+4] in (b'COPY',b'LZ4 '): n+=1;q+=8
        if n==m: ok+=1
        else: bad+=1; print('mismatch',p,n,m)
        big.append(os.path.relpath(p,X).replace(os.sep,'/'))
print('4096+ ok',ok,'bad',bad)
g={}
for e in glob.glob(X+'/**/*.emat',recursive=True)+glob.glob(X+'/**/*.et',recursive=True)+glob.glob(X+'/**/*.layer',recursive=True)[:0]:
    for m in re.finditer(r'\{([0-9A-F]{16})\}([^"]+\.edds)',open(e,encoding='utf8',errors='replace').read()): g.setdefault(m.group(2),set()).add(m.group(1))
found=[p for p in big if p in g]; print('GUID trouves',len(found),'/',len(big),'conflits',[(p,g[p]) for p in found if len(g[p])>1][:3])
print('sans GUID',[p for p in big if p not in g][:10])
