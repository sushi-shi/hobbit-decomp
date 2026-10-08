"""Owned initializer naming must preserve bytes, addends, and referent identity."""
import struct
import unittest
from pathlib import Path
from types import SimpleNamespace as NS
from unittest.mock import patch
from hobbit.delink import dyninit


class Candidate:
    section_table = [dict(characteristics=0x20000000), dict(characteristics=0x20000000)]
    def iter_symbols(self):
        return [(0, 0, 1), (1, 0, 2)]
    def sym_name(self, index):
        return ['_$E1', '_$E2'][index]
    def section_members(self, section):
        return []
    def section_payload(self, section):
        return (bytes.fromhex('6a02b900000000e800000000c3909090') if section == 1
                else bytes.fromhex('e9000000009090909090909090909090'))
    def typed_relocations(self, section):
        return ({3: ('_owner', 6), 8: ('_ctor', 20)} if section == 1
                else {1: ('_$E1', 20)})


def setup():
    direct = b'\x6a\x02\xb9' + struct.pack('<I', 0x403000) + b'\xe8' + struct.pack('<i', 0x2000 - 0x100c) + b'\xc3'
    wrapper = b'\xe9' + struct.pack('<i', 0x1000 - 0x905)
    code = {0x1000: direct, 0x900: wrapper}
    image = NS(image_base=0x400000, absolute_sites=[0x1003],
               raw_read=lambda rva, size: code.get(rva),
               relocs_in=lambda lo, hi: [x for x in [0x1003] if lo <= x < hi])
    pins = [NS(rva=rva, size=len(body), channel='src_dyninit', name='', unit='test',
               aliases=[NS(name='owner', channel='src_dyninit')]) for rva, body in code.items()]
    model = NS(functions=pins, data=[NS(rva=0x3000, name='_owner', channel='src', unit='test')])
    return model, image, code


class InitializerControls(unittest.TestCase):
    def provision(self, model, image):
        with patch.object(dyninit, 'Obj', return_value=Candidate()), patch.object(Path, 'is_file', return_value=True):
            return dyninit.provision(model, {0x2000: ('_ctor', 'test', 1)}, Path('/fixture'), image)

    def test_owned_helper_and_wrapper_are_named_from_real_candidate_symbols(self):
        model, image, _ = setup()
        self.assertEqual(self.provision(model, image), {0x1000: ('_$E1', 'test', 13), 0x900: ('_$E2', 'test', 5)})

    def test_wrong_owner_callee_or_nonaddress_byte_cannot_provision(self):
        for offset in (1, 3, 8):
            model, image, code = setup()
            changed = bytearray(code[0x1000]); changed[offset] ^= 1
            code[0x1000] = bytes(changed)
            self.assertEqual(self.provision(model, image), {})

    def test_unadmitted_address_field_cannot_provision(self):
        model, image, _ = setup()
        image.absolute_sites = []
        self.assertEqual(self.provision(model, image), {})


if __name__ == '__main__':
    unittest.main()
