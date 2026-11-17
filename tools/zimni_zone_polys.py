# Contours des zones lentes à partir du banc fin (grille 500 m) -> chemins SVG (carte 800 px = 16 384 m)
import sys, csv, json
src = sys.argv[1]
worst = {}
with open(src, encoding='utf-8') as f:
    rows = [l for l in f if not l.startswith('#')]
for r in csv.DictReader(rows, delimiter=';'):
    k = (int(float(r['x'])), int(float(r['z'])))
    v = float(r['fpsMoyen'])
    worst[k] = min(worst.get(k, 999), v)
S, H = 800 / 16384, 250  # échelle, demi-maille
def px(x, z): return (round(x * S, 1), round(800 - z * S, 1))
def loops(cells):
    edges = {}
    for (x, z) in cells:
        c = [(x-H, z-H), (x+H, z-H), (x+H, z+H), (x-H, z+H)]
        for i in range(4):
            a, b = c[i], c[(i+1) % 4]
            if (b, a) in edges: del edges[(b, a)]
            else: edges[(a, b)] = 1
    nxt = {}
    for a, b in edges: nxt.setdefault(a, []).append(b)
    out = []
    while nxt:
        a = next(iter(nxt)); loop = [a]
        while True:
            b = nxt[a].pop()
            if not nxt[a]: del nxt[a]
            if b == loop[0]: break
            loop.append(b); a = b
        out.append(loop)
    return out
def chaikin(p, n=3):
    for _ in range(n):
        q = []
        for i in range(len(p)):
            a, b = p[i], p[(i+1) % len(p)]
            q += [(0.75*a[0]+0.25*b[0], 0.75*a[1]+0.25*b[1]), (0.25*a[0]+0.75*b[0], 0.25*a[1]+0.75*b[1])]
        p = q
    return p
res = {}
for name, thr in (('red', 90), ('orange', 115)):
    cells = [k for k, v in worst.items() if v < thr]
    d = ''
    for lp in loops(cells):
        pts = [px(*q) for q in chaikin(lp)]
        d += 'M' + ' L'.join(f'{x:.1f} {y:.1f}' for x, y in pts) + 'Z '
    res[name] = d.strip()
res['pts'] = {f'{x},{z}': round(v) for (x, z), v in worst.items()}
print(json.dumps(res))
