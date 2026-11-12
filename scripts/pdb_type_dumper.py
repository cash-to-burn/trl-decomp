#!/usr/bin/env python3
"""
pdb_type_dumper.py - Dump struct definitions from PDB TPI stream (Stream 2)
"""

import sys
import struct

def get_stream_data(pdb_path, stream_idx):
    with open(pdb_path, 'rb') as f:
        data = f.read()

    magic, block_size, free_map, num_blocks, dir_size, reserved, block_dir = struct.unpack_from('<32sIIIIII', data, 0)
    dir_blocks_needed = (dir_size + block_size - 1) // block_size
    block_dir_offset = block_dir * block_size
    dir_block_indices = struct.unpack_from(f'<{dir_blocks_needed}I', data, block_dir_offset)

    dir_bytes = bytearray()
    for b in dir_block_indices:
        dir_bytes.extend(data[b * block_size : (b + 1) * block_size])
    dir_bytes = dir_bytes[:dir_size]

    num_streams = struct.unpack_from('<I', dir_bytes, 0)[0]
    stream_sizes = struct.unpack_from(f'<{num_streams}I', dir_bytes, 4)
    offset = 4 + num_streams * 4

    for i in range(stream_idx):
        size = stream_sizes[i]
        if size != 0xffffffff:
            blocks_needed = (size + block_size - 1) // block_size
            offset += blocks_needed * 4

    size = stream_sizes[stream_idx]
    if size == 0xffffffff or size == 0:
        return b''
    blocks_needed = (size + block_size - 1) // block_size
    blocks = struct.unpack_from(f'<{blocks_needed}I', dir_bytes, offset)
    out = bytearray()
    for b in blocks:
        out.extend(data[b * block_size : (b + 1) * block_size])
    return bytes(out[:size])

def parse_tpi(tpi_bytes):
    version, hdr_size, ti_begin, ti_end, type_record_bytes = struct.unpack_from('<IIIII', tpi_bytes, 0)
    records = {}
    offset = hdr_size
    ti = ti_begin
    while offset < hdr_size + type_record_bytes:
        rec_len = struct.unpack_from('<H', tpi_bytes, offset)[0]
        rec_data = tpi_bytes[offset + 2 : offset + 2 + rec_len]
        rec_type = struct.unpack_from('<H', rec_data, 0)[0]
        records[ti] = (rec_type, rec_data[2:])
        offset += 2 + rec_len
        ti += 1
    return records

def parse_numeric(data, off):
    val = struct.unpack_from('<H', data, off)[0]
    if val < 0x8000:
        return val, off + 2
    elif val == 0x8000: # LF_CHAR
        return struct.unpack_from('<b', data, off + 2)[0], off + 3
    elif val == 0x8001: # LF_SHORT
        return struct.unpack_from('<h', data, off + 2)[0], off + 4
    elif val == 0x8002: # LF_USHORT
        return struct.unpack_from('<H', data, off + 2)[0], off + 4
    elif val == 0x8003: # LF_LONG
        return struct.unpack_from('<i', data, off + 2)[0], off + 6
    elif val == 0x8004: # LF_ULONG
        return struct.unpack_from('<I', data, off + 2)[0], off + 6
    return val, off + 2

def get_type_name(records, ti):
    primitive_types = {
        0x0003: "void",
        0x0010: "signed char",
        0x0011: "unsigned char",
        0x0012: "char",
        0x0020: "short",
        0x0021: "unsigned short",
        0x0022: "int",
        0x0023: "unsigned int",
        0x0040: "float",
        0x0041: "double",
        0x0074: "int",
        0x0075: "unsigned int",
        0x0070: "short",
        0x0071: "unsigned short",
        0x0072: "char",
        0x0073: "unsigned char",
        0x0030: "bool",
        0x0470: "char*",
        0x0103: "void*",
    }
    if ti in primitive_types:
        return primitive_types[ti]
    if ti not in records:
        return f"type_0x{ti:x}"
    rtype, rdata = records[ti]
    if rtype in (0x1505, 0x1504): # LF_STRUCTURE, LF_CLASS
        count, prop, fieldlist_ti, derived, vshape = struct.unpack_from('<HHIII', rdata, 0)
        size, off = parse_numeric(rdata, 16)
        name = rdata[off:].split(b'\x00')[0].decode('latin1', errors='ignore')
        return name
    elif rtype == 0x1507: # LF_UNION
        count, prop, fieldlist_ti = struct.unpack_from('<HHI', rdata, 0)
        size, off = parse_numeric(rdata, 8)
        name = rdata[off:].split(b'\x00')[0].decode('latin1', errors='ignore')
        return name
    elif rtype == 0x1002: # LF_POINTER
        target_ti, ptr_attr = struct.unpack_from('<II', rdata, 0)
        return get_type_name(records, target_ti) + "*"
    elif rtype == 0x1503: # LF_ARRAY
        elem_ti, idx_ti = struct.unpack_from('<II', rdata, 0)
        arr_size, off = parse_numeric(rdata, 8)
        return f"{get_type_name(records, elem_ti)}[{arr_size}]"
    elif rtype == 0x1502: # LF_MODIFIER
        mod_ti, mod_attr = struct.unpack_from('<IH', rdata, 0)
        prefix = "const " if mod_attr & 1 else ""
        return prefix + get_type_name(records, mod_ti)
    return f"type_0x{ti:x}"

def dump_struct(records, target_name):
    LF_STRUCTURE = 0x1505
    LF_CLASS = 0x1504
    LF_FIELDLIST = 0x1203
    LF_MEMBER = 0x150d
    LF_METHOD = 0x150f
    LF_NESTTYPE = 0x1510
    LF_VFUNCTAB = 0x1409
    LF_BCLASS = 0x1400

    for ti, (rtype, rdata) in records.items():
        if rtype in (LF_STRUCTURE, LF_CLASS):
            count, prop, fieldlist_ti, derived, vshape = struct.unpack_from('<HHIII', rdata, 0)
            size, off = parse_numeric(rdata, 16)
            name = rdata[off:].split(b'\x00')[0].decode('latin1', errors='ignore')
            if name == target_name:
                is_fwd = (prop & 0x80) != 0
                if is_fwd:
                    continue
                print(f"struct {name} // size: 0x{size:x} ({size} bytes), TI: 0x{ti:x}")
                print("{")
                if fieldlist_ti in records:
                    fl_type, fl_data = records[fieldlist_ti]
                    fl_off = 0
                    while fl_off < len(fl_data):
                        subtype = struct.unpack_from('<H', fl_data, fl_off)[0]
                        if subtype == LF_MEMBER:
                            attr = struct.unpack_from('<H', fl_data, fl_off + 2)[0]
                            mti = struct.unpack_from('<I', fl_data, fl_off + 4)[0]
                            field_offset, name_off = parse_numeric(fl_data, fl_off + 8)
                            fname = fl_data[name_off:].split(b'\x00')[0].decode('latin1', errors='ignore')
                            tname = get_type_name(records, mti)
                            print(f"    /* 0x{field_offset:03x} */ {tname} {fname};")
                            fl_off = (name_off + len(fname) + 1 + 3) & ~3
                        elif subtype == LF_BCLASS:
                            attr = struct.unpack_from('<H', fl_data, fl_off + 2)[0]
                            bti = struct.unpack_from('<I', fl_data, fl_off + 4)[0]
                            base_off, next_off = parse_numeric(fl_data, fl_off + 8)
                            tname = get_type_name(records, bti)
                            print(f"    /* [BaseClass +0x{base_off:03x}] */ {tname};")
                            fl_off = (next_off + 3) & ~3
                        elif subtype == LF_METHOD:
                            count_m, mlist = struct.unpack_from('<HH', fl_data, fl_off + 2)
                            mname_start = fl_off + 6
                            mname_str = fl_data[mname_start:].split(b'\x00')[0].decode('latin1', errors='ignore')
                            fl_off = (mname_start + len(mname_str) + 1 + 3) & ~3
                        elif subtype == LF_NESTTYPE:
                            nt_off = fl_off + 6
                            nt_name = fl_data[nt_off:].split(b'\x00')[0].decode('latin1', errors='ignore')
                            fl_off = (nt_off + len(nt_name) + 1 + 3) & ~3
                        elif subtype == LF_VFUNCTAB:
                            fl_off = (fl_off + 8 + 3) & ~3
                        else:
                            if subtype >= 0xf0:
                                pad_len = subtype - 0xf0
                                fl_off += pad_len
                            else:
                                fl_off += 2
                print("};")
                print()

if __name__ == '__main__':
    pdb_path = '/home/h/Documents/trl_decomp/GAME_DATA/Tomb Raider Legend/tr7.pdb'
    tpi_bytes = get_stream_data(pdb_path, 2)
    records = parse_tpi(tpi_bytes)
    target = sys.argv[1] if len(sys.argv) > 1 else 'Object'
    dump_struct(records, target)
