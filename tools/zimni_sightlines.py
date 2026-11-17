# Repère les longues lignes droites de route (perspectives ouvertes) qui passent en ville.
# usage: python -I zimni_sightlines.py <zimnitrita_extract/World> <out.csv> [minLen=250] [tolDeg=4]
import sys,re,glob,math,collections
root,out=sys.argv[1],sys.argv[2]
MINL=float(sys.argv[3]) if len(sys.argv)>3 else 250; TOL=float(sys.argv[4]) if len(sys.argv)>4 else 4
VEG=re.compile(r'^(b_|t_|p_|s_|f_|FG_|RG_|Granite|Rock|Limestone|Decal|T_trunk)',re.I)
dens=collections.Counter(); roads=[]
for f in glob.glob(root+'/**/*.layer',recursive=True):
    t=open(f,encoding='utf-8',errors='ignore').read()
    last=None
    for m in re.finditer(r'"\{[0-9A-F]{16}\}([^"]+\.et)"|coords ([\d.-]+) [\d.-]+ ([\d.-]+)',t):
        if m.group(1): last=m.group(1).split('/')[-1]; continue
        if last and not VEG.match(last):
            dens[(int(float(m.group(2))//100),int(float(m.group(3))//100))]+=1
    # splines : chaque "Points {" ; coords/angles = derniers vus avant ; type de route = premier RG_Road juste après
    prev=0
    for m in re.finditer(r'\n\s*Points \{',t):
        head=t[prev:m.start()]; prev=m.end()
        cs=re.findall(r'\n\s*coords ([\d.-]+) ([\d.-]+) ([\d.-]+)',head)
        if not cs: continue
        an=re.findall(r'\n\s*angles ([\d.-]+) ([\d.-]+) ([\d.-]+)',head[head.rfind('coords'):])
        i=m.end();d=1
        while d and i<len(t): d+=(t[i]=='{')-(t[i]=='}');i+=1
        rgm=re.search(r'RG_Road_[A-Za-z0-9_]+|RG_TrailDirt_\w+',t[i:i+600])
        if not rgm: continue
        yaw=math.radians(float(an[-1][1])) if an else 0
        ox,oz=float(cs[-1][0]),float(cs[-1][2])
        pts=[]
        for q in re.finditer(r'Position ([\d.e-]+) [\d.e-]+ ([\d.e-]+)',t[m.end():i]):
            x,z=float(q.group(1)),float(q.group(2))
            pts.append((ox+x*math.cos(yaw)+z*math.sin(yaw), oz-x*math.sin(yaw)+z*math.cos(yaw)))
        roads.append((rgm.group(),pts,f.split('Layers')[-1]))
res=[]
for rg,pts,lay in roads:
    s=0
    while s<len(pts)-1:
        e=s+1;h0=math.atan2(pts[e][1]-pts[s][1],pts[e][0]-pts[s][0])
        while e+1<len(pts):
            h=math.atan2(pts[e+1][1]-pts[s][1],pts[e+1][0]-pts[s][0])
            if abs((h-h0+math.pi)%(2*math.pi)-math.pi)>math.radians(TOL): break
            e+=1
        L=math.dist(pts[s],pts[e])
        if L>=MINL:
            n=max(2,int(L//50));town=0
            for k in range(n+1):
                x=pts[s][0]+(pts[e][0]-pts[s][0])*k/n; z=pts[s][1]+(pts[e][1]-pts[s][1])*k/n
                town=max(town,dens[(int(x//100),int(z//100))])
            res.append((L,town,rg,pts[s],pts[e],lay))
        s=e
res.sort(key=lambda r:-(r[0]*min(r[1],200)))
with open(out,'w') as o:
    o.write('longueur;densiteVilleMax100m;route;x1;z1;x2;z2;layer\n')
    for L,tn,rg,a,b,lay in res: o.write(f'{L:.0f};{tn};{rg};{a[0]:.0f};{a[1]:.0f};{b[0]:.0f};{b[1]:.0f};{lay}\n')
print(len(roads),'routes,',len(res),'lignes droites >=',MINL,'m ; en ville (densite>=100):',sum(r[1]>=100 for r in res))
