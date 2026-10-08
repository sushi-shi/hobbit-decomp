"""The model's VC6 local aliases never erase external referent identities."""
import struct
import unittest
from unittest.mock import patch
from hobbit import model
from hobbit.compare.canonicalize import CoffObject, _rewrite_names, canonicalize_coff
from hobbit.compare.test_canonicalize import obj, MOV_EAX_MOFFS
from hobbit.core.msvc_names import mask


class LocalIdentity(unittest.TestCase):
    def object(self, external=False):
        raw = _rewrite_names(CoffObject(obj(bytes(4), MOV_EAX_MOFFS)),
                             {0: '_s_Initialized$HOBBIT_LOCAL_003ccff0'})
        if external:
            raw = bytearray(raw)
            symptr = struct.unpack_from('<I', raw, 8)[0]
            struct.pack_into('<h', raw, symptr + 12, 0)
            raw[symptr + 16] = 2
        return bytes(raw)

    def test_owned_definition_gets_its_source_spelling(self):
        result = canonicalize_coff(self.object())
        names = {s.name for s in CoffObject(result.data).symbols.values()}
        self.assertIn('_s_Initialized', names)

    def test_wrong_external_address_keeps_its_unique_identity(self):
        result = canonicalize_coff(self.object(external=True))
        names = {s.name for s in CoffObject(result.data).symbols.values()}
        self.assertIn('_s_Initialized$HOBBIT_LOCAL_003ccff0', names)
        self.assertNotIn('_s_Initialized', names)

    def test_actual_coff_storage_class_and_section_are_required(self):
        import tempfile
        from pathlib import Path
        raw = _rewrite_names(CoffObject(obj(bytes(4), MOV_EAX_MOFFS)),
                             {0: '_s_Initialized'})
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root / 'objdiff/base/fixture.obj'
            path.parent.mkdir(parents=True)
            with patch('hobbit.core.paths.BUILD',root):
                self.assertFalse(model._local_data_defined('fixture','_s_Initialized'))
                path.write_bytes(raw)
                self.assertTrue(model._local_data_defined('fixture','_s_Initialized'))
                external = bytearray(raw)
                symptr = struct.unpack_from('<I',external,8)[0]
                external[symptr+16] = 2
                path.write_bytes(external)
                self.assertFalse(model._local_data_defined('fixture','_s_Initialized'))
                code = bytearray(raw)
                struct.pack_into('<h',code,symptr+12,1)
                path.write_bytes(code)
                self.assertFalse(model._local_data_defined('fixture','_s_Initialized'))

    def test_only_distinct_proved_local_owners_are_disambiguated(self):
        rows = [model.Binding(r,4,'','bss','_s_Initialized',u,'src',())
                for r,u in ((0x1000,'a'),(0x2000,'b'))]
        for proof in (False,True):
            violations=[]
            with patch.object(model,'_local_data_defined',return_value=proof):
                result=model._disambiguate(rows,violations)
            self.assertEqual(bool(violations),not proof)
            if proof:
                self.assertEqual(len({r.name for r in result}),2)
                self.assertEqual({mask(r.name) for r in result},{'_s_Initialized'})
        violations=[]
        with patch.object(model,'_local_data_defined',return_value=True):
            model._disambiguate([rows[0],rows[1]._replace(unit='a')],violations)
        self.assertTrue(violations)
