"""Contribution ownership and exact access counts in the donor coverage audit."""
import random
import unittest

from hobbit.verify import data_coverage as dc


class Image:
    def read(self, lo, size): return b'\x01' * size
    def relocs_in(self, lo, hi): return []
    def refs_to_range(self, lo, hi): return []
    def section_name(self, rva): return '.rdata'


def fixture():
    claims, sections = [], []
    for ordinal, rva in [(1, 0x1000), (2, 0x1010)]:
        claim = dict(name=f'literal{ordinal}', object='consumer', rva=rva,
                     size=4, storage='rdata', section_ordinal=ordinal,
                     section_offset=0, provenance='candidate-COFF-real',
                     origin='manifest')
        claims.append(claim)
        claims.append({k: v for k, v in dict(claim, origin='model').items()
                       if k not in ('section_ordinal', 'section_offset', 'provenance')})
        sections.append(dict(name='.rdata', object='consumer', rva=rva,
                             size=4, storage='rdata', ordinal=ordinal,
                             characteristics=0x40301040, comdat_selection=2,
                             associative_ordinal='-',
                             provenance='candidate-COFF-section'))
    return claims, sections


def gap(claims, sections):
    touched = ([0x1004], [0x1010], ([0x1004], [0x1010]))
    return dc.gaps(Image(), claims, sections, touched)[0]


class DataCoverageOwnershipControls(unittest.TestCase):
    def test_complete_independent_sections_remain_unresolved(self):
        claims, sections = fixture()
        row = gap(claims, sections)
        self.assertEqual((row['rva'], row['length'], row['touched']),
                         (0x1004, 12, 12))
        self.assertEqual(row['ownership_scope'], 'unresolved-inter-contribution')
        self.assertEqual(dc.gate_rows([row]), [])

    def test_ordinary_same_unit_gap_still_fails(self):
        claims, _ = fixture()
        claims = [dict(c, origin='model') for c in claims]
        row = gap(claims, [])
        self.assertEqual(row['owner_units'], ['consumer'])
        self.assertEqual(len(dc.gate_rows([row])), 1)

    def test_real_symbol_identity_not_just_equal_extent(self):
        claims, sections = fixture()
        for c in list(claims):
            if c['origin'] == 'model':
                claims.append(dict(c, name='ordinary_' + c['name']))
        self.assertEqual(len(dc.gate_rows([gap(claims, sections)])), 1)

    def test_model_different_storage_is_not_a_duplicate(self):
        claims, sections = fixture()
        claims = [dict(c, storage='data') if c['origin'] == 'model' else c
                  for c in claims]
        self.assertEqual(len(dc.gate_rows([gap(claims, sections)])), 1)

    def test_missing_claim_fields_do_not_suppress(self):
        for field in ('section_ordinal', 'section_offset', 'provenance'):
            with self.subTest(field=field):
                claims, sections = fixture()
                for c in claims:
                    if c['origin'] == 'manifest': c.pop(field)
                self.assertEqual(len(dc.gate_rows([gap(claims, sections)])), 1)

    def test_bad_manifest_duplicates_preserve_ordinary_witness(self):
        claims, sections = fixture()
        claims.extend(dict(c, section_offset=1) for c in list(claims)
                      if c['origin'] == 'manifest')
        self.assertEqual(len(dc.gate_rows([gap(claims, sections)])), 1)

    def test_incomplete_section_metadata_does_not_suppress(self):
        for field in ('ordinal', 'characteristics', 'comdat_selection',
                      'associative_ordinal', 'provenance'):
            with self.subTest(field=field):
                claims, sections = fixture()
                for s in sections: s.pop(field)
                self.assertEqual(len(dc.gate_rows([gap(claims, sections)])), 1)

    def test_wrong_section_metadata_does_not_suppress(self):
        for field, value in [('characteristics', 0x40300040),
                             ('comdat_selection', 5), ('associative_ordinal', 1),
                             ('associative_ordinal', 'garbage'),
                             ('provenance', 'unproven'), ('storage', 'data')]:
            with self.subTest(field=field, value=value):
                claims, sections = fixture()
                for s in sections: s[field] = value
                self.assertEqual(len(dc.gate_rows([gap(claims, sections)])), 1)

    def test_conflicting_sections_do_not_suppress(self):
        claims, sections = fixture()
        sections.extend(dict(s, size=3) for s in list(sections))
        self.assertEqual(len(dc.gate_rows([gap(claims, sections)])), 1)

    def test_ordinary_witnesses_survive_folded_alias_sorting(self):
        claims, sections = fixture()
        for c in list(claims):
            if c['origin'] == 'model':
                claims.append(dict(c, object='ordinary', name='z_' + c['name']))
        expected = gap(claims, sections)
        self.assertEqual(expected['owner_units'], ['ordinary'])
        self.assertEqual(len(dc.gate_rows([expected])), 1)
        for seed in range(20):
            rng = random.Random(seed)
            rng.shuffle(claims); rng.shuffle(sections)
            row = gap(claims, sections)
            self.assertEqual(row['owner_units'], expected['owner_units'])
            self.assertEqual(row['ownership_scope'], expected['ownership_scope'])
            self.assertEqual(len(dc.gate_rows([row])), 1)

    def test_folded_aliases_cannot_witness_gap_ownership(self):
        claims, sections = fixture()
        claims.extend(dict(c, object='other') for c in list(claims))
        sections.extend(dict(s, object='other') for s in list(sections))
        for seed in range(20):
            rng = random.Random(seed)
            rng.shuffle(claims); rng.shuffle(sections)
            row = gap(claims, sections)
            self.assertEqual(row['ownership_scope'], 'unresolved-inter-contribution')
            self.assertEqual(dc.gate_rows([row]), [])

    def test_provisional_claims_cannot_establish_independence(self):
        for provenance in ('provisional-band-gap', '-', 'unproven'):
            with self.subTest(provenance=provenance):
                claims, sections = fixture()
                for c in claims:
                    if c['origin'] == 'manifest': c['provenance'] = provenance
                self.assertEqual(len(dc.gate_rows([gap(claims, sections)])), 1)

    def test_section_only_conflicts_preserve_ordinary_witness(self):
        for change in ({'size': 3}, {'storage': 'data'}):
            with self.subTest(change=change):
                _, sections = fixture()
                sections.extend(dict(s, **change) for s in list(sections))
                self.assertEqual(len(dc.gate_rows([gap([], sections)])), 1)

    def test_exact_duplicate_sections_remain_independent(self):
        _, sections = fixture()
        sections.extend(dict(s) for s in list(sections))
        row = gap([], sections)
        self.assertEqual(row['ownership_scope'], 'unresolved-inter-contribution')
        self.assertEqual(dc.gate_rows([row]), [])

    def test_extent_overlap_still_fails(self):
        claims, sections = fixture()
        claims.append(dict(claims[0], name='bad_extent', size=8))
        self.assertEqual(len(dc.overlaps(claims)), 1)


class DataCoverageTouchControls(unittest.TestCase):
    def check(self, ranges, lo, hi):
        starts, ends, sites = dc._touch_ranges(ranges)
        got = dc._touched(starts, ends, sites, lo, hi)
        count = sum(a < hi and b > lo for a, b in ranges)
        touched = set()
        for a, b in ranges: touched.update(range(max(a, lo), min(b, hi)))
        self.assertEqual(got, (len(touched), count))

    def test_merged_adjacent_accesses_are_not_charged_to_gap(self):
        self.check([(0, 4)] * 100 + [(4, 8)] + [(8, 12)] * 20, 4, 8)

    def test_crossing_nested_disjoint_duplicate_and_empty(self):
        for ranges in ([], [(0, 20)], [(0, 20), (3, 4)],
                       [(0, 4), (8, 12)], [(0, 20)] * 3):
            for lo, hi in [(1, 3), (4, 8), (0, 20), (25, 28)]:
                self.check(ranges, lo, hi)

    def test_random_intersection_oracle(self):
        rng = random.Random(617)
        for _ in range(1000):
            ranges = []
            for __ in range(rng.randrange(100)):
                lo = rng.randrange(100)
                ranges.append((lo, lo + rng.randrange(1, 30)))
            lo = rng.randrange(120)
            self.check(ranges, lo, lo + rng.randrange(1, 30))


if __name__ == '__main__':
    unittest.main(verbosity=2)
