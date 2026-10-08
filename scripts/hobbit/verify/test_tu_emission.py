from types import SimpleNamespace as NS
import unittest
import struct
from hobbit.verify.tu_emission import order_unit
from hobbit.verify.tu_order import Entry, check_intra, check_inter
from hobbit.retail_labels import Claim


def fixture():
    entries = [Entry(0x2000, 4, 1, 'B', 'src/sample.cpp'),
               Entry(0x1000, 4, 2, 'A', 'src/sample.cpp')]
    claims = [Claim(e.rva, name, 'func', 'src', e.size, 'sample', {})
              for e,name in zip(entries, ('_B', '_A'))]
    data=bytearray(256)
    struct.pack_into('<I',data,236,4);struct.pack_into('<I',data,244,4)
    coff = NS(data=bytes(data), symbol_offset=256,
              sections=[NS(name='.text', characteristics=0x60001020, raw_size=16, raw_offset=180, header_offset=20),
                        NS(name='.text', characteristics=0x60001020, raw_size=16, raw_offset=196, header_offset=60),
                        NS(name='.debug$F', characteristics=0, raw_size=8, raw_offset=232, header_offset=100),
                        NS(name='.debug$F', characteristics=0, raw_size=8, raw_offset=240, header_offset=140)],
              symbols={0:NS(name='_B', section=2, value=0, typ=0x20, storage_class=2),
                       1:NS(name='_A', section=1, value=0, typ=0x20, storage_class=2)},
              relocations=[NS(section=3,site=0,symbol_index=0,typ=7),NS(section=4,site=0,symbol_index=1,typ=7)])
    return entries, claims, coff


class EmissionOrderTests(unittest.TestCase):
    def test_deferred_emission_refutes_source_order_equality(self):
        entries,claims,coff=fixture()
        self.assertTrue(check_intra({'sample':entries}))
        ordered,_=order_unit('sample',entries,claims,coff)
        self.assertEqual([e.rva for e in ordered],[0x1000,0x2000])
        self.assertFalse(check_intra({'sample':ordered}))
        self.assertEqual({id(e) for e in ordered},{id(e) for e in entries})

    def test_real_emission_inversion_is_preserved(self):
        entries,claims,coff=fixture();entries.reverse()
        coff.symbols[0].section=1;coff.symbols[1].section=2
        self.assertFalse(check_intra({'sample':entries}))
        ordered,_=order_unit('sample',entries,claims,coff)
        self.assertTrue(check_intra({'sample':ordered}))

    def test_within_section_order_uses_symbol_offsets(self):
        entries,claims,coff=fixture();coff.symbols[0].section=1;coff.symbols[0].value=8
        ordered,_=order_unit('sample',entries,claims,coff)
        self.assertEqual([e.rva for e in ordered],[0x1000,0x2000])

    def test_missing_emission_or_stale_claim_is_not_exempted(self):
        for cause in ('missing','claim-size','claim-name','claim-channel','code-flags','body-size'):
            entries,claims,coff=fixture()
            if cause=='missing':del coff.symbols[0]
            elif cause=='claim-size':claims[0]=claims[0]._replace(size=3)
            elif cause=='claim-name':claims[0]=claims[0]._replace(name='_unrelated')
            elif cause=='claim-channel':claims[0]=claims[0]._replace(channel='src_dyninit')
            elif cause=='code-flags':coff.sections[1].characteristics=0
            else:coff.sections[1].raw_size=3
            with self.subTest(cause=cause),self.assertRaises(ValueError):order_unit('sample',entries,claims,coff)

    def test_target_extent_can_exceed_valid_current_emission(self):
        entries,claims,coff=fixture();entries[0].size=259;claims[0]=claims[0]._replace(size=259)
        ordered,_=order_unit('sample',entries,claims,coff)
        self.assertEqual(entries[0].size,259)
        self.assertEqual(ordered[-1].size,259)
        self.assertFalse(check_intra({'sample':ordered}))
        ordered[0].size=0x1100
        self.assertTrue(check_intra({'sample':ordered}))  # target overlap remains a finding

    def test_absent_metadata_retains_only_legacy_target_fit_bounds(self):
        entries,claims,coff=fixture();coff.relocations=[]
        ordered,_=order_unit('sample',entries,claims,coff)
        self.assertEqual([e.rva for e in ordered],[0x1000,0x2000])
        coff.sections[1].raw_size=3
        with self.assertRaises(ValueError):order_unit('sample',entries,claims,coff)

    def test_compiler_extent_metadata_failures_are_not_exempted(self):
        for cause in ('zero','missing','duplicate','oversize','truncated','wrong-referent','wrong-symbol-type','wrong-relocation-type','bad-relocation-section'):
            entries,claims,coff=fixture()
            if cause in ('zero','oversize'):
                data=bytearray(coff.data);struct.pack_into('<I',data,236,0 if cause=='zero' else 17);coff.data=bytes(data)
            elif cause=='missing':
                coff.relocations.pop(0);entries[0].size=259;claims[0]=claims[0]._replace(size=259)
            elif cause=='duplicate':coff.relocations.append(coff.relocations[0])
            elif cause=='truncated':coff.sections[2].raw_size=7
            elif cause=='wrong-referent':
                coff.symbols[2]=NS(name='_B',section=2,value=1,typ=0x20,storage_class=2);coff.relocations[0].symbol_index=2
            elif cause=='wrong-symbol-type':
                coff.symbols[2]=NS(name='_B',section=2,value=0,typ=0,storage_class=2);coff.relocations[0].symbol_index=2
            elif cause=='wrong-relocation-type':coff.relocations[0].typ=6
            else:coff.relocations[0].section=0
            with self.subTest(cause=cause),self.assertRaises(ValueError):order_unit('sample',entries,claims,coff)

    def test_foreign_unit_claim_rejected(self):
        entries,claims,coff=fixture();claims[0]=claims[0]._replace(unit='other')
        with self.assertRaises(ValueError):order_unit('sample',entries,claims,coff)

    def test_zero_and_negative_extents_rejected(self):
        for size in (0,-1):
            entries,claims,coff=fixture();entries[0].size=size;claims[0]=claims[0]._replace(size=size)
            with self.subTest(size=size),self.assertRaises(ValueError):order_unit('sample',entries,claims,coff)

    def test_bad_section_index_rejected_cleanly(self):
        for section in (-1,0,5):
            entries,claims,coff=fixture();coff.symbols[0].section=section
            with self.subTest(section=section),self.assertRaises(ValueError):order_unit('sample',entries,claims,coff)

    def test_code_needs_stored_bounded_payload_and_both_flags(self):
        for change in ({'raw_offset':0},{'raw_offset':12},{'raw_offset':130},
                       {'characteristics':0x20000000},{'characteristics':0x20}):
            entries,claims,coff=fixture()
            for name,value in change.items():setattr(coff.sections[1],name,value)
            with self.subTest(change=change),self.assertRaises(ValueError):order_unit('sample',entries,claims,coff)

    def test_reparsed_coff_cannot_hide_missing_payload_or_execute_only_section(self):
        from hobbit.compare.test_function_sizes import obj
        from hobbit.compare.canonicalize import CoffObject
        entry=Entry(0x1000,1,1,'_entry','src/sample.cpp')
        claim=Claim(0x1000,'_entry','func','src',1,'sample',{})
        raw=obj(b'\xc3',relocs=())
        for offset,value in ((40,0),(56,0x20000000)):
            changed=bytearray(raw);struct.pack_into('<I',changed,offset,value)
            coff=CoffObject(bytes(changed))
            with self.subTest(offset=offset),self.assertRaises(ValueError):order_unit('sample',[entry],[claim],coff)

    def test_ambiguous_folded_claim_cannot_pick_arbitrary_identity(self):
        entries,claims,coff=fixture()
        claims.append(claims[0]._replace(name='_other'))
        coff.symbols[2]=NS(name='_other',section=2,value=0,typ=0x20,storage_class=2)
        with self.assertRaises(ValueError):order_unit('sample',entries,claims,coff)

    def test_shared_position_needs_folding_proof(self):
        entries,claims,coff=fixture();coff.symbols[0].section=1
        with self.assertRaises(ValueError):order_unit('sample',entries,claims,coff)

    def test_other_section_group_is_not_given_unmeasured_order(self):
        entries,claims,coff=fixture();coff.sections[0].name='.text$X'
        with self.assertRaises(ValueError):order_unit('sample',entries,claims,coff)

    def test_ownership_interleave_and_span_overlaps_stay_findings(self):
        entries,claims,coff=fixture();ordered,_=order_unit('sample',entries,claims,coff)
        other=[Entry(0x1800,0x10,1,'C','src/other.cpp')]
        self.assertTrue(check_inter({'sample':ordered,'other':other}))
        ordered[0].size=0x1100
        self.assertTrue(check_intra({'sample':ordered}))


if __name__=='__main__':unittest.main(verbosity=2)
