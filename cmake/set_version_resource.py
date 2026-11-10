#!/usr/bin/env python3
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2026, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Usage: set_version_resource.py <input.exe> <version.res> <output.exe>
# Copies <input.exe> to <output.exe> and writes the version resource from <version.res> into the copy.
import ctypes
import os
import shutil
import struct
import sys

RT_VERSION = 16


def read_resource_id(data, pos):
    if struct.unpack_from("<H", data, pos)[0] == 0xFFFF:
        return struct.unpack_from("<H", data, pos + 2)[0], pos + 4
    end = pos
    while struct.unpack_from("<H", data, end)[0] != 0:
        end += 2
    return data[pos:end].decode("utf-16-le"), end + 2


def version_resources(res_path):
    with open(res_path, "rb") as f:
        data = f.read()
    resources = []
    pos = 0
    while pos + 8 <= len(data):
        data_size, header_size = struct.unpack_from("<II", data, pos)
        res_type, p = read_resource_id(data, pos + 8)
        name, p = read_resource_id(data, p)
        p = (p + 3) & ~3
        language = struct.unpack_from("<H", data, p + 6)[0]
        if res_type == RT_VERSION:
            resources.append((name, language, data[pos + header_size:pos + header_size + data_size]))
        pos = (pos + header_size + data_size + 3) & ~3
    return resources


def symbol_table_layout(exe_path):
    """Returns (file offset of PointerToSymbolTable, its value, file offset of the .symtab section or None)."""
    with open(exe_path, "rb") as f:
        data = f.read(4096)
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise RuntimeError("{} is not a PE file".format(exe_path))
    section_count = struct.unpack_from("<H", data, pe + 6)[0]
    pointer = struct.unpack_from("<I", data, pe + 12)[0]
    optional_header_size = struct.unpack_from("<H", data, pe + 20)[0]
    sections = pe + 24 + optional_header_size
    if sections + 40 * section_count > len(data):
        raise RuntimeError("{}: section table extends past the first {} bytes".format(exe_path, len(data)))
    symtab = None
    for i in range(section_count):
        name, _, _, _, raw_offset = struct.unpack_from("<8sIIII", data, sections + 40 * i)
        if name.rstrip(b"\0") == b".symtab":
            symtab = raw_offset
    return pe + 12, pointer, symtab


def write_version_resources(exe_path, resources):
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel32.BeginUpdateResourceW.restype = ctypes.c_void_p
    kernel32.BeginUpdateResourceW.argtypes = [ctypes.c_wchar_p, ctypes.c_int]
    kernel32.UpdateResourceW.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_ushort,
                                         ctypes.c_void_p, ctypes.c_uint]
    kernel32.EndUpdateResourceW.argtypes = [ctypes.c_void_p, ctypes.c_int]

    handle = kernel32.BeginUpdateResourceW(exe_path, False)
    if not handle:
        raise ctypes.WinError(ctypes.get_last_error())
    for name, language, data in resources:
        name_arg = name if isinstance(name, int) else ctypes.c_wchar_p(name)
        buffer = ctypes.create_string_buffer(data, len(data))
        if not kernel32.UpdateResourceW(handle, RT_VERSION, name_arg, language, buffer, len(data)):
            error = ctypes.get_last_error()
            kernel32.EndUpdateResourceW(handle, True)
            raise ctypes.WinError(error)
    if not kernel32.EndUpdateResourceW(handle, False):
        raise ctypes.WinError(ctypes.get_last_error())


def main():
    if len(sys.argv) != 4:
        sys.exit("Usage: set_version_resource.py <input.exe> <version.res> <output.exe>")
    input_exe, res_path, output_exe = sys.argv[1:]

    resources = version_resources(res_path)
    if not resources:
        sys.exit("No version resource in {}".format(res_path))

    # Go's linker always places its (empty) COFF symbol table at the start of the .symtab section. A different
    # layout means a linker change this script has not been checked against.
    _, pointer, symtab = symbol_table_layout(input_exe)
    if pointer != 0 and pointer != symtab:
        sys.exit("{}: symbol table pointer 0x{:x} does not match the .symtab section".format(input_exe, pointer))

    shutil.copyfile(input_exe, output_exe)
    try:
        write_version_resources(output_exe, resources)

        # UpdateResource moves .symtab to make room for the resources but does not update the pointer to it.
        pointer_offset, pointer, symtab = symbol_table_layout(output_exe)
        if pointer != 0:
            if symtab is None:
                raise RuntimeError("{}: .symtab section missing after writing resources".format(output_exe))
            if pointer != symtab:
                with open(output_exe, "r+b") as f:
                    f.seek(pointer_offset)
                    f.write(struct.pack("<I", symtab))
    except BaseException:
        os.remove(output_exe)
        raise


if __name__ == "__main__":
    main()
