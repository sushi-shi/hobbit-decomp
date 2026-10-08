"""Hobbit VC6 prefix unwind tables are not the donor's VC5 tail layout."""
import struct
import unittest
from types import SimpleNamespace
from hobbit.delink.eh_band import _funclets, Group, data_records


class ExceptionTableControls(unittest.TestCase):
    def image(self, *, pointer=0x403000, to_state=0, try_map=0):
        header=struct.pack('<IiIiIiI',0x19930520,2,pointer,0,try_map,0,0)
        table=struct.pack('<iIiI',-1,0x401100,to_state,0x401120)
        blobs={0x3010:header,0x3000:table}
        return SimpleNamespace(image_base=0x400000,text_lo=0x1000,text_hi=0x2000,
                               read=lambda rva,size:blobs.get(rva))

    def test_prefix_map_and_exact_record_extent(self):
        self.assertEqual(_funclets(self.image(),0x3010),({0x1100,0x1120},2,True,True))
        g=Group(0x1000,'_Owner','unit',(0x1100,0x1120),0x1140,
                funcinfo=0x3010,states=2,packed=True,simple=True,unwind_map=0x3000)
        rows=data_records([g])
        self.assertEqual([(r[0],r[3]) for r in rows],[(0x3010,28),(0x3000,16)])

    def test_bad_state_and_unexpected_try_table_are_not_simple_records(self):
        self.assertIsNone(_funclets(self.image(to_state=1),0x3010))
        parsed=_funclets(self.image(try_map=0x403100),0x3010)
        self.assertFalse(parsed[3])


if __name__=='__main__':unittest.main()
