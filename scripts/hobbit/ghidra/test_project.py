"""Viewer probes must follow admitted Hobbit evidence, never donor constants."""
import unittest
from unittest.mock import patch
from types import SimpleNamespace
from hobbit.ghidra import export
from hobbit.ghidra.project import sample_rvas


class SampleAddresses(unittest.TestCase):
    def test_claims_preferred_and_unknown_spaces_omitted(self):
        doc = {'text': [{'rva': 0x123, 'channel': ''},
                        {'rva': 0x456, 'channel': 'src'}],
               'data': [{'rva': 0x789, 'channel': ''}]}
        self.assertEqual(sample_rvas(doc), (0x456, 0x789))
        self.assertEqual(sample_rvas({'text': [], 'data': []}), ())

    def test_unknown_intervals_are_not_functions_or_data(self):
        def row(space, kind, rva):
            return dict(space=space, kind=kind, rva=rva, size='0x10',
                        name='', unit='', channel='', also_units='', aliases='')
        rows = [row('text', '', '0x1000'), row('text', 'unknown', '0x1010'),
                row('rdata', 'unknown', '0x2000'), row('rdata', 'string', '0x2010')]
        with patch.object(export, 'bindings', return_value=rows), \
             patch.object(export, 'bands', return_value=[]), \
             patch('hobbit.core.pe.image', return_value=SimpleNamespace(
                 image_base=0x400000, path='Meridian.exe')):
            doc = export.payload()
        self.assertEqual([r['rva'] for r in doc['text']], [0x1000])
        self.assertEqual([r['rva'] for r in doc['data']], [0x2010])
        self.assertEqual([r['rva'] for r in doc['regions']], [0x1010, 0x2000])
        self.assertEqual(doc['counts']['unknown_regions'], 2)


if __name__ == '__main__':
    unittest.main()
