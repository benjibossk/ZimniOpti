"""Resume un ou compare deux bench_*.csv (par point : FPS moyen des 4 directions, pire direction, pire frame).
Usage: python -I zimni_bench_compare.py <avant.csv> [apres.csv]"""
import csv,sys,collections
def load(p):
    rows=[l for l in open(p,encoding='utf8') if not l.startswith('#')]
    d=collections.OrderedDict()
    for x in csv.DictReader(rows,delimiter=';'): d.setdefault(x['label'],[]).append(x)
    return {k:(sum(float(r['fpsMoyen']) for r in v)/len(v),min(float(r['fpsMoyen']) for r in v),max(float(r['pireFrameMs']) for r in v),int(v[-1]['memoireMo']),int(v[0]['entites150m'])) for k,v in d.items()}
a=load(sys.argv[1]); b=load(sys.argv[2]) if len(sys.argv)>2 else None
print(f"{'point':18} {'fps':>6} {'fpsMin':>6} {'pireMs':>7} {'memMo':>6} {'ent150':>6}" + ("   | apres: fps  fpsMin  pireMs  memMo   delta fps" if b else ''))
for k,v in a.items():
    s=f'{k:18} {v[0]:6.1f} {v[1]:6.1f} {v[2]:7.1f} {v[3]:6} {v[4]:6}'
    if b and k in b:
        w=b[k]; s+=f'   | {w[0]:9.1f} {w[1]:6.1f} {w[2]:7.1f} {w[3]:6}   {w[0]-v[0]:+6.1f} ({(w[0]/v[0]-1)*100:+.0f}%)'
    print(s)
