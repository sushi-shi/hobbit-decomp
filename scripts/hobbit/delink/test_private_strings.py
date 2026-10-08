"""Physical duplicate literals stay separate; nonliteral addends remain strict."""
import struct
import unittest
from types import SimpleNamespace as NS
from unittest.mock import patch
from hobbit.delink.private_strings import rows
from hobbit.delink.literal_refs import referenced_addresses, _exact_instruction_alignment_tail


class Candidate:
    section_table=[dict(index=1,name='.text',size=22,characteristics=0x20000020),
                   dict(index=2,name='.data',size=8,characteristics=0x40)]
    def section_members(self,section):return [(0,'$SG1',3),(4,'$SG2',3)] if section==2 else []
    def section_payload(self,section):return (b'\x68\0\0\0\0\x68\0\0\0\0\xc3'*2 if section==1 else b'foo\0foo\0')
    def typed_relocations(self,section):return {1:('$SG1',6),6:('_anchor',6),12:('$SG2',6),17:('_anchor',6)}
    def defined_symbols(self,section):return [(0,'_UseA'),(11,'_UseB')]


class PrivateStringControls(unittest.TestCase):
    def run_case(self, *, bad_anchor=False, missing_site=False, mismatch=False):
        code=lambda target,anchor:b'\x68'+struct.pack('<I',0x400000+target)+b'\x68'+struct.pack('<I',0x400000+anchor)+b'\xc3'
        blobs={0x1000:code(0x3000,0x4001 if bad_anchor else 0x4000),0x1100:code(0x3004,0x4000),
               0x3000:b'bar\0' if mismatch else b'foo\0',0x3004:b'foo\0'}
        sites=[0x1001,0x1006,0x1101,0x1106]
        if missing_site:sites.remove(0x1001)
        image=NS(image_base=0x400000,pe=NS(read=lambda rva,size:blobs.get(rva)),
                 relocs_in=lambda lo,hi:[s for s in sites if lo<=s<hi],
                 classify_storage=lambda rva:'data-initialized')
        model=NS(functions=[NS(rva=rva,size=11,name=name,unit='fixture',channel='src') for rva,name in [(0x1000,'_UseA'),(0x1100,'_UseB')]],
                 data=[NS(rva=0x4000,name='_anchor',unit='fixture',channel='src')])
        with patch('hobbit.delink.private_strings.coffx.objects',return_value=[('fixture',Candidate())]):
            return rows(model,image,None)

    def test_identical_payloads_keep_distinct_physical_addresses(self):
        result,withheld=self.run_case()
        self.assertEqual([(r['member'],r['name'],r['rva']) for r in result],
                         [('$SG1','$SG12288',0x3000),('$SG2','$SG12292',0x3004)])
        self.assertEqual(withheld,[])

    def test_nonliteral_addend_unreviewed_field_and_wrong_bytes_are_refused(self):
        for kwargs in (dict(bad_anchor=True),dict(missing_site=True),dict(mismatch=True)):
            with self.subTest(kwargs=kwargs):
                result,withheld=self.run_case(**kwargs)
                self.assertEqual([r['member'] for r in result],['$SG2'])
                self.assertTrue(withheld)

    def test_known_relative_callee_identity_must_match(self):
        class Calls(Candidate):
            section_table=[dict(index=1,name='.text',size=11,characteristics=0x20000020)]
            def section_payload(self,section):return b'\x68\0\0\0\0\xe8\0\0\0\0\xc3'
            def typed_relocations(self,section):return {1:('$SG1',6),6:('_Callee',20)}
            def defined_symbols(self,section):return [(0,'_UseA')]
        model=NS(functions=[NS(rva=rva,size=11,name=name,unit='fixture',channel='src')
                            for rva,name in [(0x1000,'_UseA'),(0x2000,'_Callee')]],data=[])
        for target in (0x2000,0x2001):
            code=b'\x68'+struct.pack('<I',0x403000)+b'\xe8'+struct.pack('<i',target-0x100a)+b'\xc3'
            image=NS(image_base=0x400000,pe=NS(read=lambda rva,size:code),
                     relocs_in=lambda lo,hi:[0x1001])
            result=referenced_addresses(model,image,'fixture',Calls(),{'$SG1'})
            self.assertEqual(result['$SG1'],{0x3000} if target==0x2000 else set())

    def test_initialized_source_datum_supplies_only_reviewed_literal_reference(self):
        class Initializer:
            section_table=[dict(index=1,name='.data',size=4,characteristics=0x40)]
            def section_payload(self,section):return bytes(4)
            def typed_relocations(self,section):return {0:('$SG1',6)}
            def defined_symbols(self,section):return []
            def section_members(self,section):return [(0,'_Initializer',3)]
        model=NS(functions=[],data=[NS(rva=0x2000,size=4,name='_Initializer',unit='fixture',channel='src')])
        for reviewed in (True,False):
            image=NS(image_base=0x400000,pe=NS(read=lambda rva,size:struct.pack('<I',0x403000)),
                     relocs_in=lambda lo,hi:[0x2000] if reviewed else [])
            result=referenced_addresses(model,image,'fixture',Initializer(),{'$SG1'})
            self.assertEqual(result['$SG1'],{0x3000} if reviewed else set())


class LeaAlignmentControls(unittest.TestCase):
    def check(self, *, tail=b'\x8d\x49\0', retail_tail=None, relocations=None,
              retail_sites=(), members=(12,), rva=0x1000, destination=0):
        payload = b'\x68' + bytes(4) + b'\x90\x90\x90\xc3' + tail + bytes(4)
        obj = NS(section_table=[{'index': 1}, {'index': 2}],
                 section_payload=lambda sec: payload if sec == 1 else struct.pack('<i', destination),
                 typed_relocations=lambda sec: (relocations or {}).get(sec, {}),
                 iter_symbols=lambda: iter([(0, 0, 1)]),
                 sym_name=lambda index: '_Owner')
        image = NS(pe=NS(read=lambda address, size: tail if retail_tail is None else retail_tail),
                   relocs_in=lambda lo, hi: [s for s in retail_sites if lo <= s < hi])
        return _exact_instruction_alignment_tail(obj, 1, payload, 0, 9, 12, rva,
                                         set(members), image)

    def test_exact_aligned_same_register_lea(self):
        self.assertTrue(self.check())

    def test_different_instruction_or_retail_bytes_rejected(self):
        for tail in (b'\x8d\x49\x01', b'\x8d\x48\0', b'\x8d\x64\0',
                     b'\x5e\x90\x90'):
            with self.subTest(tail=tail):
                self.assertFalse(self.check(tail=tail))
        self.assertFalse(self.check(retail_tail=b'\x90' * 3))

    def test_next_symbol_and_both_alignments_required(self):
        self.assertFalse(self.check(members=()))
        self.assertFalse(self.check(rva=0x1001))

    def test_relocation_operands_cannot_overlap_alignment(self):
        for site in (6, 9, 11):
            with self.subTest(site=site):
                self.assertFalse(self.check(relocations={1: {site: ('_Other', 6)}}))
                self.assertFalse(self.check(retail_sites=(0x1000 + site,)))

    def test_other_section_cannot_reference_alignment(self):
        for kind in (6, 7, 11, 20):
            for destination in (9, 10, 11):
                with self.subTest(kind=kind, destination=destination):
                    self.assertFalse(self.check(relocations={2: {0: ('_Owner', kind)}},
                                                destination=destination))
            self.assertTrue(self.check(relocations={2: {0: ('_Owner', kind)}}, destination=12))


class MovAlignmentControls(unittest.TestCase):
    def check(self, *, tail=b'\x8b\xff', retail_tail=None, relocations=None,
              retail_sites=(), members=(12,), rva=0x1000, destination=0):
        payload = b'\x68' + bytes(4) + b'\x90' * 4 + b'\xc3' + tail + bytes(4)
        obj = NS(section_table=[{'index': 1}, {'index': 2}],
                 section_payload=lambda sec: payload if sec == 1 else struct.pack('<i', destination),
                 typed_relocations=lambda sec: (relocations or {}).get(sec, {}),
                 iter_symbols=lambda: iter([(0, 0, 1)]),
                 sym_name=lambda index: '_Owner')
        image = NS(pe=NS(read=lambda address, size: tail if retail_tail is None else retail_tail),
                   relocs_in=lambda lo, hi: [s for s in retail_sites if lo <= s < hi])
        return _exact_instruction_alignment_tail(
            obj, 1, payload, 0, 10, 12, rva, set(members), image)

    def test_exact_aligned_same_register_mov(self):
        self.assertTrue(self.check())

    def test_wrong_instruction_rejected(self):
        for tail in (b'\x8b\xfe', b'\x8b\x3f', b'\x89\xff', b'\x90\x90', b'\x8b'):
            with self.subTest(tail=tail):
                self.assertFalse(self.check(tail=tail))

    def test_retail_bytes_and_boundaries_required(self):
        self.assertFalse(self.check(retail_tail=b'\x90\x90'))
        self.assertFalse(self.check(members=()))
        self.assertFalse(self.check(rva=0x1001))

    def test_overlap_and_addressed_gap_rejected(self):
        for site in (7, 10, 11):
            with self.subTest(site=site):
                self.assertFalse(self.check(relocations={1: {site: ('_Other', 6)}}))
                self.assertFalse(self.check(retail_sites=(0x1000 + site,)))
        for kind in (6, 7, 11, 20):
            for destination in (10, 11):
                with self.subTest(kind=kind, destination=destination):
                    self.assertFalse(self.check(relocations={2: {0: ('_Owner', kind)}},
                                                destination=destination))
            self.assertTrue(self.check(relocations={2: {0: ('_Owner', kind)}}, destination=12))


if __name__=='__main__':unittest.main()
