"""Encode JSON parameter definitions into raw bytes for CCSDS payloads.

Supported types
---------------
uint8 / int8
uint16 / int16
uint32 / int32
uint64 / int64
float  (IEEE 754, 4 bytes)
double (IEEE 754, 8 bytes)
string - fixed-length, null-padded  (requires "length" in param def)
bytes  - raw hex input              (requires "length" in param def)
enum   - dropdown; stored as uint16 by default (override with "storage")
         requires "values": {"NAME": int_value, ...} in param def

Arrays
------
Add "count": N to any scalar type to get N consecutive fields.
In the GUI each array shows as a single comma-separated text box.
"""

import struct
from typing import Any, Dict, Tuple

# (struct format char, byte size)
SCALAR_TYPES: Dict[str, Tuple[str, int]] = {
    'uint8':  ('B', 1),
    'int8':   ('b', 1),
    'uint16': ('H', 2),
    'int16':  ('h', 2),
    'uint32': ('I', 4),
    'int32':  ('i', 4),
    'uint64': ('Q', 8),
    'int64':  ('q', 8),
    'float':  ('f', 4),
    'double': ('d', 8),
}

ALL_TYPES = list(SCALAR_TYPES.keys()) + ['string', 'bytes', 'enum']


def _parse_int(s: str) -> int:
    s = s.strip()
    if s.startswith(('0x', '0X')):
        return int(s, 16)
    if s.startswith(('0b', '0B')):
        return int(s, 2)
    return int(s)


def _parse_scalar(value_str: str, ptype: str) -> Any:
    value_str = value_str.strip() or '0'
    if ptype in ('float', 'double'):
        return float(value_str)
    return _parse_int(value_str)


def encode_param(value_str: str, param: dict, endian: str = '<') -> bytes:
    """Encode one parameter value string into bytes.

    Args:
        value_str: Text as entered by the user.
        param:     Parameter definition dict from the JSON file.
        endian:    '<' little-endian (default, x86 cFS targets)
                   '>' big-endian

    Returns:
        Encoded bytes for this single parameter (no array handling here).

    Raises:
        ValueError: On unrecognised type or parse failure.
    """
    ptype = param['type']

    if ptype in SCALAR_TYPES:
        fmt_char, _ = SCALAR_TYPES[ptype]
        val = _parse_scalar(value_str, ptype)
        return struct.pack(endian + fmt_char, val)

    if ptype == 'string':
        length = int(param.get('length', 32))
        encoded = value_str.encode('ascii', errors='replace')
        return encoded[:length].ljust(length, b'\x00')

    if ptype == 'bytes':
        length = int(param.get('length', 0))
        hex_str = value_str.replace(' ', '').replace('0x', '').replace('0X', '')
        try:
            data = bytes.fromhex(hex_str)
        except ValueError:
            data = b''
        if length:
            data = (data + b'\x00' * length)[:length]
        return data

    if ptype == 'enum':
        values: dict = param.get('values', {})
        storage = param.get('storage', 'uint16')
        fmt_char, _ = SCALAR_TYPES[storage]
        if value_str in values:
            int_val = int(values[value_str])
        else:
            try:
                int_val = _parse_int(value_str)
            except (ValueError, TypeError):
                int_val = 0
        return struct.pack(endian + fmt_char, int_val)

    raise ValueError(f"Unknown parameter type: {ptype!r}")


def encode_param_array(value_str: str, param: dict, endian: str = '<') -> bytes:
    """Encode a possibly-array parameter from a comma-separated string."""
    count = int(param.get('count', 1))
    if count <= 1:
        return encode_param(value_str, param, endian)

    parts = [p.strip() for p in value_str.split(',')]
    result = b''
    for i in range(count):
        part = parts[i] if i < len(parts) else '0'
        result += encode_param(part, param, endian)
    return result


def param_byte_size(param: dict) -> int:
    """Return total byte size of a parameter (accounting for count)."""
    ptype = param['type']
    count = int(param.get('count', 1))

    if ptype in SCALAR_TYPES:
        _, size = SCALAR_TYPES[ptype]
    elif ptype in ('string', 'bytes'):
        size = int(param.get('length', 0))
    elif ptype == 'enum':
        storage = param.get('storage', 'uint16')
        _, size = SCALAR_TYPES[storage]
    else:
        size = 0

    return size * count
