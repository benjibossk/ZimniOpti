# Ajoute "AutoOccluders <v>" au GenericTerrainEntity du default.layer (garde les fins de ligne). usage: <layer> <v>
import sys
p, v = sys.argv[1], sys.argv[2]
b = open(p, 'rb').read()
k = b' LayerPreset "Terrain"'
e = b'\r\n' if b'\r\n' in b else b'\n'
assert b.count(k) == 1 and b'AutoOccluders' not in b
open(p, 'wb').write(b.replace(k, k + e + b' AutoOccluders ' + v.encode()))
