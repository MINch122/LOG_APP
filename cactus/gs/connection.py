# -*- coding: utf-8 -*-
"""UDP connection manager for cFS ci_lab / to_lab communication.

Commands are sent via UDP to ci_lab (default port 1234).
Telemetry is received from to_lab on a separate UDP port (default 5011).
Reception runs in a daemon thread; incoming packets are delivered
through the on_packet_received callback (called from that thread —
callers must use Qt signals or similar to safely update the UI).
"""

import socket
import threading
from typing import Callable, Optional


class UdpConnection:
    def __init__(self) -> None:
        self.target_ip: str = '127.0.0.1'
        self.cmd_port: int = 1234
        self.tlm_port: int = 5011
        self.seq_count: int = 0

        self._send_sock: Optional[socket.socket] = None
        self._recv_sock: Optional[socket.socket] = None
        self._recv_thread: Optional[threading.Thread] = None
        self._running: bool = False

        # Set this before calling connect(); will be called from the recv thread.
        self.on_packet_received: Optional[Callable[[bytes], None]] = None

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def connect(self, target_ip: str, cmd_port: int, tlm_port: int) -> None:
        """Open sockets and start the telemetry receiver thread."""
        self.disconnect()

        self.target_ip = target_ip
        self.cmd_port = cmd_port
        self.tlm_port = tlm_port

        self._send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        self._recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._recv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._recv_sock.bind(('', tlm_port))
        self._recv_sock.settimeout(1.0)

        self._running = True
        self._recv_thread = threading.Thread(
            target=self._receive_loop, name='tlm-recv', daemon=True
        )
        self._recv_thread.start()

    def disconnect(self) -> None:
        """Close all sockets and stop the receiver thread."""
        self._running = False
        if self._recv_sock:
            try:
                self._recv_sock.close()
            except OSError:
                pass
            self._recv_sock = None
        if self._send_sock:
            try:
                self._send_sock.close()
            except OSError:
                pass
            self._send_sock = None

    def send(self, data: bytes) -> bool:
        """Send raw bytes to ci_lab.  Returns True on success."""
        if not self._send_sock:
            return False
        try:
            self._send_sock.sendto(data, (self.target_ip, self.cmd_port))
            self.seq_count = (self.seq_count + 1) & 0x3FFF
            return True
        except OSError:
            return False

    @property
    def is_connected(self) -> bool:
        return self._send_sock is not None

    # ------------------------------------------------------------------
    # Internal
    # ------------------------------------------------------------------

    def _receive_loop(self) -> None:
        while self._running:
            try:
                data, _ = self._recv_sock.recvfrom(65535)
                if data and self.on_packet_received:
                    self.on_packet_received(data)
            except socket.timeout:
                continue
            except OSError:
                break
