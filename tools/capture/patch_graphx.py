import pathlib,re,struct,sys,os,shutil
root=pathlib.Path(__file__).resolve().parents[2]
elf=pathlib.Path(sys.argv[1]);dest=pathlib.Path(sys.argv[2])
maptext=elf.with_suffix('.map').read_text()
symbols={name:int(addr,16) for addr,name in re.findall(r'^\s*(0x[0-9a-f]+)\s+([A-Za-z_][\w.]*)\s*$',maptext,re.M)}
data=(root/'dist/graphx.8xv').read_bytes()[74:-2]
codeoffset=int.from_bytes(data[-6:-3],'little');size=int.from_bytes(data[-3:],'little');base=0xd28000
code=bytearray(data[codeoffset:codeoffset+size]);relocations=data[codeoffset+size:-6]
for i in range(0,len(relocations),2):
 offset=int.from_bytes(relocations[i:i+2],'little')
 value=int.from_bytes(code[offset:offset+3],'little')+base
 code[offset:offset+3]=value.to_bytes(3,'little')
exports=re.findall(r'^\s*export\s+(gfx_\w+)',(pathlib.Path(os.environ.get('CEDEV') or pathlib.Path(shutil.which('cedev-config')).resolve().parent.parent)/'lib/libload/graphx.lib').read_text(),re.M)
patches=[(base,bytes(code))]
patches.append((0x15c,b'\xfd\xe9')) # standard boot indirect-call trampoline: jp (iy)
assert code[0]==0xcd
patches.append((int.from_bytes(code[1:4],'little'),b'\xc9')) # boot ClearVRAM stub; all VRAM starts zero
for index,name in enumerate(exports):
 symbol='_'+name
 if symbol in symbols:
  offset=int.from_bytes(data[3+index*2:5+index*2],'little')
  patches.append((symbols[symbol],b'\xc3'+(base+offset).to_bytes(3,'little')))
for i in range(3):patches.append((0x100000+i*0x10000,(root/f'assets/DBAS{i}.bin').read_bytes()))
with dest.open('wb') as f:
 f.write(struct.pack('<I',len(patches)))
 for addr,blob in patches:f.write(struct.pack('<II',addr,len(blob)));f.write(blob)
print('Entry:',hex(symbols['_main']),'patches:',len(patches),'graphx:',size,'bytes')
