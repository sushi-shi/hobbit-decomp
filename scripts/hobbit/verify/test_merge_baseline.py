import unittest

from hobbit.verify.merge_baseline import merge

HEAD = "# [functions]\tunit\tfunction\tbest_pct\tcur_pct\ttries\tsrc_hash\trva\thist_pct\tstate\n"


def row(fn, best, cur, fp, hist):
    return f"u\t{fn}\t{best}\t{cur}\t1\t{fp}\t0x1\t{hist}\t\n"


def rows(text):
    return {line.split("\t")[1]: line.split("\t") for line in text.splitlines()
            if line and not line.startswith("#")}


class MergeBaselineTests(unittest.TestCase):
    def test_untouched_side_never_wins(self):
        base = HEAD + row("a", 90, 90, "old", 90) + row("b", 100, 100, "old", 100)
        # main reset a (edited) and re-banked b under a new hash; lane touched neither
        main = HEAD + row("a", 66, 66, "new", 90) + row("b", 100, 95, "new", 100)
        lane = HEAD + row("a", 90, 70, "old", 90) + row("b", 100, 99, "old", 100)
        out, taken = merge(base, main, lane)
        got = rows(out)
        self.assertEqual((got["a"][2], got["a"][5]), ("66", "new"))
        self.assertEqual((got["b"][2], got["b"][5]), ("100", "new"))
        self.assertEqual(taken, 0)

    def test_lane_banked_row_is_kept(self):
        base = HEAD + row("c", 80, 80, "h1", 80)
        main = HEAD + row("c", 80, 79, "h1", 80)          # CUR only
        lane = HEAD + row("c", 100, 100, "h2", 100)       # probe-banked exact
        out, taken = merge(base, main, lane)
        self.assertEqual(rows(out)["c"][2], "100")
        self.assertEqual(taken, 1)

    def test_both_changed_takes_higher_max_and_max_hist(self):
        base = HEAD + row("d", 80, 80, "h1", 85)
        main = HEAD + row("d", 90, 90, "h2", 90)
        lane = HEAD + row("d", 95, 95, "h3", 95)
        got = rows(merge(base, main, lane)[0])["d"]
        self.assertEqual((got[2], got[5], got[7]), ("95", "h3", "95.0000"))


if __name__ == "__main__":
    unittest.main()
