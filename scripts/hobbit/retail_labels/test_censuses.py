"""Boundary controls for the census reader inherited from Gruntz."""

import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from hobbit.retail_labels import censuses


class CensusControls(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.path = Path(self.directory.name) / "census.tsv"
        image = SimpleNamespace(
            text_span=lambda: (0x1000, 0x1100),
            data_regions=lambda: {
                "rdata": (0x2000, 0x2200),
                "data": (0x3000, 0x3200),
                "bss": (0x3200, 0x3300),
            },
        )
        override = patch.object(censuses, "image", return_value=image)
        override.start()
        self.addCleanup(override.stop)

    def write(self, content):
        self.path.write_text("rva\tkind\n" + content)

    def test_function_extents_stop_at_next_start_and_section_end(self):
        self.write("0x1020\thelper\n0x1000\t\n")
        self.assertEqual(censuses.functions(self.path), [
            {"rva": 0x1000, "kind": "", "size": 0x20},
            {"rva": 0x1020, "kind": "helper", "size": 0xE0},
        ])

    def test_unresolved_bytes_are_explicit_not_assigned_to_previous_body(self):
        self.write("0x1000\t\n0x1010\tunknown\n0x1080\t\n")
        rows = censuses.functions(self.path)
        self.assertEqual([(r["kind"], r["size"]) for r in rows],
                         [("", 0x10), ("unknown", 0x70), ("", 0x80)])
        self.write("0x2000\tunknown\n")
        self.assertEqual(censuses.data(self.path)[0]["kind"], "unknown")

    def test_storage_can_cross_raw_to_zero_fill_but_not_sections(self):
        self.write("0x21fc\t\n0x31fc\t\n0x3204\t\n")
        rows = censuses.data(self.path)
        self.assertEqual([r["size"] for r in rows], [4, 8, 0xFC])
        self.assertEqual([r["region"] for r in rows], ["rdata", "data", "bss"])

    def test_rejects_duplicate_outside_unknown_kind_and_malformed_rows(self):
        cases = (
            ("0x1000\t\n0x1000\t\n", "duplicate row"),
            ("0x1100\t\n", "outside its address space"),
            ("0x1000\tbogus\n", "unknown kind"),
            ("0x1000\t\textra\n", "fields"),
        )
        for content, message in cases:
            with self.subTest(content=content):
                self.write(content)
                with self.assertRaisesRegex(ValueError, message):
                    censuses.functions(self.path)
        self.write("0x3300\t\n")
        with self.assertRaisesRegex(ValueError, "outside its address space"):
            censuses.data(self.path)


class ContributionTableControls(unittest.TestCase):
    HEADER = "seq\tunit\tstart\tend\tclass\tmodule\tn\tevidence\tnotes\n"

    def read(self, *rows, header=None):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "link_order.tsv"
            path.write_text((self.HEADER if header is None else header)
                            + "".join("\t".join(row) + "\n" for row in rows))
            return censuses.link_order_bands(path)

    def row(self, **overrides):
        values = dict(zip(self.HEADER.rstrip().split("\t"),
                          ["0", "probe", "0x1000", "0x1010", "cmdline", "game", "1", "fixture", ""]))
        values.update(overrides)
        return list(values.values())

    def test_valid_header_only_and_nonspan_owner_tables(self):
        self.assertEqual(self.read(), [])
        self.assertEqual(self.read(self.row()), [(0x1000, 0x1010, "probe")])
        self.assertEqual(self.read(self.row(start="", end="", **{"class": "comdat-owner"})), [])

    def test_rejects_short_rows_and_wrong_header(self):
        with self.assertRaisesRegex(ValueError, "fields"):
            self.read(["0", "probe", "bad"])
        with self.assertRaisesRegex(ValueError, "header"):
            self.read(header="unit\tstart\tend\n")

    def test_rejects_nonhex_missing_reversed_and_overlapping_spans(self):
        for override in ({"start": "garbage"}, {"start": ""}, {"end": "garbage"},
                         {"end": "0x1000"}, {"end": "0x100000001"},
                         {"class": "mystery"}, {"unit": ""}, {"n": "-1"}):
            with self.subTest(override=override), self.assertRaises(ValueError):
                self.read(self.row(**override))
        with self.assertRaisesRegex(ValueError, "overlaps"):
            self.read(self.row(), self.row(seq="1", unit="other", start="0x1008"))

    def test_missing_table_remains_explicitly_absent(self):
        with tempfile.TemporaryDirectory() as td:
            with self.assertRaises(FileNotFoundError):
                censuses.link_order_bands(Path(td) / "absent.tsv")


if __name__ == "__main__":
    unittest.main()
