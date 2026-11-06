#!/usr/bin/env python3
"""
MaiaRTOS Native Module Packer (mnm-pack)
Validates ELF32 Little-Endian RISC-V relocatable modules (ET_REL)
and generates companion MNM-1 install manifests (*.mnm.json).
Specification: MaiaRTOS Native Execution Architecture (Section 5.2).
"""

import sys
import os
import struct
import hashlib
import json
import argparse

ELF_MAGIC = b"\x7fELF"
ELFCLASS32 = 1
ELFDATA2LSB = 1
EM_RISCV = 243
ET_REL = 1

def validate_and_hash_elf(filepath):
    with open(filepath, "rb") as f:
        data = f.read()

    if len(data) < 52:
        raise ValueError(f"File too small to be a valid ELF32: {len(data)} bytes")

    if data[:4] != ELF_MAGIC:
        raise ValueError(f"Invalid ELF magic: {data[:4]!r}")

    ei_class = data[4]
    ei_data = data[5]

    if ei_class != ELFCLASS32:
        raise ValueError(f"Expected ELFCLASS32 (1), got {ei_class}")
    if ei_data != ELFDATA2LSB:
        raise ValueError(f"Expected ELFDATA2LSB (1), got {ei_data}")

    e_type, e_machine, e_version, e_entry, e_phoff, e_shoff, e_flags, e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx = struct.unpack_from(
        "<HHIIIIIHHHHHH", data, 16
    )

    if e_type != ET_REL:
        raise ValueError(f"Expected ET_REL (1), got {e_type}")
    if e_machine != EM_RISCV:
        raise ValueError(f"Expected EM_RISCV (243/0xF3), got {e_machine}")

    # Inspect sections to locate string table and symbol table
    if e_shoff == 0 or e_shnum == 0:
        raise ValueError("No section header table found")

    sh_table = []
    for i in range(e_shnum):
        offset = e_shoff + i * e_shentsize
        sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size, sh_link, sh_info, sh_addralign, sh_entsize = struct.unpack_from(
            "<IIIIIIIIII", data, offset
        )
        sh_table.append({
            "idx": i,
            "type": sh_type,
            "flags": sh_flags,
            "offset": sh_offset,
            "size": sh_size,
            "link": sh_link,
            "info": sh_info,
        })

    # Find SHT_SYMTAB
    symtab_sh = next((s for s in sh_table if s["type"] == 2), None)
    if not symtab_sh:
        raise ValueError("No SHT_SYMTAB found in ELF object")

    strtab_sh = sh_table[symtab_sh["link"]]
    strtab_bytes = data[strtab_sh["offset"] : strtab_sh["offset"] + strtab_sh["size"]]

    # Locate maia_module_main entry point
    sym_count = symtab_sh["size"] // 16
    has_entry = False

    for s_idx in range(sym_count):
        s_off = symtab_sh["offset"] + s_idx * 16
        st_name, st_value, st_size, st_info, st_other, st_shndx = struct.unpack_from(
            "<IIIBBH", data, s_off
        )
        if st_name < len(strtab_bytes):
            name_end = strtab_bytes.find(b"\0", st_name)
            name = strtab_bytes[st_name:name_end].decode("utf-8", errors="replace")
            if name == "maia_module_main":
                has_entry = True
                break

    if not has_entry:
        raise ValueError("Mandatory symbol 'maia_module_main' not found in symbol table")

    sha256_hex = hashlib.sha256(data).hexdigest().lower()
    return data, sha256_hex

def pack_module(input_path, output_mobj=None, manifest_path=None, name=None, version="0.1.0"):
    data, sha256_hex = validate_and_hash_elf(input_path)

    base_name = name or os.path.splitext(os.path.basename(input_path))[0]
    if base_name.endswith(".mobj") or base_name.endswith("_norelax"):
        base_name = base_name.split(".")[0].replace("_norelax", "")

    if output_mobj:
        with open(output_mobj, "wb") as f:
            f.write(data)
        out_file = output_mobj
    else:
        out_file = input_path

    manifest = {
        "schema": "maia.native.module/1",
        "name": base_name,
        "version": version,
        "target": "maia-rv32-esp32p4-v1",
        "abi_major": 1,
        "abi_minor_min": 0,
        "entry": "maia_module_main",
        "elf_sha256": sha256_hex,
        "memory": {
            "stack_bytes": 8192,
            "heap_limit_bytes": 65536
        },
        "permissions": ["stdio"],
        "imports": ["maia_host_api_v1"]
    }

    if not manifest_path:
        manifest_path = os.path.splitext(out_file)[0] + ".mnm.json"

    with open(manifest_path, "w") as f:
        json.dump(manifest, f, indent=2)

    print(f"[mnm-pack] Module: {base_name} ({len(data)} bytes)")
    print(f"[mnm-pack] SHA-256: {sha256_hex}")
    print(f"[mnm-pack] Manifest: {manifest_path}")
    return manifest

def main():
    parser = argparse.ArgumentParser(description="Maia Native Module Packer")
    parser.add_argument("input", help="Input ELF relocatable object (.o or .mobj)")
    parser.add_argument("-o", "--output", help="Output .mobj path (optional)")
    parser.add_argument("-m", "--manifest", help="Output .mnm.json path (optional)")
    parser.add_argument("-n", "--name", help="Module name (optional)")
    parser.add_argument("-v", "--version", default="0.1.0", help="Module version (default: 0.1.0)")

    args = parser.parse_args()
    try:
        pack_module(args.input, args.output, args.manifest, args.name, args.version)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
