"""Wrapped instructions in the Gruntz audit port."""
import unittest
from unittest import mock

from hobbit.verify import access_map


class DataAccessFieldTests(unittest.TestCase):

    def test_wrapped_mov_is_one_complete_instruction(self):
        listing = ("  248c83:\tc7 05 f4 cf 7c 00 28 \tmov DWORD PTR ds:0x7ccff4,0x74c628\n"
                   "  248c8a:\tc6 74 00 \n")
        with mock.patch('hobbit.tool.objdump.disassemble', return_value=listing):
            dec = access_map._decode(bytes.fromhex('c705f4cf7c0028c67400'), 0x248c83)
        self.assertEqual(dec.starts, [0x248c83])
        self.assertEqual(dec.length(0), 10)
        self.assertEqual(dec.at(0x248c89), 0)




if __name__ == '__main__':
    unittest.main()
