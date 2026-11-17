# Prototype « apparition à la demande » (F21).
# Usage : python -I zimni_stream_build.py <World_dir> <streamdump.bin> <streamdump_prefabs.txt> <dossier_tuiles> [--apply]
#  1. lit le relevé fait en jeu (ZO_Streamer.Dump : prefab + matrice monde de chaque objet ciblé) ;
#  2. parcourt les calques, calcule la position monde de chaque instance, la retrouve dans le relevé (même prefab, < 0,15 m) ;
#  3. écrit les tuiles de 1 km (t_<tx>_<tz>.json, binaire malgré le nom, + .meta pour l'empaquetage) avec SEULEMENT les objets retrouvés + prefabs.json ;
#  4. --apply : retire ces instances des calques (sinon simple simulation).
import sys, os, re, struct, math
from collections import defaultdict

W, DUMP, NAMES, OUT = sys.argv[1:5]
APPLY = '--apply' in sys.argv
# --thinned <World_eclairci> : les calques lus sont ceux d'ORIGINE ; un objet absent des calques éclaircis
# (buissons 1/3, rochers 1/2) est marqué « PC et serveur seulement » (index prefab + PCONLY) : jamais créé sur console.
PCONLY = 1048576
THIN = sys.argv[sys.argv.index('--thinned') + 1] if '--thinned' in sys.argv else None
thin_keep = {}
if THIN:
    from collections import Counter
    for dp, _, fs in os.walk(THIN):
        for fn in fs:
            if fn.endswith('.layer'):
                rel = os.path.relpath(os.path.join(dp, fn), THIN)
                thin_keep[rel] = Counter(l.strip() for l in open(os.path.join(dp, fn), encoding='utf-8') if l.strip().startswith('coords '))
pconly = set()
TILE, NT, TOL, B = 1000.0, 17, 0.15, 0.5

names = [l.rstrip('\r') for l in open(NAMES, encoding='utf-8', newline='').read().split('\n')]
if names and names[-1] == '':
    names.pop()  # ordre exact : l'index d'un prefab = son numéro de ligne
raw = open(DUMP, 'rb').read()
n = struct.unpack_from('<i', raw, 0)[0]
pref = struct.unpack_from('<%di' % n, raw, 4)
mat = struct.unpack_from('<%df' % (n * 12), raw, 4 + 4 * n)
print('releve :', n, 'objets,', len(names), 'prefabs')
guid_of = [(re.match(r'\{([0-9A-F]{16})\}', s).group(1) if re.match(r'\{([0-9A-F]{16})\}', s) else '?' + s) for s in names]
gset = set(guid_of)
# --keep <fichier GUID> : prefabs laissés dans la carte (cassables : murs, clôtures, panneaux — F36)
KEEP = set(l.strip() for l in open(sys.argv[sys.argv.index('--keep') + 1]) if l.strip()) if '--keep' in sys.argv else set()
idx = defaultdict(list)
for i in range(n):
    if guid_of[pref[i]] in KEEP:
        continue
    x, z = mat[i * 12 + 9], mat[i * 12 + 11]
    idx[(guid_of[pref[i]], round(x / B), round(z / B))].append(i)
used = bytearray(n)

def find(g, x, z):
    bx, bz = round(x / B), round(z / B)
    best, bd = -1, TOL * TOL
    for dx in (-1, 0, 1):
        for dz in (-1, 0, 1):
            for i in idx.get((g, bx + dx, bz + dz), ()):
                if used[i]:
                    continue
                d = (mat[i * 12 + 9] - x) ** 2 + (mat[i * 12 + 11] - z) ** 2
                if d < bd:
                    best, bd = i, d
    return best

re_grp = re.compile(r'^\$grp \w+(?: : "\{([0-9A-F]{16})\}[^"]*")? \{$')
re_pf = re.compile(r':\s*"\{([0-9A-F]{16})\}[^"]*"')
stats = defaultdict(int)

class F:
    __slots__ = ('kind', 'guid', 'pos', 'ang', 'scale', 'start', 'kids', 'world', 'parent', 'ninst', 'ndel', 'ctext')
    def __init__(s, kind, guid, start, parent):
        s.kind, s.guid, s.start, s.parent = kind, guid, start, parent
        s.pos, s.ang, s.scale, s.kids, s.world, s.ninst, s.ndel, s.ctext = (0.0, 0.0, 0.0), (0.0, 0.0, 0.0), 1.0, False, None, 0, 0, None

def world(f):
    # (x, z, yaw°, échelle) ; ne tient compte que du lacet des parents (tangage/roulis comptés à part)
    if f.world is None:
        p = f.parent
        if p is None:
            f.world = (f.pos[0], f.pos[2], f.ang[1], f.scale)
        else:
            px, pz, py, ps = world(p)
            if abs(p.ang[0]) > 0.01 or abs(p.ang[2]) > 0.01:
                stats['parent_incline'] += 1
            a = math.radians(py)
            lx, lz = f.pos[0] * ps, f.pos[2] * ps
            f.world = (px + lx * math.cos(a) + lz * math.sin(a), pz - lx * math.sin(a) + lz * math.cos(a), py + f.ang[1], ps * f.scale)
    return f.world

def alt(f):
    p = f.parent
    if p is None or abs(p.ang[1]) < 0.001:
        return None
    px, pz, py, ps = world(p)
    a = math.radians(py)
    lx, lz = f.pos[0] * ps, f.pos[2] * ps
    return (px + lx * math.cos(a) - lz * math.sin(a), pz + lx * math.sin(a) + lz * math.cos(a))

def vec(t):
    try:
        return tuple(float(v) for v in t.split()[1:4])
    except ValueError:
        return None

total_del = 0
for dp, _, fs in os.walk(W):
    for fn in fs:
        if not fn.endswith('.layer'):
            continue
        path = os.path.join(dp, fn)
        keep = thin_keep.get(os.path.relpath(path, W)) if THIN else None
        lines = open(path, 'rb').read().decode('utf-8').split('\n')
        dele = bytearray(len(lines))
        stack = [F('root', None, -1, None)]
        def owner():
            # entité propriétaire du bloc enfants en cours
            for fr in reversed(stack):
                if fr.kind in ('ent', 'inst'):
                    return fr
            return None
        for li, l in enumerate(lines):
            t = l.strip()
            top = stack[-1]
            net = t.count('{') - t.count('}')
            if t == '}' or (net < 0 and t.endswith('}')):
                fr = stack.pop()
                if fr.kind in ('inst', 'ent') and fr.guid in gset and not fr.kids:
                    stats['candidats'] += 1
                    wx, wz, _, _ = world(fr)
                    i = find(fr.guid, wx, wz)
                    if i < 0:
                        a2 = alt(fr)
                        if a2:
                            i = find(fr.guid, a2[0], a2[1])
                            if i >= 0:
                                stats['convention_B'] += 1
                    if i >= 0:
                        used[i] = 1
                        if THIN and fr.ctext:
                            if keep is not None and keep[fr.ctext] > 0:
                                keep[fr.ctext] -= 1
                            else:
                                pconly.add(i)
                        for k in range(fr.start, li + 1):
                            dele[k] = 1
                        if fr.kind == 'inst':
                            stack[-1].ndel += 1
                    else:
                        stats['candidats_non_retrouves'] += 1
                elif fr.kind == 'grp' and fr.ninst > 0 and fr.ndel == fr.ninst:
                    for k in range(fr.start, li + 1):
                        dele[k] = 1
                continue
            if net > 0 and t.endswith('{'):
                if top.kind in ('root', 'kids'):
                    m = re_grp.match(t)
                    if m:
                        stack.append(F('grp', m.group(1), li, None)); stats['grp'] += 1; stats['grp_cible'] += m.group(1) in gset
                    else:
                        m2 = re_pf.search(t)
                        stack.append(F('ent', m2.group(1) if m2 else None, li, owner()))
                elif top.kind == 'grp' and t == '{':
                    top.ninst += 1; stats['inst'] += 1
                    stack.append(F('inst', top.guid, li, owner()))
                elif top.kind in ('ent', 'inst') and t == '{':
                    top.kids = True
                    stack.append(F('kids', None, li, None))
                else:
                    stack.append(F('prop', None, li, None))
                continue
            if top.kind in ('ent', 'inst'):
                if t.startswith('coords '):
                    v = vec(t)
                    if v:
                        top.pos = v
                        top.ctext = t
                elif t.startswith('angles '):
                    v = vec(t)
                    if v:
                        top.ang = v
                elif t.startswith('scale '):
                    try:
                        top.scale = float(t.split()[1])
                    except ValueError:
                        pass
        nd = sum(dele)
        if nd:
            total_del += nd
            if APPLY:
                open(path, 'wb').write('\n'.join(l for k, l in enumerate(lines) if not dele[k]).encode('utf-8'))

matched = sum(used)
print('retrouves dans les calques :', matched, '/', n, '(', round(100.0 * matched / max(n, 1), 1), '% )')
print('PC/serveur seulement (eclaircis) :', len(pconly))
print('stats :', dict(stats), '; lignes retirees :', total_del, '(appliquees)' if APPLY else '(simulation)')

# tuiles : seulement les objets retrouvés (les autres restent dans les calques)
os.makedirs(OUT, exist_ok=True)
tiles = defaultdict(list)
for i in range(n):
    if used[i]:
        tx = min(NT - 1, max(0, int(mat[i * 12 + 9] // TILE)))
        tz = min(NT - 1, max(0, int(mat[i * 12 + 11] // TILE)))
        tiles[(tx, tz)].append(i)
for f in os.listdir(OUT):
    if f.startswith('t_') and (f.endswith('.bin') or f.endswith('.json') or f.endswith('.json.meta')):
        os.remove(os.path.join(OUT, f))
for (tx, tz), ids in tiles.items():
    with open(os.path.join(OUT, 't_%d_%d.json' % (tx, tz)), 'wb') as o:
        o.write(struct.pack('<i', len(ids)))
        o.write(struct.pack('<%di' % len(ids), *[pref[i] + (PCONLY if i in pconly else 0) for i in ids]))
        flat = []
        for i in ids:
            flat.extend(mat[i * 12:i * 12 + 12])
        o.write(struct.pack('<%df' % len(flat), *flat))
with open(os.path.join(OUT, 'prefabs.json'), 'w', encoding='utf-8', newline='\n') as o:
    o.write('\n'.join(names) + '\n')
print('tuiles :', len(tiles), 'dans', OUT)

# .meta (ressource JSON) : sans eux le Workbench n'empaquette pas les fichiers
import hashlib
META = """MetaFileClass {
 Name "{%s}Stream/%s"
 Configurations {
  JSONResourceClass PC {
  }
  JSONResourceClass XBOX_ONE : PC {
  }
  JSONResourceClass XBOX_SERIES : PC {
  }
  JSONResourceClass PS4 : PC {
  }
  JSONResourceClass PS5 : PC {
  }
  JSONResourceClass HEADLESS : PC {
  }
 }
}
"""
for f in os.listdir(OUT):
    if f.endswith('.json'):
        g = hashlib.sha256(('zimni_stream_' + f).encode()).hexdigest()[:16].upper()
        open(os.path.join(OUT, f + '.meta'), 'w', encoding='utf-8', newline=chr(10)).write(META % (g, f))
print('meta ecrits')
