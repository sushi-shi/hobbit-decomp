"""Unlabelled engine providers need source identity, not object-only claims."""
import os
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
from hobbit.retail_labels import source

class HeaderProviders(unittest.TestCase):
    def test_missing_and_source_stale_object_do_not_select(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);src=root/'provider.cpp';src.write_text('// original caller')
            with patch.object(source,'REPO',root),patch.object(source,'BASE_OBJS',root):
                self.assertEqual(source.emitted_header_candidates('provider','provider.cpp',{'header_func'}),set())
                obj=root/'provider.obj';obj.write_bytes(b'old');os.utime(obj,ns=(1,1))
                self.assertEqual(source.emitted_header_candidates('provider','provider.cpp',{'header_func'}),set())
    def test_presence_filter_requires_defined_matching_symbol(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);src=root/'provider.cpp';src.write_text('// caller');obj=root/'provider.obj';obj.write_bytes(b'coff')
            symbols={0:SimpleNamespace(name='header_func',section=1),1:SimpleNamespace(name='extern_only',section=0),2:SimpleNamespace(name='unannotated',section=2)}
            with patch.object(source,'REPO',root),patch.object(source,'BASE_OBJS',root),patch('hobbit.compare.canonicalize.CoffObject',return_value=SimpleNamespace(symbols=symbols)):
                self.assertEqual(source.emitted_header_candidates('provider','provider.cpp',{'header_func','extern_only'}),{'header_func'})

class ColdHeaderProviders(unittest.TestCase):
    def _extract(self, emitted):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);(root/'provider.cpp').write_text('// genuine caller, no own annotations')
            # Names are obtained from the existing exact LLVM annotation join;
            # the object independently decides whether this TU emits the body.
            ir='''@annotation = private unnamed_addr constant [22 x i8] c"rva:0x1234 size:0x8\\00"
@llvm.global.annotations = appending global [1 x { ptr, ptr, ptr, i32, ptr }] [{ ptr, ptr, ptr, i32, ptr } { ptr @"?NewHeader@@YAXXZ", ptr @annotation, ptr null, i32 1, ptr null }]
'''
            with patch.object(source,'REPO',root),patch.object(source,'header_annotation_names',return_value=set()),patch.object(source,'emitted_header_candidates',return_value=set()),patch.object(source,'emitted_executable_names',return_value=emitted),patch.object(source.clang,'emit_ir',return_value=ir):
                return source.extract_unit('provider','provider.cpp',{},set(),True)
    def test_new_header_without_any_cached_claim_uses_current_llvm_and_coff(self):
        rows,problems=self._extract({'?NewHeader@@YAXXZ'})
        self.assertFalse(problems)
        self.assertEqual([(r[0],r[2],r[3],r[4])for r in rows],[('0x00001234','?NewHeader@@YAXXZ','func','src')])
    def test_annotation_without_actual_emission_creates_no_provider(self):
        rows,problems=self._extract({'?Unrelated@@YAXXZ'})
        self.assertEqual(rows,[])
        self.assertFalse(problems)
    def test_cold_presence_filter_rejects_undefined_data_and_nonfunction(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);(root/'provider.cpp').write_text('// caller');(root/'provider.obj').write_bytes(b'coff')
            syms={i:SimpleNamespace(name=n,section=sec,typ=typ)for i,(n,sec,typ)in enumerate([('real_function',1,0x20),('undefined',0,0x20),('data',2,0x20),('text_label',1,0)])}
            coff=SimpleNamespace(symbols=syms,sections=[SimpleNamespace(characteristics=0x20000000),SimpleNamespace(characteristics=0x40000040)])
            with patch.object(source,'REPO',root),patch.object(source,'BASE_OBJS',root),patch('hobbit.compare.canonicalize.CoffObject',return_value=coff):
                self.assertEqual(source.emitted_executable_names('provider','provider.cpp'),{'real_function'})
