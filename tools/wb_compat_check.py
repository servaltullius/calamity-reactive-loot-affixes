#!/usr/bin/env python3
"""Wrye Bash compatibility checker for Skyrim SE ESP files.

Parses the ESP at the binary level and flags issues that are known to
cause exceptions in Wrye Bash's Python parser.
"""
import struct
import sys
from pathlib import Path

def read_uint16(f): return struct.unpack('<H', f.read(2))[0]
def read_uint32(f): return struct.unpack('<I', f.read(4))[0]
def read_float(f):  return struct.unpack('<f', f.read(4))[0]

KNOWN_RECORD_TYPES = {
    b'TES4', b'GRUP', b'KYWD', b'MGEF', b'SPEL', b'MISC', b'QUST',
    b'NPC_', b'ARMO', b'WEAP', b'AMMO', b'LVLI', b'LVLN', b'LVSP',
    b'ENCH', b'ALCH', b'INGR', b'SCRL', b'BOOK', b'FLST', b'PERK',
    b'COBJ', b'CONT', b'DOOR', b'FURN', b'LIGH', b'STAT', b'TREE',
    b'ACTI', b'FACT', b'GLOB', b'GMST', b'RACE', b'CELL', b'WRLD',
    b'DIAL', b'INFO', b'IDLE', b'PACK', b'SNDR', b'SOUN', b'EFSH',
    b'IPDS', b'IPCT', b'PROJ', b'EXPL', b'HAZD', b'TXST', b'MATT',
    b'ASPC', b'RELA', b'EQUP', b'OTFT', b'WATR', b'WTHR', b'CLMT',
    b'REGN', b'NAVI', b'NAVM', b'LAND', b'REFR', b'ACHR', b'PGRE',
    b'PHZD', b'LCTN', b'ECZN', b'SMQN', b'SMBN', b'DLBR', b'DLVW',
    b'SCEN', b'ANIO', b'ARTO', b'MOVT', b'SNCT', b'SOPM', b'DUAL',
    b'MATO', b'GRAS', b'FLOR', b'MSTT', b'TACT', b'DEBR', b'IMGS',
    b'IMAD', b'DOBJ', b'LGTM', b'MUSC', b'MUST', b'VTYP', b'AACT',
    b'CLFM', b'REVB', b'RFCT', b'LENS', b'SPGD', b'WOOP', b'SHOU',
    b'SLGM', b'LSCR', b'MESG', b'ADDN', b'AVIF', b'CAMS', b'CPTH',
    b'APPA', b'KEYM', b'LTEX', b'CSTY', b'CMPO', b'DFOB', b'CCRD',
}

class Issue:
    def __init__(self, level, offset, msg):
        self.level = level  # 'ERROR' or 'WARN'
        self.offset = offset
        self.msg = msg
    def __str__(self):
        return f"[{self.level}] @0x{self.offset:06X}: {self.msg}"

def check_esp(path: Path):
    issues: list[Issue] = []
    data = path.read_bytes()
    size = len(data)
    pos = 0

    def warn(off, msg): issues.append(Issue('WARN', off, msg))
    def error(off, msg): issues.append(Issue('ERROR', off, msg))

    # --- TES4 header ---
    if data[0:4] != b'TES4':
        error(0, f"Bad file magic: {data[0:4]!r} (expected TES4)")
        return issues

    tes4_size = struct.unpack_from('<I', data, 4)[0]
    tes4_flags = struct.unpack_from('<I', data, 8)[0]
    tes4_ver = struct.unpack_from('<H', data, 20)[0]

    is_esl = bool(tes4_flags & 0x0200)
    is_localized = bool(tes4_flags & 0x0080)
    print(f"TES4: flags=0x{tes4_flags:04X} ESL={is_esl} Localized={is_localized} formVersion={tes4_ver}")

    if tes4_ver != 44:
        warn(20, f"TES4 form version {tes4_ver} (expected 44 for SSE)")

    # Parse HEDR
    hedr_off = 24  # after TES4 record header
    if data[hedr_off:hedr_off+4] == b'HEDR':
        hedr_size = struct.unpack_from('<H', data, hedr_off+4)[0]
        ver = struct.unpack_from('<f', data, hedr_off+6)[0]
        num_records = struct.unpack_from('<I', data, hedr_off+10)[0]
        next_obj = struct.unpack_from('<I', data, hedr_off+14)[0]
        print(f"HEDR: version={ver:.2f} numRecords={num_records} nextObjectId=0x{next_obj:04X}")

        if is_esl and next_obj > 0xFFF:
            error(hedr_off+14, f"ESL plugin nextObjectId 0x{next_obj:04X} exceeds 0xFFF limit")

    # Parse masters
    master_off = hedr_off + 6 + hedr_size
    masters = []
    while master_off < 24 + tes4_size:
        sub_type = data[master_off:master_off+4]
        sub_size = struct.unpack_from('<H', data, master_off+4)[0]
        if sub_type == b'MAST':
            name = data[master_off+6:master_off+6+sub_size].rstrip(b'\x00').decode('ascii', errors='replace')
            masters.append(name)
            print(f"  Master: {name}")
        master_off += 6 + sub_size

    # --- Iterate top-level groups ---
    pos = 24 + tes4_size
    record_counts = {}

    while pos < size:
        if pos + 4 > size:
            error(pos, "Truncated: not enough bytes for record type")
            break

        rec_type = data[pos:pos+4]

        if rec_type == b'GRUP':
            if pos + 24 > size:
                error(pos, "Truncated GRUP header")
                break
            grup_size = struct.unpack_from('<I', data, pos+4)[0]
            grup_label = data[pos+8:pos+12]
            print(f"\nGRUP {grup_label.decode('ascii', errors='replace')}: size={grup_size} @0x{pos:06X}")

            if grup_size < 24:
                error(pos, f"GRUP size {grup_size} < 24 (minimum header)")
                break

            # Parse records within group
            inner_pos = pos + 24
            grup_end = pos + grup_size

            while inner_pos < grup_end:
                if inner_pos + 24 > size:
                    error(inner_pos, "Truncated record header inside GRUP")
                    break

                r_type = data[inner_pos:inner_pos+4]
                r_size = struct.unpack_from('<I', data, inner_pos+4)[0]
                r_flags = struct.unpack_from('<I', data, inner_pos+8)[0]
                r_formid = struct.unpack_from('<I', data, inner_pos+12)[0]
                r_ver = struct.unpack_from('<H', data, inner_pos+20)[0]

                r_type_str = r_type.decode('ascii', errors='replace')
                record_counts[r_type_str] = record_counts.get(r_type_str, 0) + 1

                if r_type not in KNOWN_RECORD_TYPES:
                    warn(inner_pos, f"Unknown record type: {r_type!r}")

                if r_ver != 44:
                    warn(inner_pos, f"{r_type_str} [0x{r_formid:08X}] form version {r_ver} (expected 44)")

                rec_end = inner_pos + 24 + r_size
                if rec_end > grup_end:
                    error(inner_pos, f"{r_type_str} [0x{r_formid:08X}] size {r_size} overflows GRUP boundary")
                    break

                # Parse subrecords
                check_subrecords(data, inner_pos + 24, r_size, r_type_str, r_formid, is_esl, issues)

                inner_pos = rec_end

            pos = grup_end
        else:
            error(pos, f"Expected GRUP, found {rec_type!r}")
            break

    print(f"\n--- Record counts ---")
    for k, v in sorted(record_counts.items()):
        print(f"  {k}: {v}")

    return issues


def check_subrecords(data, start, rec_size, rec_type, formid, is_esl, issues):
    """Parse subrecords within a record and check for common issues."""
    pos = start
    end = start + rec_size
    sub_list = []
    has_edid = False

    while pos < end:
        if pos + 6 > end:
            issues.append(Issue('ERROR', pos,
                f"{rec_type} [0x{formid:08X}] truncated subrecord header ({end - pos} bytes left, need 6)"))
            break

        sub_type = data[pos:pos+4]
        sub_size = struct.unpack_from('<H', data, pos+4)[0]
        sub_type_str = sub_type.decode('ascii', errors='replace')
        sub_list.append(sub_type_str)

        if pos + 6 + sub_size > end:
            issues.append(Issue('ERROR', pos,
                f"{rec_type} [0x{formid:08X}] subrecord {sub_type_str} size {sub_size} overflows record "
                f"(record end=0x{end:06X}, sub end=0x{pos+6+sub_size:06X})"))
            break

        if sub_type == b'EDID':
            has_edid = True
            edid = data[pos+6:pos+6+sub_size].rstrip(b'\x00').decode('ascii', errors='replace')

        # MGEF DATA must be exactly 152 bytes for SSE
        if rec_type == 'MGEF' and sub_type == b'DATA':
            if sub_size != 152:
                issues.append(Issue('ERROR', pos,
                    f"MGEF [0x{formid:08X}] DATA size {sub_size} (expected 152 for SSE)"))

        # VMAD basic validation
        if sub_type == b'VMAD':
            check_vmad(data, pos+6, sub_size, rec_type, formid, issues)

        # ESL FormID range check for references
        if is_esl and sub_size == 4 and sub_type in (b'ALFR', b'EFIT'):
            ref_id = struct.unpack_from('<I', data, pos+6)[0]
            # Only check self-references (high byte matches plugin index)
            idx = (ref_id >> 24)
            local = ref_id & 0x00FFFFFF
            if idx > 0 and local > 0xFFF:
                issues.append(Issue('WARN', pos,
                    f"{rec_type} [0x{formid:08X}] {sub_type_str} ref 0x{ref_id:08X} local ID > 0xFFF (ESL limit)"))

        pos += 6 + sub_size

    if rec_type == 'QUST':
        alias_headers = ('ALST', 'ALLS')
        alias_indices = [i for i, sig in enumerate(sub_list) if sig in alias_headers]
        if alias_indices:
            first_alias_idx = min(alias_indices)
            anam_indices = [i for i, sig in enumerate(sub_list) if sig == 'ANAM']
            if not anam_indices:
                issues.append(Issue(
                    'ERROR',
                    start,
                    f"QUST [0x{formid:08X}] has alias subrecords but is missing ANAM (NextAliasID). "
                    "This can trigger Wrye Bash KeyError(FNAM) while loading aliases."))
            elif min(anam_indices) > first_alias_idx:
                issues.append(Issue(
                    'ERROR',
                    start,
                    f"QUST [0x{formid:08X}] has ANAM after alias data. "
                    "Expected ANAM before ALST/ALLS for parser compatibility."))

    if not has_edid and rec_type not in ('TES4',):
        issues.append(Issue('WARN', start,
            f"{rec_type} [0x{formid:08X}] missing EDID subrecord"))


def check_vmad(data, start, vmad_size, rec_type, formid, issues):
    """Basic VMAD structure validation."""
    if vmad_size < 4:
        issues.append(Issue('ERROR', start,
            f"{rec_type} [0x{formid:08X}] VMAD too small ({vmad_size} bytes)"))
        return

    ver = struct.unpack_from('<H', data, start)[0]
    obj_fmt = struct.unpack_from('<H', data, start+2)[0]

    if ver > 5:
        issues.append(Issue('WARN', start,
            f"{rec_type} [0x{formid:08X}] VMAD version {ver} (expected <=5)"))

    if obj_fmt not in (1, 2):
        issues.append(Issue('ERROR', start,
            f"{rec_type} [0x{formid:08X}] VMAD objectFormat {obj_fmt} (expected 1 or 2)"))

    if vmad_size < 6:
        return

    script_count = struct.unpack_from('<H', data, start+4)[0]

    # Parse scripts to check string lengths
    pos = start + 6
    end = start + vmad_size
    for i in range(script_count):
        if pos + 2 > end:
            issues.append(Issue('ERROR', pos,
                f"{rec_type} [0x{formid:08X}] VMAD script {i}: truncated name length"))
            return
        name_len = struct.unpack_from('<H', data, pos)[0]
        pos += 2
        if pos + name_len > end:
            issues.append(Issue('ERROR', pos,
                f"{rec_type} [0x{formid:08X}] VMAD script {i}: name length {name_len} overflows"))
            return
        name = data[pos:pos+name_len].decode('ascii', errors='replace')
        pos += name_len

        if pos + 1 > end:
            return
        flags = data[pos]
        pos += 1

        # Property count
        if pos + 2 > end:
            return
        prop_count = struct.unpack_from('<H', data, pos)[0]
        pos += 2

        # Skip properties (complex, just check bounds)
        # Each property has: name(2+len), type(1), flags(1), data(varies)
        for j in range(prop_count):
            if pos + 2 > end:
                issues.append(Issue('ERROR', pos,
                    f"{rec_type} [0x{formid:08X}] VMAD script '{name}' property {j}: truncated"))
                return
            pname_len = struct.unpack_from('<H', data, pos)[0]
            pos += 2 + pname_len
            if pos + 2 > end:
                return
            ptype = data[pos] if pos < end else 0
            pos += 1  # type
            pflags = data[pos] if pos < end else 0
            pos += 1  # flags

            # Skip property data based on type (simplified)
            if ptype == 1:    pos += 4   # Object (objFormat 2: 4 bytes? or 8?)
            elif ptype == 2:  pos += 4   # String (2+len, but simplified)
            elif ptype == 3:  pos += 4   # Int32
            elif ptype == 4:  pos += 4   # Float
            elif ptype == 5:  pos += 1   # Bool
            else:
                # Unknown type, can't skip safely
                break

    # For QUST: check fragment + alias section
    if rec_type == 'QUST' and pos < end:
        remaining = end - pos
        print(f"  QUST VMAD: {script_count} script(s), {remaining} bytes remaining for fragments/aliases")


def main():
    if len(sys.argv) < 2:
        esp = Path(__file__).parent.parent / "Data" / "CalamityAffixes.esp"
    else:
        esp = Path(sys.argv[1])

    if not esp.exists():
        print(f"File not found: {esp}")
        sys.exit(1)

    print(f"Checking: {esp} ({esp.stat().st_size} bytes)\n")
    issues = check_esp(esp)

    print(f"\n{'='*60}")
    if not issues:
        print("No issues found.")
    else:
        errors = [i for i in issues if i.level == 'ERROR']
        warns = [i for i in issues if i.level == 'WARN']
        for i in issues:
            print(i)
        print(f"\n{len(errors)} error(s), {len(warns)} warning(s)")

    return 1 if any(i.level == 'ERROR' for i in issues) else 0

if __name__ == '__main__':
    sys.exit(main())
