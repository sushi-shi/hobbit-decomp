"""VC6 floating COMDATs require complete typed section/symbol evidence."""
import struct
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
from hobbit.retail_labels.source import obj_literals, compgen_claims


def pool_object(name, payload, *, section='.rdata', flags=0x40301040,
                typ=0, storage=2, defined=True, alias=False, selection=2):
    # Same minimal COFF fixture mechanism as compare/test_function_sizes.py.
    strings=bytearray(bytes(4));table=bytearray()
    def symbol(name,value,sn,typ,scl,aux=b''):
        table.extend(struct.pack('<II',0,len(strings)))
        strings.extend(name.encode()+b'\0')
        table.extend(struct.pack('<IhHBB',value,sn,typ,scl,len(aux)//18));table.extend(aux)
    aux=struct.pack('<IHHIHB3s',len(payload),0,0,0,0,selection,bytes(3))
    symbol(section,0,1,0,3,aux)
    symbol(name,0,1 if defined else 0,typ,storage)
    if alias:symbol('_ordinary_data',0,1,0,2)
    struct.pack_into('<I',strings,0,len(strings))
    header=struct.pack('<HHIIIHH',0x14c,1,0,60+len(payload),4 if alias else 3,0,0)
    sec=struct.pack('<8sIIIIIIHHI',section.encode(),0,0,len(payload),60,0,0,0,0,flags)
    return header+sec+payload+table+strings


class NamedFloatPools(unittest.TestCase):
    def read_pool(self, blob):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'pool.obj';p.write_bytes(blob)
            return obj_literals(p)[1]
    def test_complete_float_and_double_named_comdats(self):
        for name,raw in [('__real@447a0000',struct.pack('<f',1000.0)),
                         ('__real@3ff0000000000000',struct.pack('<d',1.0))]:
            self.assertEqual(self.read_pool(pool_object(name,raw)),{name:raw})
    def test_rejects_wrong_width_bits_type_storage_and_undefined(self):
        good=struct.pack('<f',1000.0)
        bad=[pool_object('__real@447a0000',good+bytes(4)),
             pool_object('__real@447a0000',bytes(4)),
             pool_object('__real@447a0000',good,typ=0x20),
             pool_object('__real@447a0000',good,storage=3),
             pool_object('__real@447a0000',good,defined=False)]
        for blob in bad:self.assertEqual(self.read_pool(blob),{})
    def test_rejects_nonpool_data_alias_and_noncomdat(self):
        raw=struct.pack('<f',1000.0)
        bad=[pool_object('__real@447a0000',raw,section='.data',flags=0xc0301040),
             pool_object('__real@447a0000',raw,alias=True),
             pool_object('__real@447a0000',raw,selection=0),
             pool_object('_ordinary_data',raw)]
        for blob in bad:self.assertEqual(self.read_pool(blob),{})
    def test_source_type_width_must_match_and_named_identity_is_preserved(self):
        raw=bytes(8)
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'pool.obj';p.write_bytes(pool_object('__real@0000000000000000',raw))
            with patch('hobbit.retail_labels.source.image') as img:
                img.return_value.read.side_effect=lambda a,n:bytes(n)
                good,problems=compgen_claims('DATA_COMPGEN(0x002e0000, 0.0)','fixture',p)
                self.assertEqual(problems,[]);self.assertEqual(good,[(0x2e0000,'__real@0000000000000000',8)])
                wrong,problems=compgen_claims('DATA_COMPGEN(0x002e0000, 0.0f)','fixture',p)
                self.assertEqual(wrong,[]);self.assertTrue(problems)
    def test_arbitrary_matching_words_and_noncanonical_names_are_not_pools(self):
        raw=struct.pack('<f',1000.0)
        self.assertEqual(self.read_pool(pool_object('_ordinary_data',raw)),{})
        self.assertEqual(self.read_pool(pool_object('__real@447A0000',raw)),{})
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'data.obj';p.write_bytes(pool_object('_ordinary_data',raw))
            with patch('hobbit.retail_labels.source.image'):
                claims,problems=compgen_claims('DATA_COMPGEN(0x002e0000, 1000.0f)','fixture',p)
                self.assertEqual(claims,[]);self.assertTrue(problems)
