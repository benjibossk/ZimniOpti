"""Reduit un .edds Enfusion en retirant ses N plus grands niveaux de mip, sans recompresser.
Format: en-tete DDS (128 o, +20 si DX10) puis table [tag 'COPY'|'LZ4 ', u32 taille] par mip du plus PETIT au plus GRAND,
puis les blocs de donnees dans le meme ordre. On retire les N dernieres entrees + blocs et on corrige l'en-tete.
Usage: python -I edds_dropmip.py <entree.edds> <sortie.edds> [N=1]"""
import struct,sys
def dropmip(b,n=1):
    q0=148 if b[84:88]==b'DX10' else 128
    h,w=struct.unpack_from('<II',b,12); m=struct.unpack_from('<I',b,28)[0]
    t=[];q=q0
    while b[q:q+4] in (b'COPY',b'LZ4 '): t.append((b[q:q+4],struct.unpack_from('<I',b,q+4)[0])); q+=8
    assert len(t)==m and m>n, f'table {len(t)} != mips {m}'
    assert q+sum(s for _,s in t)==len(b), 'taille incoherente'
    keep=t[:-n]; data=b[q:q+sum(s for _,s in keep)]
    hd=bytearray(b[:q0]); nw,nh=max(w>>n,1),max(h>>n,1)
    struct.pack_into('<II',hd,12,nh,nw); struct.pack_into('<I',hd,28,m-n)
    pitch=struct.unpack_from('<I',b,20)[0]; struct.pack_into('<I',hd,20,max(pitch>>(2*n),1))
    return bytes(hd)+b''.join(tag+struct.pack('<I',s) for tag,s in keep)+data,(w,h,nw,nh)
if __name__=='__main__':
    n=int(sys.argv[3]) if len(sys.argv)>3 else 1
    out,info=dropmip(open(sys.argv[1],'rb').read(),n); open(sys.argv[2],'wb').write(out); print(info,len(out))
