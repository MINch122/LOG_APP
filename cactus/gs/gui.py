# -*- coding: utf-8 -*-
"""Main GUI for CACTUS.

Layout
------
  Toolbar: IP  CmdPort  TlmPort  [Connect]  status  Endian
  Left panel: Search + command tree grouped by app
  Right panel: Command tab (form + send) | Telemetry tab
  Bottom: Log panel
"""

import datetime
import json
import shutil
import threading
from pathlib import Path
from typing import List, Optional, Tuple

from PyQt5.QtCore import QEvent, QObject, QPropertyAnimation, Qt, QTimer, pyqtSignal
from PyQt5.QtGui import QColor, QDoubleValidator, QFont, QKeySequence, QPixmap
from PyQt5.QtWidgets import (
    QAbstractItemView, QAction, QApplication, QComboBox, QDialog, QDialogButtonBox,
    QFormLayout, QFrame, QGraphicsOpacityEffect, QHBoxLayout, QHeaderView, QInputDialog,
    QLabel, QLineEdit, QMainWindow, QMessageBox, QPushButton, QScrollArea,
    QShortcut, QSizePolicy, QSpinBox, QSplitter, QTabWidget, QTableWidget,
    QTableWidgetItem, QTextEdit, QToolBar, QTreeWidget, QTreeWidgetItem,
    QVBoxLayout, QWidget,
)

from .ccsds import build_command_packet, parse_primary_header


# ---------------------------------------------------------------------------
# Theme helpers
# ---------------------------------------------------------------------------

def make_palette(dark: bool) -> 'QPalette':
    """Return a QPalette for dark or light mode."""
    from PyQt5.QtGui import QPalette  # already imported at call site; safe to re-import
    pal = QPalette()
    if dark:
        pal.setColor(QPalette.Window,          QColor(45,  45,  45))
        pal.setColor(QPalette.WindowText,      QColor(220, 220, 220))
        pal.setColor(QPalette.Base,            QColor(30,  30,  30))
        pal.setColor(QPalette.AlternateBase,   QColor(45,  45,  45))
        pal.setColor(QPalette.ToolTipBase,     QColor(30,  30,  30))
        pal.setColor(QPalette.ToolTipText,     QColor(220, 220, 220))
        pal.setColor(QPalette.Text,            QColor(220, 220, 220))
        pal.setColor(QPalette.Button,          QColor(55,  55,  55))
        pal.setColor(QPalette.ButtonText,      QColor(220, 220, 220))
        pal.setColor(QPalette.BrightText,      Qt.red)
        pal.setColor(QPalette.Highlight,       QColor(70,  130, 180))
        pal.setColor(QPalette.HighlightedText, Qt.white)
        pal.setColor(QPalette.Disabled, QPalette.Text,       QColor(120, 120, 120))
        pal.setColor(QPalette.Disabled, QPalette.ButtonText, QColor(120, 120, 120))
        pal.setColor(QPalette.Link,            QColor(100, 170, 230))
    else:
        pal.setColor(QPalette.Window,          QColor(240, 240, 240))
        pal.setColor(QPalette.WindowText,      QColor(30,  30,  30))
        pal.setColor(QPalette.Base,            QColor(255, 255, 255))
        pal.setColor(QPalette.AlternateBase,   QColor(233, 233, 233))
        pal.setColor(QPalette.ToolTipBase,     QColor(255, 255, 220))
        pal.setColor(QPalette.ToolTipText,     QColor(30,  30,  30))
        pal.setColor(QPalette.Text,            QColor(30,  30,  30))
        pal.setColor(QPalette.Button,          QColor(220, 220, 220))
        pal.setColor(QPalette.ButtonText,      QColor(30,  30,  30))
        pal.setColor(QPalette.BrightText,      Qt.red)
        pal.setColor(QPalette.Highlight,       QColor(70,  130, 180))
        pal.setColor(QPalette.HighlightedText, Qt.white)
        pal.setColor(QPalette.Disabled, QPalette.Text,       QColor(160, 160, 160))
        pal.setColor(QPalette.Disabled, QPalette.ButtonText, QColor(160, 160, 160))
        pal.setColor(QPalette.Link,            QColor(0,   100, 200))
    return pal


# ---------------------------------------------------------------------------
# Cat animation overlay (dark → light mode transition)
# ---------------------------------------------------------------------------

_LIGHTMODECAT_DIR = Path(__file__).parent / 'lightmodecat'


class CatAnimationOverlay(QWidget):
    """Full-window overlay that plays the light-mode cat frames with fade-in/out.

    The overlay background is transparent so the underlying UI remains visible.
    Only the image label fades, capped at ALPHA opacity.
    ``last_frame`` is emitted when the final frame begins its hold period so
    callers can apply a theme change underneath before the overlay fades out.
    """

    finished   = pyqtSignal()
    last_frame = pyqtSignal()

    FADE_MS = 50   # fade-in / fade-out duration (ms)
    HOLD_MS = 100   # time each frame is fully visible (ms)
    ALPHA   = 0.5

    def __init__(self, parent: QWidget, image_paths: List[str]) -> None:
        super().__init__(parent)
        self._paths = image_paths
        self._idx   = 0
        self._phase = 'fade_in'

        # Cover the entire parent window; transparent so underlying UI shows
        self.setGeometry(0, 0, parent.width(), parent.height())
        self.raise_()

        # Image label fills the overlay; pixmap is centered inside it
        self._lbl = QLabel(self)
        self._lbl.setAlignment(Qt.AlignCenter)
        self._lbl.setGeometry(self.rect())
        self._lbl.setStyleSheet('background: transparent;')

        # Opacity effect is on the label only — background stays invisible
        self._effect = QGraphicsOpacityEffect(self._lbl)
        self._effect.setOpacity(0.0)
        self._lbl.setGraphicsEffect(self._effect)

        # Property animation drives opacity
        self._anim = QPropertyAnimation(self._effect, b'opacity', self)
        self._anim.finished.connect(self._on_anim_done)

        # Hold timer between frames
        self._timer = QTimer(self)
        self._timer.setSingleShot(True)
        self._timer.timeout.connect(self._on_hold_done)

        # Track parent resizes so we stay full-window
        parent.installEventFilter(self)

        self._load_frame()
        self.show()
        self._start_fade(0.0, self.ALPHA)

    # ------------------------------------------------------------------

    def paintEvent(self, event) -> None:
        pass  # fully transparent background — don't paint anything

    def _load_frame(self) -> None:
        px = QPixmap(self._paths[self._idx])
        if px.isNull():
            return
        side = int(0.5 * max(self.width() or 1, self.height() or 1))
        px = px.scaled(side, side, Qt.KeepAspectRatio, Qt.SmoothTransformation)
        self._lbl.setPixmap(px)

    def _start_fade(self, start: float, end: float) -> None:
        self._anim.stop()
        self._anim.setDuration(self.FADE_MS)
        self._anim.setStartValue(start)
        self._anim.setEndValue(end)
        self._anim.start()

    def _on_anim_done(self) -> None:
        if self._phase == 'fade_in':
            self._phase = 'hold'
            # Single-image case: this frame is already the last one
            if self._idx == len(self._paths) - 1:
                self.last_frame.emit()
            self._timer.start(self.HOLD_MS)
        elif self._phase == 'fade_out':
            self._do_cleanup()

    def _on_hold_done(self) -> None:
        self._idx += 1
        if self._idx < len(self._paths):
            self._load_frame()
            # Emit before starting hold so theme switches as the frame appears
            if self._idx == len(self._paths) - 1:
                self.last_frame.emit()
            self._timer.start(self.HOLD_MS)
        else:
            self._phase = 'fade_out'
            self._start_fade(self.ALPHA, 0.0)

    def _do_cleanup(self) -> None:
        self._timer.stop()
        self._anim.stop()
        p = self.parent()
        if p is not None:
            p.removeEventFilter(self)
        self.hide()
        self.deleteLater()
        self.finished.emit()

    # ------------------------------------------------------------------

    def eventFilter(self, obj: QObject, event: QEvent) -> bool:
        if obj is self.parent() and event.type() == QEvent.Resize:
            self.setGeometry(0, 0, obj.width(), obj.height())
            self._lbl.setGeometry(self.rect())
            self._load_frame()
        return super().eventFilter(obj, event)


from .connection import UdpConnection
from .loader import AppDef, CommandDef, load_commands_dir, save_command
from .payload import ALL_TYPES, SCALAR_TYPES, encode_param_array, param_byte_size
from .scene import SceneWidget

# ---------------------------------------------------------------------------
# Paths & defaults
# ---------------------------------------------------------------------------

_HERE = Path(__file__).parent.parent
COMMANDS_DIR = str(_HERE / 'commands')
CONFIG_FILE = str(_HERE / 'config.json')

DEFAULT_CONFIG: dict = {
    'target_ip': '127.0.0.1',
    'cmd_port': 1234,
    'tlm_port': 5011,
    'endian': 'little',
    'commands_dir': COMMANDS_DIR,
}


# ---------------------------------------------------------------------------
# Qt signal bridge (telemetry arrives on a background thread)
# ---------------------------------------------------------------------------

class _TlmBridge(QObject):
    packet = pyqtSignal(bytes)


# ---------------------------------------------------------------------------
# ParamWidget — one row in the command form
# ---------------------------------------------------------------------------

class ParamWidget(QWidget):
    """Input widget + type badge for a single command parameter."""

    changed = pyqtSignal()

    def __init__(self, param: dict, endian: str = '<', parent: QWidget = None):
        super().__init__(parent)
        self.param = param
        self.endian = endian
        self._build()

    def _build(self) -> None:
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(6)

        ptype = self.param['type']
        count = int(self.param.get('count', 1))
        default = str(self.param.get('default', ''))

        badge = QLabel(self._badge_text())
        badge.setFixedWidth(80)
        badge.setAlignment(Qt.AlignLeft | Qt.AlignVCenter)
        badge.setStyleSheet('color:#777; font-size:10px;')
        layout.addWidget(badge)

        if ptype == 'enum':
            self.input: QWidget = QComboBox()
            for key in self.param.get('values', {}):
                self.input.addItem(str(key))
            if default and self.input.findText(default) >= 0:
                self.input.setCurrentText(default)
            self.input.currentTextChanged.connect(self.changed)
        else:
            self.input = QLineEdit()
            self.input.setText(default)
            self.input.textChanged.connect(self.changed)

            if ptype == 'string':
                length = int(self.param.get('length', 32))
                self.input.setMaxLength(length)
                self.input.setPlaceholderText(f'string ≤{length} chars')
            elif ptype == 'bytes':
                length = int(self.param.get('length', 0))
                self.input.setPlaceholderText(f'hex, e.g. AA BB CC  ({length} bytes)')
            elif ptype in ('float', 'double'):
                self.input.setValidator(QDoubleValidator())
                self.input.setPlaceholderText('0.0')
            else:
                placeholder = f'0  (or 0x…)'
                if count > 1:
                    placeholder = f'{count} values, comma-separated'
                self.input.setPlaceholderText(placeholder)

        layout.addWidget(self.input, 1)

    def _badge_text(self) -> str:
        ptype = self.param['type']
        count = int(self.param.get('count', 1))
        if ptype in SCALAR_TYPES:
            suffix = f'[{count}]' if count > 1 else ''
            return f'{ptype}{suffix}'
        if ptype == 'string':
            return f"str[{self.param.get('length','?')}]"
        if ptype == 'bytes':
            return f"bytes[{self.param.get('length','?')}]"
        if ptype == 'enum':
            storage = self.param.get('storage', 'uint16')
            return f'enum/{storage}'
        return ptype

    def get_value(self) -> str:
        if isinstance(self.input, QComboBox):
            return self.input.currentText()
        return self.input.text()

    def encode(self) -> bytes:
        return encode_param_array(self.get_value(), self.param, self.endian)

    def mark_error(self, err: bool) -> None:
        if err:
            self.input.setStyleSheet('border: 1px solid #e53935;')
        else:
            self.input.setStyleSheet('')


# ---------------------------------------------------------------------------
# CommandPanel — right-hand command tab
# ---------------------------------------------------------------------------

class CommandPanel(QWidget):
    log_message = pyqtSignal(str)   # forwarded to the main window log
    cmd_sent    = pyqtSignal()      # emitted after every successful uplink

    def __init__(self, conn: UdpConnection, parent: QWidget = None):
        super().__init__(parent)
        self.conn = conn
        self.current_cmd: Optional[CommandDef] = None
        self.param_widgets: List[ParamWidget] = []
        self._param_order: List[Tuple[dict, Optional['ParamWidget']]] = []
        self.endian: str = '<'
        self._last_hex: str = ''
        self._build()

    # ------------------------------------------------------------------
    # Layout
    # ------------------------------------------------------------------

    def _build(self) -> None:
        root = QVBoxLayout(self)
        root.setContentsMargins(8, 8, 8, 8)
        root.setSpacing(6)

        # ── Header ──────────────────────────────────────────────────
        self.title_lbl = QLabel('Select a command from the list')
        self.title_lbl.setStyleSheet('font-size:15px; font-weight:bold;')
        root.addWidget(self.title_lbl)

        self.desc_lbl = QLabel('')
        self.desc_lbl.setWordWrap(True)
        self.desc_lbl.setStyleSheet('color:#999;')
        root.addWidget(self.desc_lbl)

        meta = QHBoxLayout()
        meta.setSpacing(6)

        _field_style = self._field_style(dark=True)

        meta.addWidget(QLabel('MID:'))
        self.mid_edit = QLineEdit()
        self.mid_edit.setFixedWidth(80)
        self.mid_edit.setPlaceholderText('0x1806')
        self.mid_edit.setToolTip('Message ID (hex) — overrides the JSON default for this session')
        self.mid_edit.setStyleSheet(_field_style)
        self.mid_edit.textChanged.connect(self._schedule_preview_update)
        meta.addWidget(self.mid_edit)

        meta.addSpacing(10)
        meta.addWidget(QLabel('FC:'))
        self.fc_spin = QSpinBox()
        self.fc_spin.setRange(0, 127)
        self.fc_spin.setFixedWidth(58)
        self.fc_spin.setToolTip('Function code — overrides the JSON default for this session')
        self.fc_spin.setStyleSheet(_field_style)
        self.fc_spin.valueChanged.connect(self._schedule_preview_update)
        meta.addWidget(self.fc_spin)

        meta.addSpacing(10)
        self.size_lbl = QLabel()
        self.size_lbl.setStyleSheet('font-family:monospace; color:#7ec8e3;')
        meta.addWidget(self.size_lbl)

        meta.addStretch()

        self.reset_btn = QPushButton('Reset')
        self.reset_btn.setFixedWidth(48)
        self.reset_btn.setFixedHeight(22)
        self.reset_btn.setToolTip('Reset MID and FC to the values defined in the JSON file')
        self.reset_btn.setEnabled(False)
        self.reset_btn.setStyleSheet('''
            QPushButton {
                background:#4a3800; color:#ffcc02; border:none;
                border-radius:3px; font-size:10px;
            }
            QPushButton:hover   { background:#5c4600; }
            QPushButton:pressed { background:#332600; }
            QPushButton:disabled{ background:#2a2a2a; color:#555; }
        ''')
        self.reset_btn.clicked.connect(self._reset_mid_fc)
        meta.addWidget(self.reset_btn)

        root.addLayout(meta)

        sep = QFrame()
        sep.setFrameShape(QFrame.HLine)
        sep.setFrameShadow(QFrame.Sunken)
        root.addWidget(sep)

        # ── Scrollable parameter form ────────────────────────────────
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QFrame.NoFrame)

        self.form_container = QWidget()
        self.form_layout = QFormLayout(self.form_container)
        self.form_layout.setLabelAlignment(Qt.AlignRight)
        self.form_layout.setSpacing(8)
        self.form_layout.setContentsMargins(0, 4, 0, 4)
        scroll.setWidget(self.form_container)
        root.addWidget(scroll, 1)

        # ── Packet preview ───────────────────────────────────────────
        preview_row = QHBoxLayout()
        preview_row.setSpacing(4)

        self.preview_lbl = QLabel('Packet: —')
        self.preview_lbl.setFont(QFont('Courier', 9))
        self.preview_lbl.setStyleSheet(self._preview_style(dark=True))
        self.preview_lbl.setWordWrap(True)
        preview_row.addWidget(self.preview_lbl, 1)

        self.copy_btn = QPushButton('Copy')
        self.copy_btn.setFixedWidth(52)
        self.copy_btn.setFixedHeight(28)
        self.copy_btn.setToolTip('Copy hex bytes to clipboard (useful for TTC/SC sub-commands)')
        self.copy_btn.setEnabled(False)
        self.copy_btn.setStyleSheet(self._copy_btn_style(dark=True))
        self.copy_btn.clicked.connect(self._copy_hex)
        preview_row.addWidget(self.copy_btn, 0, Qt.AlignTop)

        root.addLayout(preview_row)

        # ── Send button ──────────────────────────────────────────────
        self.send_btn = QPushButton('Send Command    (Ctrl+Enter)')
        self.send_btn.setFixedHeight(42)
        self.send_btn.setEnabled(False)
        self.send_btn.setStyleSheet(self._send_btn_style(dark=True))
        self.send_btn.clicked.connect(self.send_command)
        root.addWidget(self.send_btn)

        # Ctrl+Enter shortcut
        shortcut = QShortcut(QKeySequence('Ctrl+Return'), self)
        shortcut.activated.connect(self.send_command)

    # ------------------------------------------------------------------
    # Theme helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _field_style(dark: bool) -> str:
        if dark:
            return ('font-family:monospace; color:#7ec8e3;'
                    'background:#1a1a1a; border:1px solid #444;'
                    'border-radius:3px; padding:1px 4px;')
        return ('font-family:monospace; color:#1a6e8a;'
                'background:#ffffff; border:1px solid #bbb;'
                'border-radius:3px; padding:1px 4px;')

    @staticmethod
    def _preview_style(dark: bool) -> str:
        if dark:
            return 'color:#aaa; background:#1a1a1a; padding:4px; border-radius:3px;'
        return 'color:#444; background:#f0f0f0; padding:4px; border-radius:3px;'

    @staticmethod
    def _copy_btn_style(dark: bool) -> str:
        if dark:
            return '''
                QPushButton {
                    background:#37474f; color:#cfd8dc; border:none;
                    border-radius:3px; font-size:11px;
                }
                QPushButton:hover   { background:#546e7a; color:white; }
                QPushButton:pressed { background:#263238; }
                QPushButton:disabled{ background:#2a2a2a; color:#555; }
            '''
        return '''
            QPushButton {
                background:#cfd8dc; color:#263238; border:none;
                border-radius:3px; font-size:11px;
            }
            QPushButton:hover   { background:#b0bec5; color:#263238; }
            QPushButton:pressed { background:#90a4ae; }
            QPushButton:disabled{ background:#e0e0e0; color:#aaa; }
        '''

    @staticmethod
    def _send_btn_style(dark: bool) -> str:
        dis_bg = '#444'    if dark else '#e0e0e0'
        dis_fg = '#777'    if dark else '#aaa'
        return f'''
            QPushButton {{
                background:#2e7d32; color:white; border:none;
                border-radius:4px; font-size:14px; font-weight:bold;
            }}
            QPushButton:hover   {{ background:#388e3c; }}
            QPushButton:pressed {{ background:#1b5e20; }}
            QPushButton:disabled{{ background:{dis_bg}; color:{dis_fg}; }}
        '''

    def apply_theme(self, dark: bool) -> None:
        self.title_lbl.setStyleSheet(
            'font-size:15px; font-weight:bold; color:#dcdcdc;' if dark
            else 'font-size:15px; font-weight:bold; color:#1a1a1a;'
        )
        self.mid_edit.setStyleSheet(self._field_style(dark))
        self.fc_spin.setStyleSheet(self._field_style(dark))
        self.preview_lbl.setStyleSheet(self._preview_style(dark))
        self.copy_btn.setStyleSheet(self._copy_btn_style(dark))
        self.send_btn.setStyleSheet(self._send_btn_style(dark))
        self.desc_lbl.setStyleSheet('color:#999;' if dark else 'color:#666;')
        self.size_lbl.setStyleSheet(
            'font-family:monospace; color:#7ec8e3;' if dark
            else 'font-family:monospace; color:#1a6e8a;'
        )

    # ------------------------------------------------------------------
    # Public
    # ------------------------------------------------------------------

    def load_command(self, cmd: CommandDef) -> None:
        self.current_cmd = cmd
        self.title_lbl.setText(f'{cmd.app}  /  {cmd.name}')
        self.desc_lbl.setText(cmd.description or 'No description.')
        # Populate editable fields with JSON defaults (block signals to avoid
        # triggering a preview update before the form is ready)
        self.mid_edit.blockSignals(True)
        self.fc_spin.blockSignals(True)
        self.mid_edit.setText(f'0x{cmd.mid:04X}')
        self.fc_spin.setValue(cmd.fc)
        self.mid_edit.blockSignals(False)
        self.fc_spin.blockSignals(False)
        self.reset_btn.setEnabled(True)

        # Clear form
        while self.form_layout.rowCount():
            self.form_layout.removeRow(0)
        self.param_widgets.clear()
        self._param_order.clear()

        visible_params = [p for p in cmd.params if not p.get('hidden', False)]
        if not visible_params:
            empty_lbl = QLabel('No parameters — command is self-contained.')
            empty_lbl.setStyleSheet('color:#777; font-style:italic;')
            self.form_layout.addRow('', empty_lbl)

        for param in cmd.params:
            if param.get('hidden', False):
                self._param_order.append((param, None))
                continue
            pw = ParamWidget(param, self.endian)
            pw.changed.connect(self._schedule_preview_update)
            tip = param.get('description', '')
            pw.setToolTip(tip)

            lbl = QLabel(param['name'] + ':')
            lbl.setToolTip(tip)
            lbl.setAlignment(Qt.AlignRight | Qt.AlignVCenter)

            self.form_layout.addRow(lbl, pw)
            self.param_widgets.append(pw)
            self._param_order.append((param, pw))

        payload_size = sum(param_byte_size(p) for p in cmd.params)
        self.size_lbl.setText(f'Pkt: {6 + 2 + payload_size}B')
        self.send_btn.setEnabled(self.conn.is_connected)
        self._update_preview()

    def set_connected(self, connected: bool) -> None:
        if self.current_cmd:
            self.send_btn.setEnabled(connected)

    def send_command(self) -> None:
        if not self.current_cmd or not self.conn.is_connected:
            return
        try:
            mid = self._get_mid()
            fc  = self._get_fc()
            payload, errors = self._encode_all()
            if errors:
                self.log_message.emit('[ERROR] Fix field errors before sending.')
                return
            pkt = build_command_packet(mid, fc, payload, self.conn.seq_count)
            if self.conn.send(pkt):
                self.log_message.emit(
                    f'[SENT] {self.current_cmd.app}/{self.current_cmd.name}'
                    f'  MID=0x{mid:04X}  FC={fc}'
                    f'  {len(pkt)}B  seq={self.conn.seq_count}'
                )
                self.cmd_sent.emit()
                self._update_preview()
            else:
                self.log_message.emit('[ERROR] send() failed — socket not open?')
        except Exception as exc:
            self.log_message.emit(f'[ERROR] {exc}')

    # ------------------------------------------------------------------
    # Internal
    # ------------------------------------------------------------------

    def _encode_all(self) -> Tuple[bytes, bool]:
        payload = b''
        any_error = False
        for param, pw in self._param_order:
            if pw is None:
                # Hidden padding param — always encode with its default value.
                default_val = str(param.get('default', '0'))
                if param['type'] == 'enum' and not default_val:
                    default_val = next(iter(param.get('values', {'0': 0})), '0')
                payload += encode_param_array(default_val, param, self.endian)
            else:
                try:
                    payload += pw.encode()
                    pw.mark_error(False)
                except Exception:
                    pw.mark_error(True)
                    any_error = True
        return payload, any_error

    def _schedule_preview_update(self) -> None:
        QTimer.singleShot(0, self._update_preview)

    def _get_mid(self) -> int:
        text = self.mid_edit.text().strip()
        return int(text, 16) if text.startswith(('0x', '0X')) else int(text)

    def _get_fc(self) -> int:
        return self.fc_spin.value()

    def _reset_mid_fc(self) -> None:
        if not self.current_cmd:
            return
        self.mid_edit.setText(f'0x{self.current_cmd.mid:04X}')
        self.fc_spin.setValue(self.current_cmd.fc)

    def _update_preview(self) -> None:
        if not self.current_cmd:
            return
        try:
            mid = self._get_mid()
            fc  = self._get_fc()
            payload, _ = self._encode_all()
            pkt = build_command_packet(mid, fc, payload, self.conn.seq_count)
            self._last_hex = ' '.join(f'{b:02X}' for b in pkt)
            self.preview_lbl.setText(f'Packet ({len(pkt)}B): {self._last_hex}')
            self.copy_btn.setEnabled(True)
        except Exception as exc:
            self._last_hex = ''
            self.preview_lbl.setText(f'Packet: <error: {exc}>')
            self.copy_btn.setEnabled(False)

    def _copy_hex(self) -> None:
        if not self._last_hex:
            return
        QApplication.clipboard().setText(self._last_hex)
        # Brief visual confirmation on the button
        self.copy_btn.setText('Copied!')
        QTimer.singleShot(1200, lambda: self.copy_btn.setText('Copy'))


# ---------------------------------------------------------------------------
# AddCommandDialog
# ---------------------------------------------------------------------------

class AddCommandDialog(QDialog):
    def __init__(self, existing_apps: List[str], parent: QWidget = None):
        super().__init__(parent)
        self.setWindowTitle('Add New Command')
        self.setMinimumWidth(640)
        self.setMinimumHeight(520)
        self._build(existing_apps)

    def _build(self, existing_apps: List[str]) -> None:
        root = QVBoxLayout(self)

        form = QFormLayout()
        form.setSpacing(8)

        self.app_combo = QComboBox()
        self.app_combo.setEditable(True)
        self.app_combo.addItems(existing_apps)
        self.app_combo.setPlaceholderText('App name (new or existing)')
        form.addRow('App:', self.app_combo)

        self.name_edit = QLineEdit()
        self.name_edit.setPlaceholderText('e.g.  Noop')
        form.addRow('Command Name:', self.name_edit)

        self.mid_edit = QLineEdit()
        self.mid_edit.setPlaceholderText('0x1806')
        form.addRow('MID (hex):', self.mid_edit)

        self.fc_spin = QSpinBox()
        self.fc_spin.setRange(0, 127)
        form.addRow('Function Code:', self.fc_spin)

        self.desc_edit = QLineEdit()
        self.desc_edit.setPlaceholderText('Optional human-readable description')
        form.addRow('Description:', self.desc_edit)

        root.addLayout(form)

        root.addWidget(QLabel('Parameters:'))

        self.param_table = QTableWidget(0, 5)
        self.param_table.setHorizontalHeaderLabels(
            ['Name', 'Type', 'Length / Count', 'Default', 'Description']
        )
        hh = self.param_table.horizontalHeader()
        hh.setSectionResizeMode(0, QHeaderView.ResizeToContents)
        hh.setSectionResizeMode(1, QHeaderView.ResizeToContents)
        hh.setSectionResizeMode(2, QHeaderView.ResizeToContents)
        hh.setSectionResizeMode(3, QHeaderView.ResizeToContents)
        hh.setSectionResizeMode(4, QHeaderView.Stretch)
        self.param_table.setSelectionBehavior(QAbstractItemView.SelectRows)
        root.addWidget(self.param_table, 1)

        btn_row = QHBoxLayout()
        add_p = QPushButton('+ Add Parameter')
        add_p.clicked.connect(self._add_row)
        rm_p = QPushButton('Remove Selected')
        rm_p.clicked.connect(self._remove_row)
        up_p = QPushButton('▲')
        up_p.setFixedWidth(30)
        up_p.clicked.connect(self._move_up)
        dn_p = QPushButton('▼')
        dn_p.setFixedWidth(30)
        dn_p.clicked.connect(self._move_down)
        btn_row.addWidget(add_p)
        btn_row.addWidget(rm_p)
        btn_row.addWidget(up_p)
        btn_row.addWidget(dn_p)
        btn_row.addStretch()
        root.addLayout(btn_row)

        buttons = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        buttons.accepted.connect(self._validate_and_accept)
        buttons.rejected.connect(self.reject)
        root.addWidget(buttons)

    # ------------------------------------------------------------------

    def _add_row(self) -> None:
        row = self.param_table.rowCount()
        self.param_table.insertRow(row)

        self.param_table.setItem(row, 0, QTableWidgetItem('param'))

        type_cb = QComboBox()
        type_cb.addItems(ALL_TYPES)
        self.param_table.setCellWidget(row, 1, type_cb)

        self.param_table.setItem(row, 2, QTableWidgetItem(''))
        self.param_table.setItem(row, 3, QTableWidgetItem(''))
        self.param_table.setItem(row, 4, QTableWidgetItem(''))

    def _remove_row(self) -> None:
        rows = sorted({idx.row() for idx in self.param_table.selectedIndexes()},
                      reverse=True)
        for r in rows:
            self.param_table.removeRow(r)

    def _move_up(self) -> None:
        rows = sorted({idx.row() for idx in self.param_table.selectedIndexes()})
        for r in rows:
            if r > 0:
                self._swap_rows(r, r - 1)

    def _move_down(self) -> None:
        rows = sorted({idx.row() for idx in self.param_table.selectedIndexes()},
                      reverse=True)
        for r in rows:
            if r < self.param_table.rowCount() - 1:
                self._swap_rows(r, r + 1)

    def _swap_rows(self, a: int, b: int) -> None:
        for col in range(self.param_table.columnCount()):
            wa = self.param_table.cellWidget(a, col)
            wb = self.param_table.cellWidget(b, col)
            ia = self.param_table.takeItem(a, col)
            ib = self.param_table.takeItem(b, col)
            if wa and wb:
                # Both are widgets — swap text
                ta, tb = (w.currentText() if isinstance(w, QComboBox) else ''
                          for w in (wa, wb))
                if isinstance(wa, QComboBox):
                    wa.setCurrentText(tb)
                if isinstance(wb, QComboBox):
                    wb.setCurrentText(ta)
            elif ia and ib:
                self.param_table.setItem(a, col, ib)
                self.param_table.setItem(b, col, ia)

    def _validate_and_accept(self) -> None:
        if not self.name_edit.text().strip():
            QMessageBox.warning(self, 'Missing field', 'Command name is required.')
            return
        mid_text = self.mid_edit.text().strip()
        if not mid_text:
            QMessageBox.warning(self, 'Missing field', 'MID is required.')
            return
        try:
            int(mid_text, 16) if mid_text.startswith(('0x', '0X')) else int(mid_text)
        except ValueError:
            QMessageBox.warning(self, 'Invalid MID', f'Cannot parse MID: {mid_text!r}')
            return
        self.accept()

    # ------------------------------------------------------------------
    # Result accessors
    # ------------------------------------------------------------------

    def get_app_name(self) -> str:
        return self.app_combo.currentText().strip() or 'Unknown'

    def get_command_dict(self) -> dict:
        params = []
        for row in range(self.param_table.rowCount()):
            name_item = self.param_table.item(row, 0)
            type_cb = self.param_table.cellWidget(row, 1)
            len_item = self.param_table.item(row, 2)
            def_item = self.param_table.item(row, 3)
            desc_item = self.param_table.item(row, 4)

            param: dict = {
                'name': name_item.text() if name_item else f'param{row}',
                'type': type_cb.currentText() if type_cb else 'uint8',
                'description': desc_item.text() if desc_item else '',
            }
            if def_item and def_item.text():
                param['default'] = def_item.text()
            if len_item and len_item.text():
                try:
                    param['length'] = int(len_item.text())
                except ValueError:
                    try:
                        param['count'] = int(len_item.text())
                    except ValueError:
                        pass
            params.append(param)

        mid_text = self.mid_edit.text().strip()
        return {
            'name': self.name_edit.text().strip(),
            'mid': mid_text,
            'fc': self.fc_spin.value(),
            'description': self.desc_edit.text().strip(),
            'params': params,
        }


# ---------------------------------------------------------------------------
# MainWindow
# ---------------------------------------------------------------------------

class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.conn = UdpConnection()
        self._tlm = _TlmBridge()
        self._tlm.packet.connect(self._on_telemetry)
        self.conn.on_packet_received = lambda d: self._tlm.packet.emit(d)

        self.apps: List[AppDef] = []
        self.config = self._load_config()

        self._ping_pending: bool = False
        self._ping_timer: Optional[QTimer] = None
        self._dark_mode: bool = True
        self._log_history: List[Tuple[str, str]] = []   # (ts, raw_msg)

        self.setWindowTitle('CACTUS')
        self.setMinimumSize(960, 620)
        self.resize(1280, 800)

        self._build_toolbar()
        self._build_central()
        self._load_commands()

    # ------------------------------------------------------------------
    # Config
    # ------------------------------------------------------------------

    def _load_config(self) -> dict:
        try:
            with open(CONFIG_FILE, encoding='utf-8') as fh:
                return {**DEFAULT_CONFIG, **json.load(fh)}
        except Exception:
            return dict(DEFAULT_CONFIG)

    def _save_config(self) -> None:
        pass  # intentionally no-op — runtime changes are never written back to disk

    # ------------------------------------------------------------------
    # Toolbar
    # ------------------------------------------------------------------

    def _build_toolbar(self) -> None:
        tb = QToolBar('Connection')
        tb.setMovable(False)
        self.addToolBar(tb)

        def spacer(w: int = 6) -> QWidget:
            s = QWidget()
            s.setFixedWidth(w)
            return s

        # Mission selector — leftmost so it's always visible
        tb.addWidget(spacer())
        tb.addWidget(QLabel('Mission:'))
        tb.addWidget(spacer(4))
        self.mission_cb = QComboBox()
        self.mission_cb.setMinimumWidth(130)
        self._populate_mission_cb()
        self.mission_cb.currentIndexChanged.connect(self._mission_changed)
        tb.addWidget(self.mission_cb)

        new_mission_btn = QPushButton('+')
        new_mission_btn.setFixedWidth(26)
        new_mission_btn.setFixedHeight(24)
        new_mission_btn.setToolTip('Create a new mission preset directory')
        new_mission_btn.setStyleSheet('''
            QPushButton { background:#2e5c2e; color:white; border:none;
                          border-radius:3px; font-size:13px; font-weight:bold; }
            QPushButton:hover   { background:#388e3c; }
            QPushButton:pressed { background:#1b5e20; }
        ''')
        new_mission_btn.clicked.connect(self._new_mission)
        tb.addWidget(new_mission_btn)

        tb.addSeparator()

        tb.addWidget(QLabel('Target IP:'))
        tb.addWidget(spacer(4))
        self.ip_edit = QLineEdit(self.config['target_ip'])
        self.ip_edit.setFixedWidth(130)
        self.ip_edit.setPlaceholderText('127.0.0.1')
        tb.addWidget(self.ip_edit)

        tb.addWidget(spacer(10))
        tb.addWidget(QLabel('Cmd Port:'))
        tb.addWidget(spacer(4))
        self.cmd_port_spin = QSpinBox()
        self.cmd_port_spin.setRange(1, 65535)
        self.cmd_port_spin.setValue(self.config['cmd_port'])
        self.cmd_port_spin.setFixedWidth(72)
        tb.addWidget(self.cmd_port_spin)

        tb.addWidget(spacer(10))
        tb.addWidget(QLabel('Tlm Port:'))
        tb.addWidget(spacer(4))
        self.tlm_port_spin = QSpinBox()
        self.tlm_port_spin.setRange(1, 65535)
        self.tlm_port_spin.setValue(self.config['tlm_port'])
        self.tlm_port_spin.setFixedWidth(72)
        tb.addWidget(self.tlm_port_spin)

        tb.addSeparator()

        self.connect_btn = QPushButton('Connect')
        self.connect_btn.setFixedWidth(100)
        self.connect_btn.clicked.connect(self._toggle_connection)
        self._style_connect_btn(False)
        tb.addWidget(self.connect_btn)

        tb.addWidget(spacer(4))
        self.ping_btn = QPushButton('Ping')
        self.ping_btn.setFixedWidth(50)
        self.ping_btn.setEnabled(False)
        self.ping_btn.setToolTip('Send TO_LAB Output Enable and wait for telemetry reply (activates downlink and verifies the link)')
        self.ping_btn.clicked.connect(self._ping)
        tb.addWidget(self.ping_btn)

        tb.addSeparator()

        self.status_dot = QLabel('●')
        self.status_dot.setStyleSheet('color:#f44336; font-size:20px;')
        tb.addWidget(self.status_dot)
        tb.addWidget(spacer(2))
        self.status_lbl = QLabel('Disconnected')
        tb.addWidget(self.status_lbl)

        tb.addSeparator()

        tb.addWidget(QLabel(' Payload byte-order:'))
        tb.addWidget(spacer(4))
        self.endian_cb = QComboBox()
        self.endian_cb.addItems(['Little-Endian (x86)', 'Big-Endian'])
        self.endian_cb.setCurrentIndex(0 if self.config['endian'] == 'little' else 1)
        self.endian_cb.currentIndexChanged.connect(self._endian_changed)
        tb.addWidget(self.endian_cb)

        tb.addSeparator()

        self._theme_btn = QPushButton('☀')
        self._theme_btn.setFixedWidth(30)
        self._theme_btn.setFixedHeight(24)
        self._theme_btn.setToolTip('Toggle light / dark mode')
        self._theme_btn.clicked.connect(self._toggle_theme)
        tb.addWidget(self._theme_btn)
        tb.addWidget(spacer(4))
        self._theme_lbl = QLabel('Dark Mode')
        tb.addWidget(self._theme_lbl)


    def _apply_theme_visuals(self) -> None:
        """Apply palette, widget styles, and tree colors for the current _dark_mode."""
        dark = self._dark_mode
        QApplication.instance().setPalette(make_palette(dark))
        self.cmd_panel.apply_theme(dark)
        self._populate_tree(self.search_edit.text())

        if dark:
            self.delete_btn.setStyleSheet('''
                QPushButton { background:#5c1a1a; color:#ef9a9a; border:none;
                              border-radius:3px; padding:3px 6px; }
                QPushButton:hover   { background:#7b2020; color:white; }
                QPushButton:pressed { background:#3e1010; }
            ''')
        else:
            self.delete_btn.setStyleSheet('''
                QPushButton { background:#ffcdd2; color:#b71c1c; border:none;
                              border-radius:3px; padding:3px 6px; }
                QPushButton:hover   { background:#ef9a9a; color:#b71c1c; }
                QPushButton:pressed { background:#e57373; }
            ''')

        # Re-render log history with current theme colors
        self._rerender_log()
        # Telemetry has no stored history — old entries keep their colors,
        # new entries will use the correct theme colors

    def _toggle_theme(self) -> None:
        self._dark_mode = not self._dark_mode
        going_light = not self._dark_mode

        self._theme_btn.setText('☀' if self._dark_mode else '🌙')
        self._theme_lbl.setText('Dark Mode' if self._dark_mode else 'Light Mode')

        if going_light and _LIGHTMODECAT_DIR.is_dir():
            imgs = sorted(
                str(p) for p in _LIGHTMODECAT_DIR.iterdir()
                if p.suffix.lower() in ('.jpg', '.jpeg', '.png', '.gif', '.webp')
            )
            if imgs:
                self._theme_btn.setEnabled(False)
                overlay = CatAnimationOverlay(self, imgs)
                # Theme flips visually when the last frame appears
                overlay.last_frame.connect(self._apply_theme_visuals)
                overlay.finished.connect(lambda: self._theme_btn.setEnabled(True))
                return  # visuals applied later via last_frame signal

        # Immediate apply: dark mode toggle, or light mode with no cat images
        self._apply_theme_visuals()

    def _style_connect_btn(self, connected: bool) -> None:
        if connected:
            self.connect_btn.setText('Disconnect')
            self.connect_btn.setStyleSheet('''
                QPushButton { background:#c62828; color:white; border:none;
                              padding:4px 8px; border-radius:3px; }
                QPushButton:hover { background:#d32f2f; }
            ''')
        else:
            self.connect_btn.setText('Connect')
            self.connect_btn.setStyleSheet('''
                QPushButton { background:#1565c0; color:white; border:none;
                              padding:4px 8px; border-radius:3px; }
                QPushButton:hover { background:#1976d2; }
            ''')

    # ------------------------------------------------------------------
    # Central widget
    # ------------------------------------------------------------------

    def _build_central(self) -> None:
        outer = QSplitter(Qt.Vertical)

        # Top: left tree + right tabs
        top_split = QSplitter(Qt.Horizontal)

        # ── Left panel ───────────────────────────────────────────────
        left = QWidget()
        ll = QVBoxLayout(left)
        ll.setContentsMargins(4, 4, 4, 4)
        ll.setSpacing(4)

        search_row = QHBoxLayout()
        self.search_edit = QLineEdit()
        self.search_edit.setPlaceholderText('Search commands...')
        self.search_edit.textChanged.connect(self._filter_tree)
        search_row.addWidget(self.search_edit)

        add_btn = QPushButton('+')
        add_btn.setFixedWidth(28)
        add_btn.setToolTip('Add a new command definition (saved to JSON)')
        add_btn.clicked.connect(self._add_command)
        search_row.addWidget(add_btn)
        ll.addLayout(search_row)

        self.tree = QTreeWidget()
        self.tree.setHeaderHidden(True)
        self.tree.setIndentation(14)
        self.tree.itemClicked.connect(self._tree_clicked)
        self.tree.currentItemChanged.connect(self._tree_current_changed)
        ll.addWidget(self.tree)

        btn_row = QHBoxLayout()
        btn_row.setSpacing(4)
        reload_btn = QPushButton('Reload')
        reload_btn.setToolTip('Reload commands from the current mission directory')
        reload_btn.clicked.connect(self._load_commands)
        btn_row.addWidget(reload_btn)

        self.delete_btn = QPushButton('Delete')
        self.delete_btn.setToolTip('Delete the selected command or app from the mission JSON')
        self.delete_btn.setStyleSheet('''
            QPushButton { background:#5c1a1a; color:#ef9a9a; border:none;
                          border-radius:3px; padding:3px 6px; }
            QPushButton:hover   { background:#7b2020; color:white; }
            QPushButton:pressed { background:#3e1010; }
        ''')
        self.delete_btn.clicked.connect(self._delete_selected)
        btn_row.addWidget(self.delete_btn)
        ll.addLayout(btn_row)

        top_split.addWidget(left)
        top_split.setStretchFactor(0, 1)

        # ── Right panel ──────────────────────────────────────────────
        self.tabs = QTabWidget()

        self.cmd_panel = CommandPanel(self.conn)
        self.cmd_panel.log_message.connect(self._log)
        self.tabs.addTab(self.cmd_panel, 'Command')

        tlm_widget = self._build_tlm_tab()
        self.tabs.addTab(tlm_widget, 'Telemetry')

        top_split.addWidget(self.tabs)
        top_split.setStretchFactor(1, 3)
        top_split.setSizes([290, 900])

        outer.addWidget(top_split)
        outer.setStretchFactor(0, 3)

        # ── Bottom panel: scene (left) + log (right) ──────────────────
        bot_split = QSplitter(Qt.Horizontal)

        # Scene widget — aligns with the command tree below it
        self.scene = SceneWidget()
        bot_split.addWidget(self.scene)
        bot_split.setStretchFactor(0, 0)

        # Log panel
        log_w = QWidget()
        log_l = QVBoxLayout(log_w)
        log_l.setContentsMargins(4, 2, 4, 4)
        log_l.setSpacing(2)

        log_hdr = QHBoxLayout()
        log_hdr.addWidget(QLabel('Log'))
        clr = QPushButton('Clear')
        clr.setFixedWidth(55)
        clr.clicked.connect(lambda: self.log_edit.clear())
        log_hdr.addWidget(clr)
        log_hdr.addStretch()
        log_l.addLayout(log_hdr)

        self.log_edit = QTextEdit()
        self.log_edit.setReadOnly(True)
        self.log_edit.setFont(QFont('Courier', 9))
        log_l.addWidget(self.log_edit)

        bot_split.addWidget(log_w)
        bot_split.setStretchFactor(1, 1)
        bot_split.setSizes([290, 900])   # mirror top_split sizes

        outer.addWidget(bot_split)
        outer.setStretchFactor(1, 1)
        outer.setSizes([560, 180])

        # Wire scene signals
        self.cmd_panel.cmd_sent.connect(self.scene.on_cmd_sent)

        self.setCentralWidget(outer)

    def _build_tlm_tab(self) -> QWidget:
        w = QWidget()
        l = QVBoxLayout(w)
        l.setContentsMargins(4, 4, 4, 4)

        hdr = QHBoxLayout()
        hdr.addWidget(QLabel('Received telemetry packets'))
        clr = QPushButton('Clear')
        clr.setFixedWidth(55)
        clr.clicked.connect(lambda: self.tlm_edit.clear())
        hdr.addWidget(clr)
        hdr.addStretch()
        l.addLayout(hdr)

        self.tlm_edit = QTextEdit()
        self.tlm_edit.setReadOnly(True)
        self.tlm_edit.setFont(QFont('Courier', 9))
        l.addWidget(self.tlm_edit)
        return w

    # ------------------------------------------------------------------
    # Commands
    # ------------------------------------------------------------------

    def _load_commands(self) -> None:
        path = self.mission_cb.currentData() if self.mission_cb.currentIndex() > 0 else None
        if path:
            self._load_commands_from(path)
        else:
            self.apps = []
            self._populate_tree()
            self._log('[INFO] No mission selected — choose a mission or add new commands')

    def _load_commands_from(self, directory: str) -> None:
        self.apps = load_commands_dir(directory)
        self._populate_tree()
        total = sum(len(a.commands) for a in self.apps)
        self._log(f'[INFO] Loaded {total} commands across {len(self.apps)} apps from {directory}')

    def _populate_tree(self, filter_text: str = '') -> None:
        self.tree.clear()
        fl = filter_text.lower()

        dark = self._dark_mode
        if not self.apps:
            placeholder = QTreeWidgetItem(['Select a mission or add new commands'])
            placeholder.setForeground(0, QColor('#555' if dark else '#999'))
            placeholder.setFlags(Qt.NoItemFlags)
            self.tree.addTopLevelItem(placeholder)
            return

        for app in self.apps:
            app_item = QTreeWidgetItem([f'  {app.name}'])
            app_item.setData(0, Qt.UserRole, None)
            f = app_item.font(0)
            f.setBold(True)
            app_item.setFont(0, f)
            app_item.setForeground(0, QColor('#7ec8e3' if dark else '#1a6e8a'))
            if app.description:
                app_item.setToolTip(0, app.description)

            matched = False
            for cmd in app.commands:
                if fl and fl not in cmd.name.lower() and fl not in app.name.lower():
                    continue
                ci = QTreeWidgetItem([f'    {cmd.name}'])
                ci.setData(0, Qt.UserRole, cmd)
                ci.setToolTip(0, cmd.description or '')
                app_item.addChild(ci)
                matched = True

            if not fl or matched:
                self.tree.addTopLevelItem(app_item)
                app_item.setExpanded(bool(fl))  # expand only when filtering

    def _filter_tree(self, text: str) -> None:
        self._populate_tree(text)

    def _load_tree_item(self, item: QTreeWidgetItem) -> None:
        if item is None:
            return
        cmd = item.data(0, Qt.UserRole)
        if isinstance(cmd, CommandDef):
            self.cmd_panel.endian = '<' if self.endian_cb.currentIndex() == 0 else '>'
            self.cmd_panel.load_command(cmd)
            self.tabs.setCurrentIndex(0)

    def _tree_clicked(self, item: QTreeWidgetItem, _col: int) -> None:
        self._load_tree_item(item)

    def _tree_current_changed(self, current: QTreeWidgetItem, _prev: QTreeWidgetItem) -> None:
        self._load_tree_item(current)

    # ------------------------------------------------------------------
    # Connection
    # ------------------------------------------------------------------

    def _toggle_connection(self) -> None:
        if self.conn.is_connected:
            self.conn.disconnect()
            self._set_connected(False)
        else:
            ip = self.ip_edit.text().strip() or '127.0.0.1'
            cp = self.cmd_port_spin.value()
            tp = self.tlm_port_spin.value()
            try:
                self.conn.connect(ip, cp, tp)
                self._set_connected(True)
                # Auto-ping to verify the uplink/downlink immediately
                self._ping()
            except Exception as exc:
                self._log(f'[ERROR] Could not open socket: {exc}')

    def _set_connected(self, connected: bool) -> None:
        self._style_connect_btn(connected)
        self.ping_btn.setEnabled(connected)
        if connected:
            # Yellow while ping is in flight; colour changes after ping result
            self.status_dot.setStyleSheet('color:#ffc107; font-size:20px;')
            ip, cp, tp = (self.ip_edit.text(),
                          self.cmd_port_spin.value(),
                          self.tlm_port_spin.value())
            self.status_lbl.setText(f'Socket open {ip}:{cp}')
            self._log(f'[INFO] Socket open  →  {ip}:{cp}  (tlm :{tp})  — verifying…')
            self.scene.set_state(SceneWidget.TX_ONLY)
        else:
            self.status_dot.setStyleSheet('color:#f44336; font-size:20px;')
            self.status_lbl.setText('Disconnected')
            self._log('[INFO] Disconnected')
            if self._ping_timer:
                self._ping_timer.stop()
                self._ping_pending = False
            self.scene.set_state(SceneWidget.DISCONNECTED)
        # Send button follows socket state, not ping result
        self.cmd_panel.set_connected(connected)

    # ------------------------------------------------------------------
    # Endianness
    # ------------------------------------------------------------------

    def _endian_changed(self, index: int) -> None:
        # Propagate to the command panel and all live param widgets
        endian = '<' if index == 0 else '>'
        self.cmd_panel.endian = endian
        for pw in self.cmd_panel.param_widgets:
            pw.endian = endian
        self.cmd_panel._schedule_preview_update()

    # ------------------------------------------------------------------
    # Ping / connection verification
    # ------------------------------------------------------------------

    def _ping(self) -> None:
        if not self.conn.is_connected or self._ping_pending:
            return
        # TO_LAB Output Enable: MID 0x1880, FC 6, payload = dest_IP (16-byte null-terminated string)
        # This activates telemetry output from cFS so the downlink can be verified.
        dest_ip = self.ip_edit.text().strip() or '127.0.0.1'
        ip_bytes = dest_ip.encode('ascii')[:15]
        payload = ip_bytes + b'\x00' * (16 - len(ip_bytes))
        pkt = build_command_packet(0x1880, 6, payload, self.conn.seq_count)
        self.conn.send(pkt)
        self._ping_pending = True
        self.status_lbl.setText('Enabling telemetry output…')
        self._ping_timer = QTimer(self)
        self._ping_timer.setSingleShot(True)
        self._ping_timer.timeout.connect(self._ping_timeout)
        self._ping_timer.start(1500)

    def _ping_timeout(self) -> None:
        self._ping_pending = False
        self.status_dot.setStyleSheet('color:#ff9800; font-size:20px;')
        self.status_lbl.setText(
            f'TX only (no reply) {self.ip_edit.text()}:{self.cmd_port_spin.value()}'
        )
        self._log('[PING] No telemetry within 1.5 s — Output Enable sent but no downlink received')
        self.scene.set_state(SceneWidget.TX_ONLY)

    # ------------------------------------------------------------------
    # Mission selection
    # ------------------------------------------------------------------

    def _scan_missions(self) -> List[str]:
        """Return sorted list of mission directory names under commands/missions/."""
        p = Path(COMMANDS_DIR) / 'missions'
        if not p.is_dir():
            return []
        return sorted(d.name for d in p.iterdir() if d.is_dir())

    def _populate_mission_cb(self) -> None:
        self.mission_cb.blockSignals(True)
        self.mission_cb.clear()
        self.mission_cb.addItem('— default —', userData=COMMANDS_DIR)
        for name in self._scan_missions():
            path = str(Path(COMMANDS_DIR) / 'missions' / name)
            self.mission_cb.addItem(name, userData=path)
        self.mission_cb.blockSignals(False)

    def _mission_changed(self, _index: int) -> None:
        path = self.mission_cb.currentData()
        if path:
            self._load_commands_from(path)

    def _center_dialog(self, dialog) -> None:
        """Move *dialog* to the center of the main window, clamped to the screen."""
        dialog.adjustSize()
        mw = self.frameGeometry()
        x = mw.left() + (mw.width()  - dialog.width())  // 2
        y = mw.top()  + (mw.height() - dialog.height()) // 2
        screen = QApplication.primaryScreen().availableGeometry()
        x = max(screen.left(), min(x, screen.right()  - dialog.width()))
        y = max(screen.top(),  min(y, screen.bottom() - dialog.height()))
        dialog.move(x, y)

    def _new_mission(self) -> None:
        dlg = QInputDialog(self)
        dlg.setWindowTitle('New Mission Preset')
        dlg.setLabelText('Mission name:')
        dlg.setInputMode(QInputDialog.TextInput)
        self._center_dialog(dlg)
        if not dlg.exec_():
            return
        name = dlg.textValue().strip()
        if not name:
            return

        mission_dir = Path(COMMANDS_DIR) / 'missions' / name
        if mission_dir.exists():
            box = QMessageBox(QMessageBox.Warning, 'Already Exists',
                              f'A mission named "{name}" already exists.',
                              QMessageBox.Ok, self)
            self._center_dialog(box)
            box.exec_()
            return

        mission_dir.mkdir(parents=True)

        preset_dir = Path(__file__).parent / 'default_preset'
        for src_name, dst_name in [('default_mission_defs.json', 'mission_defs.json'),
                                    ('default_cfe_core.json', 'cfe_core.json')]:
            src = preset_dir / src_name
            if src.exists():
                shutil.copy2(str(src), str(mission_dir / dst_name))

        self._populate_mission_cb()
        for i in range(self.mission_cb.count()):
            if self.mission_cb.itemText(i) == name:
                self.mission_cb.setCurrentIndex(i)
                break

        self._log(f'[INFO] Created mission "{name}" at {mission_dir}')

    # ------------------------------------------------------------------
    # Delete command / app
    # ------------------------------------------------------------------

    def _delete_selected(self) -> None:
        item = self.tree.currentItem()
        if not item:
            return

        mission_dir = (self.mission_cb.currentData()
                       if self.mission_cb.currentIndex() > 0 else None)
        if not mission_dir:
            self._log('[ERROR] No mission loaded — cannot delete')
            return

        cmd = item.data(0, Qt.UserRole)

        if isinstance(cmd, CommandDef):
            box = QMessageBox(QMessageBox.Question, 'Confirm Delete',
                              f'Delete command "{cmd.name}" from app "{cmd.app}"?\n\n'
                              f'This will permanently remove the entry from the mission '
                              f'JSON files and cannot be undone.',
                              QMessageBox.Yes | QMessageBox.No, self)
            box.setDefaultButton(QMessageBox.No)
            self._center_dialog(box)
            if box.exec_() != QMessageBox.Yes:
                return
            self._remove_command_from_json(cmd.app, cmd.name, mission_dir)
        else:
            app_name = item.text(0).strip()
            box = QMessageBox(QMessageBox.Question, 'Confirm Delete',
                              f'Delete all commands under "{app_name}"?\n\n'
                              f'This will permanently remove the entire app section from '
                              f'the mission JSON files and cannot be undone.',
                              QMessageBox.Yes | QMessageBox.No, self)
            box.setDefaultButton(QMessageBox.No)
            self._center_dialog(box)
            if box.exec_() != QMessageBox.Yes:
                return
            self._remove_app_from_json(app_name, mission_dir)

        self._load_commands()

    def _remove_command_from_json(self, app_name: str, cmd_name: str, directory: str) -> None:
        for json_file in sorted(Path(directory).glob('*.json')):
            if json_file.name == 'mission_defs.json':
                continue
            try:
                with json_file.open(encoding='utf-8') as fh:
                    data = json.load(fh)
            except Exception:
                continue

            modified = False
            entries = data if isinstance(data, list) else [data]
            for entry in entries:
                if entry.get('app') == app_name:
                    before = len(entry.get('commands', []))
                    entry['commands'] = [c for c in entry.get('commands', [])
                                         if c.get('name') != cmd_name]
                    if len(entry['commands']) != before:
                        modified = True

            if modified:
                with json_file.open('w', encoding='utf-8') as fh:
                    json.dump(data, fh, indent=2)
                self._log(f'[INFO] Deleted command "{cmd_name}" from {json_file.name}')
                return

        self._log(f'[WARN] Command "{cmd_name}" not found in any mission JSON file')

    def _remove_app_from_json(self, app_name: str, directory: str) -> None:
        for json_file in sorted(Path(directory).glob('*.json')):
            if json_file.name == 'mission_defs.json':
                continue
            try:
                with json_file.open(encoding='utf-8') as fh:
                    data = json.load(fh)
            except Exception:
                continue

            if isinstance(data, list):
                new_data = [e for e in data if e.get('app') != app_name]
                if len(new_data) != len(data):
                    if new_data:
                        with json_file.open('w', encoding='utf-8') as fh:
                            json.dump(new_data, fh, indent=2)
                    else:
                        json_file.unlink()
                    self._log(f'[INFO] Deleted app "{app_name}" from {json_file.name}')
                    return
            elif isinstance(data, dict) and data.get('app') == app_name:
                json_file.unlink()
                self._log(f'[INFO] Deleted app "{app_name}" — removed {json_file.name}')
                return

        self._log(f'[WARN] App "{app_name}" not found in any mission JSON file')

    # ------------------------------------------------------------------
    # Add command
    # ------------------------------------------------------------------

    def _add_command(self) -> None:
        app_names = [a.name for a in self.apps]
        dlg = AddCommandDialog(app_names, self)
        if dlg.exec_() != QDialog.Accepted:
            return

        cmd_dict = dlg.get_command_dict()
        app_name = dlg.get_app_name()
        cmds_dir = (self.mission_cb.currentData()
                    if self.mission_cb.currentIndex() > 0
                    else COMMANDS_DIR)
        if cmds_dir == COMMANDS_DIR:
            box = QMessageBox(QMessageBox.Warning, 'No Mission Selected',
                              'Please select or create a mission before adding commands.',
                              QMessageBox.Ok, self)
            self._center_dialog(box)
            box.exec_()
            return
        try:
            saved = save_command(cmd_dict, app_name, cmds_dir)
            self._log(f"[INFO] Saved '{cmd_dict['name']}' → {saved}")
            self._load_commands()
        except Exception as exc:
            box = QMessageBox(QMessageBox.Critical, 'Save Error', str(exc),
                              QMessageBox.Ok, self)
            self._center_dialog(box)
            box.exec_()

    # ------------------------------------------------------------------
    # Theme-aware colors for HTML log/telemetry output
    # ------------------------------------------------------------------

    def _tc(self) -> dict:
        """Return a dict of colors for the current theme."""
        if self._dark_mode:
            return {
                'ts':      '#666',
                'error':   '#ef9a9a',
                'sent':    '#a5d6a7',
                'info':    '#90caf9',
                'tlm_ts':  '#888',
                'tlm_mid': '#7ec8e3',
                'tlm_meta':'#aaa',
                'tlm_hex': '#666',
                'tlm_raw': '#aaa',
            }
        return {
            'ts':      '#888',
            'error':   '#c62828',
            'sent':    '#2e7d32',
            'info':    '#1565c0',
            'tlm_ts':  '#888',
            'tlm_mid': '#1a6e8a',
            'tlm_meta':'#555',
            'tlm_hex': '#777',
            'tlm_raw': '#555',
        }

    # ------------------------------------------------------------------
    # Telemetry
    # ------------------------------------------------------------------

    def _on_telemetry(self, data: bytes) -> None:
        if self._ping_pending:
            self._ping_pending = False
            if self._ping_timer:
                self._ping_timer.stop()
            self.status_dot.setStyleSheet('color:#4caf50; font-size:20px;')
            self.status_lbl.setText(
                f'Connected {self.ip_edit.text()}:{self.cmd_port_spin.value()}'
            )
            self._log('[PING] Telemetry received — uplink and downlink confirmed')
            self.scene.set_state(SceneWidget.VERIFIED)

        self.scene.on_tlm_received()

        hdr = parse_primary_header(data)
        ts = datetime.datetime.now().strftime('%H:%M:%S.%f')[:-3]
        c = self._tc()

        if hdr:
            preview = ' '.join(f'{b:02X}' for b in data[:24])
            if len(data) > 24:
                preview += ' …'
            line = (
                f'<span style="color:{c["tlm_ts"]}">[{ts}]</span> '
                f'<span style="color:{c["tlm_mid"]}">MID=0x{hdr.mid:04X}</span> '
                f'<span style="color:{c["tlm_meta"]}"> seq={hdr.seq_count}'
                f' len={hdr.data_length + 7}B</span> '
                f'<span style="color:{c["tlm_hex"]}">{preview}</span>'
            )
        else:
            line = (
                f'<span style="color:{c["tlm_ts"]}">[{ts}]</span> '
                f'<span style="color:{c["tlm_raw"]}">[RAW] {data.hex()}</span>'
            )

        self._append_capped(self.tlm_edit, line, 5000)

    # ------------------------------------------------------------------
    # Log
    # ------------------------------------------------------------------

    def _format_log_entry(self, ts: str, msg: str) -> str:
        c = self._tc()
        colour = c['error'] if '[ERROR]' in msg else c['sent'] if '[SENT]' in msg else c['info']
        return (f'<span style="color:{c["ts"]}">[{ts}]</span> '
                f'<span style="color:{colour}">{msg}</span>')

    def _log(self, msg: str) -> None:
        ts = datetime.datetime.now().strftime('%H:%M:%S.%f')[:-3]
        self._log_history.append((ts, msg))
        if len(self._log_history) > 2000:
            self._log_history.pop(0)
        self._append_capped(self.log_edit, self._format_log_entry(ts, msg), 2000)

    def _rerender_log(self) -> None:
        """Re-render all stored log entries with current theme colors."""
        self.log_edit.clear()
        for ts, msg in self._log_history:
            self.log_edit.append(self._format_log_entry(ts, msg))

    @staticmethod
    def _append_capped(widget: QTextEdit, html: str, max_blocks: int) -> None:
        """Append an HTML line and trim the document if it exceeds max_blocks."""
        widget.append(html)
        doc = widget.document()
        while doc.blockCount() > max_blocks:
            cursor = widget.textCursor()
            cursor.movePosition(cursor.Start)
            cursor.select(cursor.BlockUnderCursor)
            cursor.removeSelectedText()
            cursor.deleteChar()   # remove the trailing newline

    # ------------------------------------------------------------------
    # Cleanup
    # ------------------------------------------------------------------

    def closeEvent(self, event) -> None:
        self.conn.disconnect()
        event.accept()
