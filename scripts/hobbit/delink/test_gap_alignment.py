"""Alignment slack is not a missing datum; larger unknown gaps stay visible."""
import unittest
from types import SimpleNamespace
from unittest.mock import patch
from hobbit.delink import data_manifest as manifest


class GapAlignment(unittest.TestCase):
    def gaps(self, next_rva, type_name):
        rows = [dict(name='_clock32',rva=0x1000,size=4,object='timer.c',storage='bss'),
                dict(name='_clock64',rva=next_rva,size=8,object='timer.c',storage='bss')]
        image=SimpleNamespace(relocs_in=lambda lo,hi:[])
        with patch.object(manifest,'retail',return_value=image), \
             patch.object(manifest,'_classify',return_value='data-loader-zero-tail'), \
             patch.object(manifest,'declared_types',return_value={next_rva:type_name}):
            return manifest.gap_rows(rows,[])

    def test_declared_wide_scalar_alignment_explains_four_bytes(self):
        rows,withheld=self.gaps(0x1008,'__int64')
        self.assertFalse(rows)
        self.assertIn('alignment',withheld[0][2])

    def test_unknown_or_larger_zero_gap_stays_visible(self):
        for rva,type_name in ((0x1008,'unknown'),(0x1010,'__int64')):
            rows,withheld=self.gaps(rva,type_name)
            self.assertEqual(rows[0]['size'],rva-0x1004)
            self.assertFalse(withheld)
