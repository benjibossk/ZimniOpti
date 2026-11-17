# Plan B : convertit les tuiles binaires (Stream_data/t_X_Z.json + prefabs.json) en ressources que le jeu sait charger
# depuis un mod publié : Stream/t_X_Z.conf (+ .meta CONFResourceClass) et Scripts/.../ZO_StreamIndex.c (prefabs + tuiles).
# Usage : python -I zimni_stream_conf.py <Stream_data> <dossier_sortie_mod_Stream> <ZO_StreamIndex.c>
import sys, os, struct, hashlib
SRC, OUT, IDX = sys.argv[1:4]
NT = 17
os.makedirs(OUT, exist_ok=True)
names = open(os.path.join(SRC, 'prefabs.json'), encoding='utf-8').read().split('\n')
if names and names[-1] == '':
    names.pop()
META = 'MetaFileClass {\n Name "{%s}Stream/%s"\n Configurations {\n' + ''.join(
    '  CONFResourceClass %s {\n  }\n' % c for c in ['PC']) + ''.join(
    '  CONFResourceClass %s : PC {\n  }\n' % c for c in ['XBOX_ONE', 'XBOX_SERIES', 'PS4', 'PS5', 'HEADLESS']) + ' }\n}\n'
def num(v, d):
    s = ('%.' + str(d) + 'f') % v
    s = s.rstrip('0').rstrip('.')
    return '0' if s in ('-0', '') else s
tiles, total, size = [], 0, 0
for f in sorted(os.listdir(SRC)):
    if not (f.startswith('t_') and f.endswith('.json')):
        continue
    raw = open(os.path.join(SRC, f), 'rb').read()
    n = struct.unpack_from('<i', raw, 0)[0]
    if n <= 0:
        continue
    pref = struct.unpack_from('<%di' % n, raw, 4)
    mat = struct.unpack_from('<%df' % (n * 12), raw, 4 + 4 * n)
    tx, tz = (int(x) for x in f[2:-5].split('_'))
    cf = 't_%d_%d.conf' % (tx, tz)
    lines = ['ZO_StreamTile {', ' m_aP {']
    for i in range(0, n, 40):
        lines.append('  ' + ' '.join(str(p) for p in pref[i:i + 40]))
    lines.append(' }')
    lines.append(' m_aM {')
    for i in range(n):
        m = mat[i * 12:i * 12 + 12]
        # axes (avec échelle) : 4 décimales ; position : 3 décimales
        lines.append('  ' + ' '.join([num(v, 4) for v in m[:9]] + [num(v, 3) for v in m[9:]]))
    lines.append(' }')
    lines.append('}')
    txt = '\n'.join(lines) + '\n'
    open(os.path.join(OUT, cf), 'w', encoding='utf-8', newline='\n').write(txt)
    g = hashlib.sha256(('zimni_stream_conf_' + cf).encode()).hexdigest()[:16].upper()
    open(os.path.join(OUT, cf + '.meta'), 'w', encoding='utf-8', newline='\n').write(META % (g, cf))
    tiles.append((tx * NT + tz, '{%s}Stream/%s' % (g, cf)))
    total += n; size += len(txt)
BS = chr(92)
esc = lambda s: s.replace(BS, BS + BS).replace(chr(34), BS + chr(34))
c = ['// ZimniOpti — index des données de streaming (plan B). GÉNÉRÉ par zimni_stream_conf.py : ne pas modifier à la main.',
     '// %d prefabs, %d tuiles, %d objets.' % (len(names), len(tiles), total),
     'class ZO_StreamIndex', '{',
     '\tstatic void Fill(notnull array<string> names, notnull map<int, ResourceName> tiles)', '\t{']
for nm in names:
    c.append('\t\tnames.Insert("%s");' % esc(nm))
for k, rn in tiles:
    c.append('\t\ttiles.Set(%d, "%s");' % (k, rn))
c += ['\t}', '}', '']
open(IDX, 'w', encoding='utf-8', newline='\n').write('\n'.join(c))
print('tuiles conf :', len(tiles), '; objets :', total, '; taille texte : %.0f Mo' % (size / 1e6), '; prefabs :', len(names))
