# Génère une grille de points de bench (mode heatmap, comme SCR_FPSDiagnosticPlugin).
# usage: python -I zimni_grid_points.py <sortie.txt> [pas_m=1000] [taille_m=16384] [settleMs=1500] [sampleS=1.0] [pitch=-10]
import sys
out = sys.argv[1]
step = float(sys.argv[2]) if len(sys.argv) > 2 else 1000
size = float(sys.argv[3]) if len(sys.argv) > 3 else 16384
settle, sample, pitch = (sys.argv[4:7] + ['1500', '1.0', '-10'][len(sys.argv[4:7]):])
with open(out, 'w', encoding='utf-8') as f:
    f.write(f"#cfg {settle} {sample} {pitch}\n")
    n = 0
    x = step / 2
    while x < size:
        z = step / 2
        while z < size:
            f.write(f"G{int(x)}_{int(z)} {x:.0f} {z:.0f}\n"); n += 1
            z += step
        x += step
print(n, "points ->", out)
