#!/usr/bin/env python3
"""Embed the pinned CA PEM as bytes for the PS5 payload."""
from pathlib import Path
import sys

source, destination = map(Path, sys.argv[1:3])
data = source.read_bytes() + b"\0"
with destination.open("w", encoding="ascii") as out:
    out.write("#include <stddef.h>\nconst char g_cacert_pem[] = {\n")
    for start in range(0, len(data), 16):
        out.write("  " + ", ".join(f"0x{byte:02x}" for byte in data[start:start + 16]) + ",\n")
    out.write("};\nconst size_t g_cacert_pem_size = sizeof(g_cacert_pem);\n")
