"""Genere dans ZimniOpti les .edds 4096 reduits en 2048 + guids.json (chemin -> GUID d'origine).
Usage: python -I zimni_textures.py <extract> <ZimniOpti> [filtre_chemin]"""
import sys,os,re,glob,json,struct
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
from edds_dropmip import dropmip
X,Z=sys.argv[1],sys.argv[2]; flt=sys.argv[3] if len(sys.argv)>3 else ''
g={}
for e in glob.glob(X+'/**/*.emat',recursive=True)+glob.glob(X+'/**/*.et',recursive=True):
    for m in re.finditer(r'\{([0-9A-F]{16})\}([^"]+\.edds)',open(e,encoding='utf8',errors='replace').read()): g[m.group(2)]=m.group(1)
gp=os.path.join(Z,'guids.json'); done=json.load(open(gp)) if os.path.exists(gp) else {}
n=gain=0
for p in glob.glob(X+'/**/*.edds',recursive=True):
    rel=os.path.relpath(p,X).replace(os.sep,'/')
    if flt not in rel or rel not in g: continue
    b=open(p,'rb').read(); h,w=struct.unpack_from('<II',b,12)
    if max(w,h)<4096: continue
    out,_=dropmip(b,1); d=os.path.join(Z,rel); os.makedirs(os.path.dirname(d),exist_ok=True)
    open(d,'wb').write(out); done[rel]=g[rel]; n+=1; gain+=w*h*(1 if b[84:88]==b'DX10' else 0.5)
json.dump(done,open(gp,'w'),indent=1)
print(f'{n} textures reduites, VRAM gagnee ~{gain/2**20:.0f} MiB ; guids.json: {len(done)} entrees')
