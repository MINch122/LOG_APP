"""CCSDS packet building and parsing for cFS.

Primary header (6 bytes, big-endian per CCSDS standard):
  [15:13] Version = 000
  [12]    Type    = 1 (command) / 0 (telemetry)
  [11]    Secondary header flag = 1
  [10:0]  APID
  [31:30] Sequence flags = 11 (standalone)
  [29:16] Sequence count (14 bits)
  [47:32] Packet data length = total_bytes - 7

Command secondary header (2 bytes):
  [7:0]  Function code
  [15:8] Checksum (XOR of all bytes from sec-hdr onward must equal 0xFF)
"""

import struct
from dataclasses import dataclass
from typing import Optional


@dataclass
class PrimaryHeader:
    mid: int
    is_command: bool
    has_secondary: bool
    apid: int
    seq_flags: int
    seq_count: int
    data_length: int   # bytes after primary header minus 1


def parse_primary_header(data: bytes) -> Optional[PrimaryHeader]:
    if len(data) < 6:
        return None
    stream_id, seq_word, data_len = struct.unpack('>HHH', data[:6])
    return PrimaryHeader(
        mid=stream_id,
        is_command=bool((stream_id >> 12) & 1),
        has_secondary=bool((stream_id >> 11) & 1),
        apid=stream_id & 0x7FF,
        seq_flags=(seq_word >> 14) & 0x3,
        seq_count=seq_word & 0x3FFF,
        data_length=data_len,
    )


def _compute_checksum(fc: int, payload: bytes) -> int:
    """Return the checksum byte such that XOR of [fc, checksum, *payload] == 0xFF."""
    cs = 0xFF ^ fc
    for b in payload:
        cs ^= b
    return cs


def build_command_packet(mid: int, fc: int, payload: bytes = b'', seq_count: int = 0) -> bytes:
    """Build a complete CCSDS command packet ready to send to ci_lab.

    Args:
        mid:       Message ID (e.g. 0x1806 for CFE_ES). Must already have
                   the command-type bit set (bit 12).
        fc:        Function code (0-127).
        payload:   Encoded parameter bytes (little-endian for x86 targets).
        seq_count: Rolling sequence counter (0-16383).

    Returns:
        Complete packet as bytes.
    """
    seq_word = 0xC000 | (seq_count & 0x3FFF)          # flags=11, count
    pkt_data_len = 1 + len(payload)                    # sec-hdr(2B) - 1 + payload
    primary = struct.pack('>HHH', mid, seq_word, pkt_data_len)
    checksum = _compute_checksum(fc, payload)
    secondary = bytes([fc, checksum])
    return primary + secondary + payload
