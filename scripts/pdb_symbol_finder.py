#!/usr/bin/env python3
"""
pdb_symbol_finder.py - Query symbols and modules from tr7.pdb
"""

import sys
import struct

def search_symbols(pdb_path, query):
    with open(pdb_path, 'rb') as f:
        data = f.read()
    
    q_bytes = query.encode('ascii')
    idx = 0
    results = []
    while True:
        idx = data.find(q_bytes, idx)
        if idx == -1:
            break
        # Scan backward for record start
        for back in range(4, 60):
            pos = idx - back
            if pos < 0:
                continue
            reclen, rectype = struct.unpack_from('<HH', data, pos)
            if pos + reclen + 2 < idx:
                continue
            # S_PUB32 (0x110e)
            if rectype == 0x110e:
                pubflags, offset, seg = struct.unpack_from('<IIH', data, pos + 4)
                name_bytes = data[pos + 14: pos + reclen + 2].split(b'\x00')[0]
                results.append((rectype, seg, offset, name_bytes.decode('ascii', errors='ignore'), 0))
                break
            # S_GPROC32 (0x1110) or S_LPROC32 (0x110f)
            elif rectype in (0x1110, 0x110f):
                pParent, pEnd, pNext, procLen, dStart, dEnd, ptype, offset, seg = struct.unpack_from('<IIIIIIIIH', data, pos + 4)
                name_bytes = data[pos + 39: pos + reclen + 2].split(b'\x00')[0]
                results.append((rectype, seg, offset, name_bytes.decode('ascii', errors='ignore'), procLen))
                break
            # S_GDATA32 (0x110d) or S_LDATA32 (0x110c)
            elif rectype in (0x110d, 0x110c):
                ptype, offset, seg = struct.unpack_from('<IIH', data, pos + 4)
                name_bytes = data[pos + 14: pos + reclen + 2].split(b'\x00')[0]
                results.append((rectype, seg, offset, name_bytes.decode('ascii', errors='ignore'), 0))
                break
        idx += len(q_bytes)
    return results

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: pdb_symbol_finder.py <symbol_name>")
        sys.exit(1)
    pdb = '/home/h/Documents/trl_decomp/GAME_DATA/Tomb Raider Legend/tr7.pdb'
    res = search_symbols(pdb, sys.argv[1])
    seen = set()
    for r in res:
        key = (r[1], r[2], r[3])
        if key not in seen:
            seen.add(key)
            rectype_str = {0x110e: 'PUB', 0x1110: 'GPROC', 0x110f: 'LPROC', 0x110d: 'GDATA', 0x110c: 'LDATA'}.get(r[0], hex(r[0]))
            print(f"[{rectype_str}] Seg: {r[1]} Offset: 0x{r[2]:08x} (PDB VA: 0x{0x00401000 + r[2] if r[1] == 1 else r[2]:08x}) Size: 0x{r[4]:x} Name: {r[3]}")
