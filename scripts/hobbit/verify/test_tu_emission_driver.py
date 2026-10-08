from pathlib import Path
import tempfile
import unittest
import contextlib
import io
from unittest.mock import patch

from hobbit.verify import tu_order as gate
from hobbit.verify import tu_emission as helper


class EvidenceErrorTests(unittest.TestCase):
    def setUp(self):
        self.rows = {'sample': [gate.Entry(0x1000, 4, 1, 'A', 'src/sample.cpp')]}

    def test_missing_or_stale_build_retains_every_entry_and_reports_error(self):
        for error in (ValueError('stale source/header'), OSError('missing ninja')):
            with self.subTest(error=str(error)), patch.object(
                    helper, 'require_current_units', side_effect=error):
                ordered, findings = gate.load_in_emission_order(self.rows)
                self.assertIs(ordered, self.rows)
                self.assertEqual(len(findings), 1)
                self.assertIn('EVIDENCE ERROR', findings[0])

    def test_missing_object_is_not_a_dropped_unit(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(
                helper, 'require_current_units'), patch.object(gate, 'BUILD', Path(directory)):
            ordered, findings = gate.load_in_emission_order(self.rows)
            self.assertIs(ordered['sample'], self.rows['sample'])
            self.assertEqual(len(findings), 1)
            self.assertIn('EVIDENCE ERROR: sample', findings[0])

    def test_malformed_claims_are_explicit_evidence_errors(self):
        from hobbit.compare.test_function_sizes import obj
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory); (root/'objdiff/base').mkdir(parents=True)
            (root/'objdiff/base/sample.obj').write_bytes(obj(b'\xc3',relocs=()))
            with patch.object(helper, 'require_current_units'), patch.object(gate, 'BUILD', root), \
                    patch('hobbit.retail_labels.fragments.unit_claims', side_effect=KeyError('rva')):
                ordered, findings = gate.load_in_emission_order(self.rows)
            self.assertIs(ordered['sample'], self.rows['sample'])
            self.assertEqual(len(findings), 1)
            self.assertIn('EVIDENCE ERROR: sample', findings[0])

    def test_update_cannot_bless_failed_or_missing_evidence(self):
        for finding in ('tu-order EVIDENCE ERROR: stale object',
                        'tu-order EXILE LEDGER STALE: missing owner',
                        'tu-order: 0 RVA-labelled function(s) found'):
            with self.subTest(finding=finding), patch.object(
                    gate, 'gate_findings', return_value=([finding], ({}, 0, 0, 0))), \
                    patch.object(gate, '_write_baseline') as write, \
                    contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(gate.main(['--update']), 1)
                write.assert_not_called()

    def test_empty_scan_does_not_trigger_default_ninja_build(self):
        with patch.object(helper, 'require_current_units') as required:
            self.assertEqual(gate.load_in_emission_order({}), ({}, []))
            required.assert_not_called()


if __name__ == '__main__':
    unittest.main(verbosity=2)
