"""Genere bench_points.txt (label x z) depuis analyses/zimnitrita_entities.csv : 12 zones denses, 4 moyennes, 2 vides.
Usage: python -I zimni_bench_points.py <entities.csv> <sortie.txt>"""
import csv,sys,collections
r=[x for x in csv.DictReader(open(sys.argv[1],encoding='utf8')) if x['x']]
cells=collections.defaultdict(list)
for x in r: cells[(int(float(x['x'])//1000),int(float(x['z'])//1000))].append((float(x['x']),float(x['z'])))
s=sorted(cells.items(),key=lambda kv:-len(kv[1]))
pick=s[:12]+s[len(s)//2-2:len(s)//2+2]+s[-40:-38]
with open(sys.argv[2],'w',encoding='utf8',newline='\n') as f:
    f.write('# label x z  (genere depuis la densite d objets ; ajoute tes propres points si besoin)\n')
    for (gx,gz),pts in pick:
        cx=sum(p[0] for p in pts)/len(pts); cz=sum(p[1] for p in pts)/len(pts)
        f.write(f'X{gx}Z{gz}_n{len(pts)} {cx:.0f} {cz:.0f}\n')
print(open(sys.argv[2]).read())
