# -*- coding: utf-8 -*-
"""Animated scene widget for CACTUS.

Shows a little cat + cactus world whose mood reflects the connection state,
with flower/star particles flying up on CMD sent and falling on TLM received.
"""

import math
import random
from typing import List, Tuple

from PyQt5.QtCore import Qt, QRectF, QPointF, QTimer
from PyQt5.QtGui import (
    QColor, QLinearGradient, QPainter, QPainterPath,
    QPen, QBrush, QFont,
)
from PyQt5.QtWidgets import QWidget


# ---------------------------------------------------------------------------
# Particle
# ---------------------------------------------------------------------------

class _Particle:
    _COLORS = [
        QColor('#FF6B9D'), QColor('#FFD93D'), QColor('#6BCB77'),
        QColor('#4D96FF'), QColor('#FF6348'), QColor('#C77DFF'),
        QColor('#FF9F43'), QColor('#00D2D3'),
    ]

    def __init__(self, x: float, y: float, going_up: bool) -> None:
        self.x   = x
        self.y   = y
        self.vx  = random.uniform(-0.9, 0.9)
        self.vy  = (-random.uniform(1.8, 3.2) if going_up
                    else  random.uniform(1.2, 2.8))
        self.life  = 1.0
        self.decay = random.uniform(0.009, 0.018)
        self.color = random.choice(self._COLORS)
        self.size  = random.uniform(5, 10)
        self.angle = random.uniform(0, 360)
        self.spin  = random.uniform(-5, 5)
        self.star  = random.random() < 0.4

    def step(self) -> bool:
        self.x     += self.vx
        self.y     += self.vy
        self.vy    += 0.045          # gravity
        self.angle += self.spin
        self.life  -= self.decay
        return self.life > 0


# ---------------------------------------------------------------------------
# SceneWidget
# ---------------------------------------------------------------------------

class SceneWidget(QWidget):
    """Animated desert / meadow scene reflecting connection state."""

    DISCONNECTED = 'disconnected'
    TX_ONLY      = 'tx_only'
    VERIFIED     = 'verified'

    # palette per state ─────────────────────────────────────────────────────
    _SKY = {
        DISCONNECTED: (QColor('#8B4A2A'), QColor('#D4855A')),
        TX_ONLY:      (QColor('#9B5A20'), QColor('#E8A05A')),
        VERIFIED:     (QColor('#3A8EC8'), QColor('#88C8F0')),
    }
    _GND = {
        DISCONNECTED: (QColor('#B87848'), QColor('#7A4E28')),
        TX_ONLY:      (QColor('#C08040'), QColor('#886030')),
        VERIFIED:     (QColor('#58A030'), QColor('#386818')),
    }

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setMinimumSize(160, 110)

        self._state     = self.DISCONNECTED
        self._tick      = 0
        self._particles: List[_Particle] = []

        # clouds: [x_frac, y_frac, speed]
        self._clouds: List[List[float]] = [
            [0.10, 0.16, 0.00028],
            [0.48, 0.09, 0.00018],
            [0.75, 0.20, 0.00024],
        ]

        self._timer = QTimer(self)
        self._timer.timeout.connect(self._tick_frame)
        self._timer.start(33)   # ~30 fps

    # ── Public API ─────────────────────────────────────────────────────────

    def set_state(self, state: str) -> None:
        self._state = state

    def on_cmd_sent(self) -> None:
        w, h = self.width(), self.height()
        cx, cy = self._cat_anchor(w, h)
        for _ in range(5):
            self._particles.append(_Particle(
                cx + random.uniform(-18, 18), cy - 8, going_up=True))

    def on_tlm_received(self) -> None:
        w = self.width()
        for _ in range(4):
            self._particles.append(_Particle(
                random.uniform(0.05 * w, 0.95 * w), -4, going_up=False))

    # ── Animation ──────────────────────────────────────────────────────────

    def _tick_frame(self) -> None:
        self._tick += 1
        for c in self._clouds:
            c[0] += c[2]
            if c[0] > 1.12:
                c[0] = -0.18
        self._particles = [p for p in self._particles if p.step()]
        self.update()

    # ── Geometry helpers ───────────────────────────────────────────────────

    def _gnd_y(self, h: int) -> int:
        return int(h * 0.66)

    def _cat_anchor(self, w: int, h: int) -> Tuple[int, int]:
        """Bottom-centre of the cat body."""
        return int(w * 0.30), self._gnd_y(h)

    # ── paintEvent ─────────────────────────────────────────────────────────

    def paintEvent(self, _event) -> None:
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        w, h  = self.width(), self.height()
        gnd   = self._gnd_y(h)

        self._draw_sky   (p, w, h, gnd)
        self._draw_ground(p, w, h, gnd)

        if self._state == self.VERIFIED:
            self._draw_clouds (p, w, h)
            self._draw_ground_flowers(p, w, h, gnd)
        else:
            self._draw_sun(p, w, h)
            if self._state == self.TX_ONLY:
                self._draw_question_marks(p, w, h)

        self._draw_cactus   (p, w, h, gnd)
        self._draw_cat      (p, w, h, gnd)
        self._draw_particles(p)
        self._draw_label    (p, w, h)
        p.end()

    # ── Sky / ground ───────────────────────────────────────────────────────

    def _draw_sky(self, p, w, h, gnd):
        t, b = self._SKY.get(self._state, self._SKY[self.DISCONNECTED])
        g = QLinearGradient(0, 0, 0, gnd)
        g.setColorAt(0, t); g.setColorAt(1, b)
        p.fillRect(0, 0, w, gnd, g)

    def _draw_ground(self, p, w, h, gnd):
        t, b = self._GND.get(self._state, self._GND[self.DISCONNECTED])
        g = QLinearGradient(0, gnd, 0, h)
        g.setColorAt(0, t); g.setColorAt(1, b)
        p.fillRect(0, gnd, w, h - gnd, g)

    # ── Sun ────────────────────────────────────────────────────────────────

    def _draw_sun(self, p, w, h):
        sx, sy = int(w * 0.83), int(h * 0.13)
        r = max(9, int(w * 0.065))
        p.setPen(Qt.NoPen)
        for gr in range(r + 10, r - 1, -3):
            p.setBrush(QBrush(QColor(255, 200, 60, 40)))
            p.drawEllipse(QRectF(sx - gr, sy - gr, gr * 2, gr * 2))
        p.setBrush(QBrush(QColor('#FFD93D')))
        p.drawEllipse(QRectF(sx - r, sy - r, r * 2, r * 2))

    # ── Clouds ─────────────────────────────────────────────────────────────

    def _draw_clouds(self, p, w, h):
        p.setPen(Qt.NoPen)
        for xf, yf, _ in self._clouds:
            cx, cy = int(xf * w), int(yf * h)
            r = max(14, int(w * 0.09))
            p.setBrush(QBrush(QColor(255, 255, 255, 210)))
            for dx, dy, dr in [(0, 0, 1.0), (-r*.6, r*.25, .72),
                               (r*.6, r*.25, .72), (0, r*.35, .65)]:
                rr = r * dr
                p.drawEllipse(QRectF(cx+dx-rr, cy+dy-rr, rr*2, rr*2))

    # ── Ground flowers ─────────────────────────────────────────────────────

    def _draw_ground_flowers(self, p, w, h, gnd):
        rng = random.Random(7)          # stable seed → static layout
        p.setPen(Qt.NoPen)
        colors = [QColor('#FF6B9D'), QColor('#FFD93D'),
                  QColor('#FF6348'), QColor('#C77DFF'), QColor('#4D96FF')]
        for _ in range(11):
            fx = rng.uniform(0.04, 0.96) * w
            fy = gnd + rng.uniform(3, (h - gnd) * 0.55)
            self._small_flower(p, fx, fy, rng.choice(colors))

    def _small_flower(self, p, x, y, col):
        r = 2.8
        p.setBrush(QBrush(col))
        for a in range(0, 360, 72):
            rad = math.radians(a)
            p.drawEllipse(QRectF(x + math.cos(rad)*r*1.7 - r,
                                 y + math.sin(rad)*r*1.7 - r, r*2, r*2))
        p.setBrush(QBrush(QColor('#FDEF72')))
        p.drawEllipse(QRectF(x - r, y - r, r*2, r*2))

    # ── Question marks (TX-only) ───────────────────────────────────────────

    def _draw_question_marks(self, p, w, h):
        t = self._tick * 0.06
        font = QFont('Arial', max(9, int(w * 0.065)), QFont.Bold)
        p.setFont(font)
        for i, (xf, yf) in enumerate([(0.57, 0.28), (0.68, 0.16), (0.77, 0.32)]):
            alpha = int(110 + 90 * math.sin(t + i * 1.3))
            p.setPen(QColor(255, 210, 70, alpha))
            p.drawText(int(w * xf), int(h * yf), '?')

    # ── Cactus ─────────────────────────────────────────────────────────────

    def _draw_cactus(self, p, w, h, gnd):
        p.setPen(Qt.NoPen)

        cx   = int(w * 0.76)
        sw   = max(7, int(w * 0.055))    # stem width
        sh   = int(h * 0.32)             # stem height
        aw   = int(sw * 0.72)            # arm thickness
        green = QColor('#3A7D2E')
        p.setBrush(QBrush(green))

        # Main stem
        p.drawRoundedRect(QRectF(cx - sw//2, gnd - sh, sw, sh), sw//2, sw//2)

        # Left arm: horizontal stub then up
        alh = int(sw * 1.7)             # horizontal length
        alv = int(sh * 0.28)            # vertical length
        aly = gnd - int(sh * 0.38)      # y where arm leaves stem
        p.drawRoundedRect(QRectF(cx - sw//2 - alh, aly, alh + sw//2, aw), 3, 3)
        p.drawRoundedRect(QRectF(cx - sw//2 - alh, aly - alv, aw, alv), 3, 3)

        # Right arm: lower than left
        arh = int(sw * 1.5)
        arv = int(sh * 0.22)
        ary = gnd - int(sh * 0.52)
        p.drawRoundedRect(QRectF(cx + sw//2 - sw//2, ary, arh, aw), 3, 3)
        p.drawRoundedRect(QRectF(cx + sw//2 + arh - aw, ary - arv, aw, arv), 3, 3)

        # Spines
        p.setPen(QPen(QColor('#2A5C20'), 1))
        spine_xs = [(cx - sw//2 - 3, cx - sw//2),
                    (cx + sw//2, cx + sw//2 + 3)]
        for yf in [0.15, 0.35, 0.55, 0.75, 0.90]:
            sy = gnd - sh + sh * yf
            for x0, x1 in spine_xs:
                p.drawLine(int(x0), int(sy), int(x1), int(sy))

    # ── Cat ────────────────────────────────────────────────────────────────

    def _draw_cat(self, p, w, h, gnd):
        t = self._tick * 0.055
        blink = (self._tick % 110) < 3
    
        cx, cy = self._cat_anchor(w, h)
        u  = max(7, int(w * 0.042))     # base unit

        bw = u * 2.1                    # body width
        bh = u * 1.5                    # body height
        hr = u * 1.08                   # head radius

        p.setPen(Qt.NoPen)

        # Tail
        wag = math.sin(t) * 16
        p.setBrush(Qt.NoBrush)
        tx0 = cx + bw * 0.42
        ty0 = cy - u * 0.4
        tail = QPainterPath()
        tail.moveTo(tx0, ty0)
        tail.cubicTo(tx0 + u*1.6, ty0 - u*0.4,
                     tx0 + u*2.1 + wag*0.25,  ty0 - u*1.9 + wag*0.25,
                     tx0 + u*1.6,  ty0 - u*2.9 + wag*0.25)
        p.setPen(QPen(QColor('#222222'), u * 0.62, Qt.SolidLine, Qt.RoundCap, Qt.RoundJoin))
        p.drawPath(tail)
        p.setPen(Qt.NoPen)

        # Body — black oval
        p.setBrush(QBrush(QColor('#1A1A1A')))
        p.drawEllipse(QRectF(cx - bw/2, cy - bh, bw, bh))

        # Head — white circle, overlapping top of body so no gap
        hx = cx - u * 0.08
        hy = cy - bh + hr * 0.25      # overlap: head centre sits inside body top
        p.setBrush(QBrush(QColor('white')))
        p.drawEllipse(QRectF(hx - hr, hy - hr, hr*2, hr*2))

        # Ears — black triangles
        p.setBrush(QBrush(QColor('#1A1A1A')))
        for side in (-1, 1):
            ex = hx + side * hr * 0.63
            ey = hy - hr * 0.65
            ear = QPainterPath()
            ear.moveTo(ex,                    ey - hr*0.70)
            ear.lineTo(ex - side*hr*0.42,     ey + hr*0.12)
            ear.lineTo(ex + side*hr*0.42,     ey + hr*0.12)
            ear.closeSubpath()
            p.drawPath(ear)
            # pink inner
            inn = QPainterPath()
            inn.moveTo(ex,                    ey - hr*0.52)
            inn.lineTo(ex - side*hr*0.26,     ey + hr*0.08)
            inn.lineTo(ex + side*hr*0.26,     ey + hr*0.08)
            inn.closeSubpath()
            p.setBrush(QBrush(QColor('#FF9DAE')))
            p.drawPath(inn)
            p.setBrush(QBrush(QColor('#1A1A1A')))

        # Eyes — two small black dots
        ey_y = hy - hr * 0.04
        er   = max(1.5, hr * 0.13)
        p.setBrush(QBrush(QColor('#111111')))
        for side in (-1, 1):
            ex = hx + side * hr * 0.37
            p.drawEllipse(QRectF(ex - er, ey_y - er, er*2, er*2))

        # Nose — tiny pink dot
        p.setBrush(QBrush(QColor('#FF8FAB')))
        ns = hr * 0.14
        p.drawEllipse(QRectF(hx - ns/2, hy + hr*0.15, ns, ns))

        # Mouth
        p.setPen(QPen(QColor('#3A2A1A'), max(1, u//6)))
        mp = QPainterPath()
        if self._state == self.VERIFIED:         # happy ᵕ
            mp.moveTo(hx - hr*0.21, hy + hr*0.28)
            mp.quadTo(hx, hy + hr*0.46, hx + hr*0.21, hy + hr*0.28)
        elif self._state == self.TX_ONLY:        # worried
            mp.moveTo(hx - hr*0.21, hy + hr*0.42)
            mp.quadTo(hx, hy + hr*0.26, hx + hr*0.21, hy + hr*0.42)
        else:                                    # flat —
            mp.moveTo(hx - hr*0.20, hy + hr*0.32)
            mp.lineTo(hx + hr*0.20, hy + hr*0.32)
        p.drawPath(mp)

        # Paws — small black ovals at ground level
        p.setPen(Qt.NoPen)
        p.setBrush(QBrush(QColor('#1A1A1A')))
        for side in (-0.48, 0.48):
            px = cx + side * bw * 0.55
            p.drawEllipse(QRectF(px - u*0.52, cy - u*0.38, u*1.04, u*0.52))

    # ── Particles ──────────────────────────────────────────────────────────

    def _draw_particles(self, p):
        p.setPen(Qt.NoPen)
        for pt in self._particles:
            alpha = int(pt.life * 230)
            col   = QColor(pt.color)
            col.setAlpha(alpha)
            p.save()
            p.translate(pt.x, pt.y)
            p.rotate(pt.angle)
            if pt.star:
                self._draw_star(p, col, pt.size)
            else:
                self._draw_flower_particle(p, col, pt.size, alpha)
            p.restore()

    def _draw_flower_particle(self, p, col, size, alpha):
        r = size / 2
        p.setBrush(QBrush(col))
        for a in range(0, 360, 72):
            rad = math.radians(a)
            p.drawEllipse(QRectF(
                math.cos(rad)*r*1.55 - r*0.62,
                math.sin(rad)*r*1.55 - r*0.62,
                r*1.24, r*1.24))
        ctr = QColor('#FDEF72'); ctr.setAlpha(alpha)
        p.setBrush(QBrush(ctr))
        p.drawEllipse(QRectF(-r*0.62, -r*0.62, r*1.24, r*1.24))

    def _draw_star(self, p, col, size):
        path = QPainterPath()
        outer = size / 2
        inner = outer * 0.42
        for i in range(10):
            a   = math.radians(i * 36 - 90)
            r   = outer if i % 2 == 0 else inner
            x, y = math.cos(a)*r, math.sin(a)*r
            path.moveTo(x, y) if i == 0 else path.lineTo(x, y)
        path.closeSubpath()
        p.setBrush(QBrush(col))
        p.drawPath(path)

    # ── Status label ───────────────────────────────────────────────────────

    def _draw_label(self, p, w, h):
        info = {
            self.DISCONNECTED: ('no signal', QColor('#FF9999')),
            self.TX_ONLY:      ('tx only',   QColor('#FFD080')),
            self.VERIFIED:     ('tx/rx ✓',   QColor('#90EE90')),
        }
        text, col = info.get(self._state, ('?', QColor('white')))
        font = QFont('Courier', max(7, int(w * 0.048)), QFont.Bold)
        p.setFont(font)
        p.setPen(QColor(0, 0, 0, 100))
        p.drawText(6, h - 5 + 1, text)
        p.setPen(col)
        p.drawText(5, h - 5, text)
