"""Réduit en 1024 les textures de bâtiments 2048 non encore réduites (liste zimnitrita_textures_batiments.csv)."""
import sys,os,json,re,glob,struct
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
from edds_dropmip import dropmip
X,Z,C=sys.argv[1],sys.argv[2],sys.argv[3]
g={}
for e in glob.glob(X+'/**/*.emat',recursive=True)+glob.glob(X+'/**/*.et',recursive=True):
    for m in re.finditer(r'\{([0-9A-F]{16})\}([^"]+\.edds)',open(e,encoding='utf8',errors='replace').read()): g[m.group(2)]=m.group(1)
gp=os.path.join(Z,'guids.json'); done=json.load(open(gp))
n=gain=miss=0
for l in open(C,encoding='utf-8-sig').read().splitlines()[1:]:
    c=l.split(';')
    if c[6]!='non' or c[3]!='2048x2048': continue
    rel=c[8].lstrip('/')
    p=os.path.join(X,rel)
    if rel not in g or not os.path.exists(p): miss+=1; continue
    b=open(p,'rb').read(); out,_=dropmip(b,1)
    d=os.path.join(Z,rel); os.makedirs(os.path.dirname(d),exist_ok=True); open(d,'wb').write(out)
    done[rel]=g[rel]; n+=1; gain+=float(c[5])*0.75
json.dump(done,open(gp,'w'),indent=1)
print(f'{n} textures 2048->1024, gain max ~{gain:.0f} Mo, ignorees {miss}')
