# Usage: python -I zimni_thin_bushes.py <World_dir> <garder_1_sur_N> [regex_classe_et_prefab]
# regex par défaut = buissons (Tree b_...). Ex rochers : 'SCR_IndestructibleEnvironmentalEntity\s*:\s*"\{[0-9A-F]{16}\}[^"]*/Granite_Beach'
# Éclaircit les buissons (prefabs Tree dont le nom commence par b_) dans les $grp des calques :
# garde 1 instance sur N (N=2 -> -50 %). Les arbres (t_...) et le reste ne bougent pas.
import sys,os,re
W=sys.argv[1]; N=int(sys.argv[2])
PAT=sys.argv[3] if len(sys.argv)>3 else r'Tree\s*:\s*"\{[0-9A-F]{16}\}[^"]*/b_[^"/]*\.et"'
hdr=re.compile(r'^\s*\$grp\s+'+PAT+r'[^{]*\{\s*$')
kept=removed=0
for dp,_,fs in os.walk(W):
    for f in fs:
        if not f.endswith('.layer'): continue
        p=os.path.join(dp,f); lines=open(p,'rb').read().decode('utf-8').split('\n')
        out=[]; i=0; changed=False
        while i<len(lines):
            l=lines[i]; out.append(l)
            if hdr.match(l):
                d=1; i+=1; idx=0
                while i<len(lines) and d>0:
                    t=lines[i].strip()
                    if d==1 and t=='{':           # début d'une instance
                        blk=[lines[i]]; dd=1; i+=1
                        while i<len(lines) and dd>0:
                            tt=lines[i].strip(); dd+=tt.count('{')-tt.count('}'); blk.append(lines[i]); i+=1
                        if idx%N==0: out+=blk; kept+=1
                        else: removed+=1; changed=True
                        idx+=1; continue
                    d+=t.count('{')-t.count('}'); out.append(lines[i]); i+=1
                continue
            i+=1
        if changed: open(p,'wb').write('\n'.join(out).encode('utf-8'))
print('buissons gardes',kept,'retires',removed)
