# Génère les prefabs d'occluders en boîtes (tailles par paliers) : ZimniOpti/Prefabs/ZO/Occl/ZO_Occl_<x>_<y>_<z>.et (+ .meta)
import os, hashlib, sys
M = sys.argv[1]
D = os.path.join(M, 'Prefabs', 'ZO', 'Occl'); os.makedirs(D, exist_ok=True)
XZ = [4, 6, 8, 11, 15, 20, 28]; Y = [3, 5, 8, 12]
META = '''MetaFileClass {
 Name "{%s}Prefabs/ZO/Occl/%s"
 Configurations {
  EntityTemplateResourceClass PC {
  }
  EntityTemplateResourceClass XBOX_ONE : PC {
  }
  EntityTemplateResourceClass XBOX_SERIES : PC {
  }
  EntityTemplateResourceClass PS4 : PC {
  }
  EntityTemplateResourceClass PS5 : PC {
  }
  EntityTemplateResourceClass HEADLESS : PC {
  }
 }
}
'''
n = 0
for x in XZ:
    for z in XZ:
        if z > x: continue
        for y in Y:
            f = 'ZO_Occl_%d_%d_%d.et' % (x, y, z)
            g = hashlib.sha256(('zo_occl_' + f).encode()).hexdigest()
            open(os.path.join(D, f), 'w', encoding='utf-8', newline='\n').write(
                'WorldOccluderEntity {\n ID "%s"\n BoundingVolume Box\n Size %d %d %d\n}\n' % (g[16:32].upper(), x, y, z))
            open(os.path.join(D, f + '.meta'), 'w', encoding='utf-8', newline='\n').write(META % (g[:16].upper(), f))
            n += 1
print(n, 'prefabs dans', D)
