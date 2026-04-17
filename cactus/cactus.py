# -*- coding: utf-8 -*-
#!/usr/bin/env python3
"""
╭─ 🌵 CACTUS ───────────────────────────────────╮
│                                                │
│      A Convenient Alternative to the CFS       │
│        Telecommand Utility System              │
│                                                │
│                   ▄█▄ ✿                       │
│                  ▐███▌                         │
│               ▄█▄▐███▌▄█▄                      │
│              ▐██████████▌                      │
│               ▀▀▐███▌▀▀                        │
│                 ▐███▌ ▄█▄                      │
│                 ▐███▌ ▐███▌▄█▄    ∧ ∧          │
│                 ▐███▌ ▐██████▌  (=^·^=)        │
│                 ▐███▌  ▀▐███▌    (  ˘)つ       │
│                 ▐███▌   ▐███▌     U U          │
│              ▄▄▄█████▄▄▐███▌                   │
│             ░░░░░░░░░░░░░░░░░░                 │
│                                                │
╰────────────────────────────────────────────────╯

Usage:
    python cactus.py

Requirements:
    pip install PyQt5

Entire code written by Claude (Sonnet 4.6)
Directed by ryu@yonsei.ac.kr

"""

import sys

def main() -> None:
    try:
        from PyQt5.QtWidgets import QApplication
    except ImportError:
        print("PyQt5 is required.  Install it with:\n\n    pip install PyQt5\n")
        sys.exit(1)

    from gs.gui import MainWindow, make_palette

    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    app.setPalette(make_palette(dark=True))

    app.setApplicationName('CACTUS')
    app.setOrganizationName('Yonsei')

    window = MainWindow()
    window.show()
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()
