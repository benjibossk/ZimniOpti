# Usage: python -I edds_mip_png.py <in.edds> <taille_px> <out.png>  (EDDS COPY non compressé, BC7/BCn lu par Pillow)
import sys,struct,io
from PIL import Image
d=open(sys.argv[1],'rb').read(); want=int(sys.argv[2])
w=struct.unpack_from('<I',d,16)[0]; mips=struct.unpack_from('<I',d,28)[0]
off=148; tab=[]
for i in range(mips):
    tag,sz=struct.unpack_from('<4sI',d,off+i*8); tab.append((tag,sz))
pos=off+mips*8; sizes=[w>>(mips-1-i) for i in range(mips)]  # plus petit d'abord
for (tag,sz),s in zip(tab,sizes):
    if s==want:
        assert tag==b'COPY',tag
        blob=d[pos:pos+sz]; break
    pos+=sz
hdr=bytearray(d[:148]); struct.pack_into('<III',hdr,12,want,want,len(blob)); struct.pack_into('<I',hdr,28,1)
struct.pack_into('<I',hdr,8,0x1007|0x80000)
Image.open(io.BytesIO(bytes(hdr)+blob)).convert('RGB').save(sys.argv[3]); print('ok',want)
