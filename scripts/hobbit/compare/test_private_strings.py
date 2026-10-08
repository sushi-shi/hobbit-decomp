"""Internal compiler-string namespace controls; synthetic COFF, no game inputs.
Fixture packing follows hobbit.compare.test_canonicalize.obj.
"""
import struct, tempfile, unittest
from pathlib import Path
from unittest.mock import patch
from types import SimpleNamespace
from hobbit import model
from hobbit.compare import canonicalize


def fixture(payload=b'first\0', name='$SG7', storage=3, section=1, flags=0xc0400040, duplicate=False):
    rawptr=60; symptr=rawptr+len(payload); strings=bytearray(bytes(4)); symbols=bytearray()
    for _ in range(2 if duplicate else 1):
        symbols+=struct.pack('<II',0,len(strings))+struct.pack('<IhHBB',0,section,0,storage,0)
        strings+=name.encode()+b'\0'
    struct.pack_into('<I',strings,0,len(strings))
    return struct.pack('<HHIIIHH',0x14c,1,0,symptr,2 if duplicate else 1,0,0)+struct.pack('<8sIIIIIIHHI',b'.data',0,0,len(payload),rawptr,0,0,0,0,flags)+payload+symbols+strings


class PrivateStringNamespaceControls(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.root=Path(self.temp.name);self.base=self.root/'objdiff/base';self.base.mkdir(parents=True)
        self.addCleanup(patch.stopall);patch.object(model,'BUILD',self.root).start()
        self.payloads={0x100:b'first\0',0x200:b'second\0'}
        patch('hobbit.evidence.pc_structure.checked_pe',return_value=SimpleNamespace(read=lambda a,n:self.payloads.get(a,b'')[:n])).start()
    def binding(self, unit='one',rva=0x100):
        return model.Binding(rva,len(self.payloads[rva]),'string','data','$SG7',unit,'data_compgen',())
    def write(self, unit='one',**kwargs):
        (self.base/(unit+'.obj')).write_bytes(fixture(**kwargs))
    def test_two_owners_normalize_by_distinct_content(self):
        self.write();self.write('two',payload=b'second\0');bs=[self.binding(),self.binding('two',0x200)];v=[];out=model._disambiguate(bs,v);self.assertEqual(v,[]);self.assertNotEqual(out[0].name,out[1].name)
        normalized=[]
        for b,named in zip(bs,out):
            self.assertTrue(model._local_compiler_string_defined(b));raw=(self.base/(b.unit+'.obj')).read_bytes();obj=canonicalize.CoffObject(raw);idx=next(iter(obj.symbols));renamed=canonicalize._rewrite_names(obj,{idx:named.name});native=canonicalize.canonicalize_coff(raw).data;self.assertEqual(native,canonicalize.canonicalize_coff(renamed).data);normalized.append(native)
        self.assertNotEqual(normalized[0],normalized[1])
    def test_definition_and_section_negative_controls(self):
        for kwargs in [{'storage':2},{'storage':2,'section':0},{'payload':b'wrong\0'},{'duplicate':True},{'flags':0xc0400000},{'flags':0xc04000c0},{'flags':0xc0400060},{'flags':0xe0400040}]:
            with self.subTest(kwargs=kwargs):self.write(**kwargs);self.assertFalse(model._local_compiler_string_defined(self.binding()))
        self.write();self.assertFalse(model._local_compiler_string_defined(self.binding()._replace(size=100)));self.assertFalse(model._local_compiler_string_defined(self.binding()._replace(kind='common')))
    def test_same_owner_and_external_global_collisions_fail(self):
        self.write();bs=[self.binding(),self.binding(rva=0x200)];v=[];model._disambiguate(bs,v);self.assertTrue(v)
        self.write(storage=2);self.write('two',payload=b'second\0',storage=2);v=[];model._disambiguate([self.binding(),self.binding('two',0x200)],v);self.assertTrue(v)
    def test_undefined_cross_tu_discriminator_stays_distinct(self):
        name='$SG7$HOBBIT_LOCAL_00000200';raw=fixture(name=name,storage=2,section=0);out=canonicalize.CoffObject(canonicalize.canonicalize_coff(raw).data);self.assertIn(name,[s.name for s in out.symbols.values()])

if __name__=='__main__':unittest.main(verbosity=2)
