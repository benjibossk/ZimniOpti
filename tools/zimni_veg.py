# Végétation vs bâti par carré de 1 km + croisement avec le FPS de la grille 2 km (pire direction).
# usage: python -I zimni_veg.py <World> <g2_ref.csv>
import sys,re,glob,collections,csv
VEG=re.compile(r'^(b_|t_|p_|s_|f_|T_trunk)',re.I)
veg=collections.Counter();bat=collections.Counter();vt=collections.Counter()
for f in glob.glob(sys.argv[1]+'/**/*.layer',recursive=True):
    t=open(f,encoding='utf-8',errors='ignore').read();last=None
    for m in re.finditer(r'"\{[0-9A-F]{16}\}([^"]+\.et)"|coords ([\d.-]+) [\d.-]+ ([\d.-]+)',t):
        if m.group(1): last=m.group(1).split('/')[-1]; continue
        if not last: continue
        k=(int(float(m.group(2))//1000),int(float(m.group(3))//1000))
        if VEG.match(last):
            veg[k]+=1
            if last.lower().startswith('t_'): vt[k]+=1
        elif not re.match(r'^(FG_|RG_|Granite|Rock|Limestone|Decal)',last): bat[k]+=1
fps={}
for x in csv.reader(open(sys.argv[2]),delimiter=';'):
    if x and x[0][0]=='G':
        k=(int(x[1])//1000,int(x[2])//1000); fps[k]=min(fps.get(k,999),float(x[4]))
print('carre  fpsMin  vegetation(arbres)  bati')
for k in sorted(fps,key=fps.get)[:12]: print(f'X{k[0]}Z{k[1]}  {fps[k]:5.0f}  {veg[k]:7}({vt[k]})  {bat[k]}')
import statistics as s
ks=list(fps); 
def r(a,b): ma,mb=s.mean(a),s.mean(b); return sum((x-ma)*(y-mb) for x,y in zip(a,b))/(len(a)*s.pstdev(a)*s.pstdev(b))
print('corr fps~vegetation',round(r([fps[k] for k in ks],[veg[k] for k in ks]),2),' fps~bati',round(r([fps[k] for k in ks],[bat[k] for k in ks]),2))
