# Heatmap PNG + liste des zones lentes à partir d'un bench grille.
# usage: python -I zimni_heatmap.py <bench.csv> <sortie.png> [seuil_fps=90]
import sys, zlib, struct, collections
src, png = sys.argv[1], sys.argv[2]
seuil = float(sys.argv[3]) if len(sys.argv) > 3 else 90
pts = collections.defaultdict(dict)
for l in open(src, encoding='utf-8'):
    if l.startswith('#') or l.startswith('label'): continue
    p = l.strip().split(';')
    pts[(float(p[1]), float(p[2]))][int(p[3])] = float(p[4])
xs = sorted({k[0] for k in pts}); zs = sorted({k[1] for k in pts})
W, H, S = len(xs), len(zs), 16
def col(f):  # rouge (bas) -> jaune -> vert (haut)
    t = max(0, min(1, (f - 30) / (144 - 30)))
    return (int(255 * min(1, 2 - 2 * t)), int(255 * min(1, 2 * t)), 0)
rows = []
for zi in range(H - 1, -1, -1):  # nord en haut
    line = []
    for xi in range(W):
        d = pts.get((xs[xi], zs[zi]))
        c = col(min(d.values())) if d else (40, 40, 40)
        line += list(c) * S
    for _ in range(S): rows.append(b'\0' + bytes(line))
def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
open(png, 'wb').write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', W * S, H * S, 8, 2, 0, 0, 0))
                       + chunk(b'IDAT', zlib.compress(b''.join(rows), 9)) + chunk(b'IEND', b''))
lents = sorted(((min(d.values()), k, d) for k, d in pts.items() if min(d.values()) < seuil))
print(f"{len(pts)} points, {len(lents)} sous {seuil} fps -> {png}")
noms = {0: 'N', 90: 'E', 180: 'S', 270: 'O'}
for f, k, d in lents[:40]:
    print(f"  {f:6.1f} fps  x={k[0]:.0f} z={k[1]:.0f}  " + ' '.join(f"{noms.get(a,a)}={v:.0f}" for a, v in sorted(d.items())))
