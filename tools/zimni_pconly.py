# Buissons/rochers « PC et serveur seulement » (F24).
# check : <World_actuel> <streamdump.bin> <streamdump_prefabs.txt>
#   -> retrouve les objets de premier niveau des calques dans le relevé fait en jeu et cherche la convention
#      angles -> matrice qui reproduit exactement la matrice relevée (et si la hauteur des calques est absolue).
# diff  : <World_origine> <World_actuel> <sortie.bin>
#   -> objets présents dans les calques d'origine mais retirés par l'éclaircissement (buissons 1/3, rochers 1/2).
import sys, os, re, struct, math, itertools
from collections import defaultdict, Counter

re_grp = re.compile(r'^\$grp \w+(?: : "\{([0-9A-F]{16})\}[^"]*")? \{$')
re_pf = re.compile(r':\s*"\{([0-9A-F]{16})\}([^"]*)"')

def vec(t):
    try:
        return tuple(float(v) for v in t.split()[1:4])
    except ValueError:
        return None

def parse(W):
    """objets feuilles de premier niveau : (guid, chemin prefab, pos, angles, scale) ; + nb objets avec parent ignorés"""
    out, nested = [], 0
    for dp, _, fs in os.walk(W):
        for fn in fs:
            if not fn.endswith('.layer'):
                continue
            lines = open(os.path.join(dp, fn), 'rb').read().decode('utf-8').split('\n')
            stack = [['root', None, None, None, None, 1.0, False, 0]]  # kind guid path pos ang scale kids depth_ent
            for l in lines:
                t = l.strip()
                net = t.count('{') - t.count('}')
                top = stack[-1]
                if t == '}' or (net < 0 and t.endswith('}')):
                    fr = stack.pop()
                    if fr[0] in ('ent', 'inst') and fr[1] and not fr[6] and fr[3] is not None:
                        if fr[7] == 1:
                            out.append((fr[1], fr[2], fr[3], fr[4] or (0.0, 0.0, 0.0), fr[5]))
                        else:
                            nested += 1
                    continue
                if net > 0 and t.endswith('{'):
                    depth = sum(1 for f in stack if f[0] in ('ent', 'inst'))
                    if top[0] in ('root', 'kids'):
                        m = re_grp.match(t)
                        if m:
                            m2 = re_pf.search(t)
                            stack.append(['grp', m.group(1), m2.group(2) if m2 else None, None, None, 1.0, False, depth])
                        else:
                            m2 = re_pf.search(t)
                            stack.append(['ent', m2.group(1) if m2 else None, m2.group(2) if m2 else None, None, None, 1.0, False, depth + 1])
                    elif top[0] == 'grp' and t == '{':
                        stack.append(['inst', top[1], top[2], None, None, 1.0, False, depth + 1])
                    elif top[0] in ('ent', 'inst') and t == '{':
                        top[6] = True
                        stack.append(['kids', None, None, None, None, 1.0, False, 0])
                    else:
                        stack.append(['prop', None, None, None, None, 1.0, False, 0])
                    continue
                if top[0] in ('ent', 'inst'):
                    if t.startswith('coords '):
                        top[3] = vec(t)
                    elif t.startswith('angles '):
                        top[4] = vec(t)
                    elif t.startswith('scale '):
                        try:
                            top[5] = float(t.split()[1])
                        except ValueError:
                            pass
    return out, nested

def rx(a):
    c, s = math.cos(a), math.sin(a); return [[1, 0, 0], [0, c, -s], [0, s, c]]
def ry(a):
    c, s = math.cos(a), math.sin(a); return [[c, 0, s], [0, 1, 0], [-s, 0, c]]
def rz(a):
    c, s = math.cos(a), math.sin(a); return [[c, -s, 0], [s, c, 0], [0, 0, 1]]
def mul(A, B):
    return [[sum(A[i][k] * B[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
ROT = {'x': rx, 'y': ry, 'z': rz}

def matrix(ang, scale, order, signs, transpose):
    """angles (x, y, z) en degrés -> 3 axes (lignes) mis à l'échelle"""
    a = {'x': math.radians(ang[0]) * signs[0], 'y': math.radians(ang[1]) * signs[1], 'z': math.radians(ang[2]) * signs[2]}
    R = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
    for ax in order:
        R = mul(R, ROT[ax](a[ax]))
    if transpose:
        R = [list(r) for r in zip(*R)]
    return [[v * scale for v in row] for row in R]

def load_dump(dump, names_path):
    names = [l.rstrip('\r') for l in open(names_path, encoding='utf-8', newline='').read().split('\n')]
    if names and names[-1] == '':
        names.pop()
    raw = open(dump, 'rb').read()
    n = struct.unpack_from('<i', raw, 0)[0]
    pref = struct.unpack_from('<%di' % n, raw, 4)
    mat = struct.unpack_from('<%df' % (n * 12), raw, 4 + 4 * n)
    return names, n, pref, mat

def check(W, dump, names_path):
    names, n, pref, mat = load_dump(dump, names_path)
    g_of = [(re.match(r'\{([0-9A-F]{16})\}', s).group(1) if re.match(r'\{([0-9A-F]{16})\}', s) else None) for s in names]
    idx = defaultdict(list)
    for i in range(n):
        idx[(g_of[pref[i]], round(mat[i * 12 + 9] / 0.5), round(mat[i * 12 + 11] / 0.5))].append(i)
    ents, nested = parse(W)
    print('calques : %d objets de premier niveau, %d avec parent (ignorés)' % (len(ents), nested))
    pairs = []
    for g, p, pos, ang, sc in ents:
        bx, bz = round(pos[0] / 0.5), round(pos[2] / 0.5)
        best = None
        for dx in (-1, 0, 1):
            for dz in (-1, 0, 1):
                for i in idx.get((g, bx + dx, bz + dz), ()):
                    d = (mat[i * 12 + 9] - pos[0]) ** 2 + (mat[i * 12 + 11] - pos[2]) ** 2
                    if d < 0.0225 and (best is None or d < best[0]):
                        best = (d, i)
        if best:
            pairs.append((pos, ang, sc, best[1]))
    print('retrouvés dans le relevé :', len(pairs))
    # échantillon avec rotations non triviales pour départager les conventions
    tilted = [q for q in pairs if abs(q[1][0]) > 1 or abs(q[1][2]) > 1]
    sample = (tilted[:3000] + pairs[:2000])
    print('dont inclinés (tangage/roulis) :', len(tilted))
    res = []
    for order in itertools.permutations('xyz'):
        for signs in itertools.product((1, -1), repeat=3):
            for tr in (False, True):
                err = 0.0
                for pos, ang, sc, i in sample:
                    M = matrix(ang, sc, order, signs, tr)
                    m = mat[i * 12:i * 12 + 9]
                    err = max(err, max(abs(M[r][c] - m[r * 3 + c]) for r in range(3) for c in range(3)))
                res.append((err, ''.join(order), signs, tr))
    res.sort()
    for r in res[:4]:
        print('erreur max %.5f  ordre %s  signes %s  transposée %s' % r)
    dy = Counter(round(mat[i * 12 + 10] - pos[1], 1) for pos, ang, sc, i in pairs[:20000])
    print('hauteur relevée - hauteur calque (10 plus fréquents) :', dy.most_common(10))

def diff(Wo, Wc, out):
    o, no = parse(Wo)
    c, nc = parse(Wc)
    key = lambda e: (e[0], round(e[2][0], 2), round(e[2][2], 2))
    have = Counter(key(e) for e in c)
    rem = []
    for e in o:
        k = key(e)
        if have[k] > 0:
            have[k] -= 1
        else:
            rem.append(e)
    print('origine %d, actuel %d, retirés %d (avec parent ignorés : origine %d, actuel %d)' % (len(o), len(c), len(rem), no, nc))
    print('prefabs les plus retirés :', Counter(os.path.basename(e[1] or '?') for e in rem).most_common(8))
    import pickle
    pickle.dump(rem, open(out, 'wb'))
    print('écrit', out)

if __name__ == '__main__':
    if sys.argv[1] == 'check':
        check(*sys.argv[2:5])
    elif sys.argv[1] == 'diff':
        diff(*sys.argv[2:5])
