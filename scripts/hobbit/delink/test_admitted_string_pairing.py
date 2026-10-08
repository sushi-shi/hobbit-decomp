import importlib.util,struct,unittest
from pathlib import Path
from types import SimpleNamespace as NS
from unittest.mock import patch
from hobbit.delink import private_strings
from hobbit.delink import literal_refs as m
class Pairing(unittest.TestCase):
 def run_case(self,*,admitted=True,size=3,payload=b'rb\0',opcode=0x68,callee=0x2000,kind=6,site=1,storage='data-initialized',unknown=False):
  class Candidate:
   section_table=[dict(index=1,name='.text',size=12,characteristics=0x20000020),dict(index=2,name='.data',size=3,characteristics=0x40)]
   def section_payload(self,s):return b'\x68'+bytes(4)+b'\xe8'+bytes(4)+b'\x40\xc3' if s==1 else b'rb\0'
   def typed_relocations(self,s):return {site:('$SG1',kind),6:('_Callee',20)} if s==1 else {}
   def section_members(self,s):return [(0,'$SG1',3)]if s==2 else[(0,'_Use',2)]
   def defined_symbols(self,s):return [(0,'_Use')]if s==1 else []
  code=bytes([opcode])+struct.pack('<I',0x403000)+b'\xe8'+struct.pack('<i',callee-0x100a)+b'\x41\xc3'
  image=NS(image_base=0x400000,pe=NS(read=lambda rva,n:code if rva==0x1000 else payload),relocs_in=lambda lo,hi:[0x1001]if lo<=0x1001<hi else [],classify_storage=lambda r:storage)
  model=NS(functions=[NS(unit='fixture',name='_Use',rva=0x1000,size=12,channel='src')]+([]if unknown else[NS(unit='fixture',name='_Callee',rva=0x2000,size=1,channel='src')]),data=[])
  with patch.object(m,'_manual_string_extents',return_value={0x3000:size}if admitted else{}),patch.object(private_strings,'referenced_addresses',lambda *args,**kwargs:m.referenced_addresses(*args,admitted_strings=True)),patch.object(private_strings.coffx,'objects',return_value=[('fixture',Candidate())]):return private_strings.rows(model,image,None)[0]
 def test_reviewed_operand_with_other_real_instruction_difference(self):self.assertEqual(self.run_case()[0]['rva'],0x3000)
 def test_negative_controls(self):
  for kw in [dict(admitted=False),dict(size=4),dict(payload=b'rc\0'),dict(payload=b'rbX'),dict(opcode=0xb8),dict(callee=0x2001),dict(kind=20),dict(site=2),dict(storage='rdata'),dict(unknown=True)]:
   with self.subTest(kw=kw):self.assertEqual(self.run_case(**kw),[])
 def test_only_reviewed_subset_returned(self):
  candidate=bytearray(b'\x68'+bytes(4)+b'\x68'+bytes(4)+b'\x40\xc3')
  retail=bytearray(candidate);retail[-2]=0x41
  image=NS(pe=NS(read=lambda r,n:b'rb\0'))
  with patch.object(m,'_manual_string_extents',return_value={0x3000:3}):
   self.assertEqual(m._admitted_string_pairing(candidate,retail,{1:('$SG1',6),6:('$SG2',6)},[('$SG1',0x3000),('$SG2',0x3100)],{'$SG1':('data',b'rb\0'),'$SG2':('data',b'rb\0')},NS(data=[]),image),[('$SG1',0x3000)])
if __name__=='__main__':unittest.main()
