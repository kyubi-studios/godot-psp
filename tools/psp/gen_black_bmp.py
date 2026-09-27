#!/usr/bin/env python3
import struct, sys
w, h = 512, 272
data = b"\x00" * (w * h * 4)
hdr = b"BM" + struct.pack("<IHHI", 54 + len(data), 0, 0, 54)
dib = struct.pack("<IiiHHIIiiII", 40, w, h, 1, 32, 0, len(data), 2835, 2835, 0, 0)
open(sys.argv[1], "wb").write(hdr + dib + data)
