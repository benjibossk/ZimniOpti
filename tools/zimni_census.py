import re,glob,collections,sys
cx,cz,R=float(sys.argv[1]),float(sys.argv[2]),float(sys.argv[3])
c=collections.Counter();light=0
for f in glob.glob('**/*.layer',recursive=True):
    t=open(f,encoding='utf-8',errors='ignore').read()
    # préfab courant = dernière ligne avec "{GUID}path" avant chaque coords
    last=None
    for m in re.finditer(r'"\{[0-9A-F]{16}\}([^"]+\.et)"|coords ([\d.-]+) [\d.-]+ ([\d.-]+)',t):
        if m.group(1): last=m.group(1).split('/')[-1]; continue
        x,z=float(m.group(2)),float(m.group(3))
        if abs(x-cx)<R and abs(z-cz)<R and last: c[last]+=1
print(sum(c.values()));[print(v,k) for k,v in c.most_common(400)]
