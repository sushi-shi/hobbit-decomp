"""IR-bound DATA claims retain declaration types used by alignment proofs."""
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
from hobbit.retail_labels import source


class DataTypes(unittest.TestCase):
    def test_ir_covered_data_keeps_canonical_type_without_ast_fallback(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            (root/'timer.cpp').write_text('DATA(0x1000) static __int64 base;\n')
            with patch.object(source,'REPO',root), \
                 patch.object(source.clang,'emit_ir',return_value='fixture'), \
                 patch.object(source,'ir_claims',return_value=([],[(0x1000,'_base')])), \
                 patch.object(source.clang,'var_facts',return_value={'_base':dict(size=8,internal=True,defined=True,type='long long')}), \
                 patch.object(source,'message_map_claims',return_value=([],[])), \
                 patch.object(source.clang,'ast_dump',side_effect=AssertionError('IR already binds declaration')):
                rows,problems=source.extract_unit('timer','timer.cpp',{})
            self.assertFalse(problems)
            self.assertEqual(rows,[['0x00001000','0x8','_base','data','src','long long']])
