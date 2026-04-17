#!/usr/bin/env python3
"""T16/T32 MIDI Controller TUI Monitor"""

import curses
import math
import time

from cube import Cube
from notes import (
    compute_note_map, final_midi, format_note,
    SCALE_NAMES, NOTE_NAMES, MODE_NAMES, VEL_CURVES,
)

# ── grid characters ──────────────────────────────────────────────
CORNER = '+'
V_SEP  = ':'
CELL_W = 5          # content chars per cell

def dashed(n):
    return ''.join('-' if i % 2 == 0 else ' ' for i in range(n))

def dots(n):
    return '\u00b7' * n   # middle dot

# ── keyboard → grid maps ────────────────────────────────────────
KEY_MAP_16 = {
    ord('1'):  0, ord('2'):  1, ord('3'):  2, ord('4'):  3,
    ord('q'):  4, ord('w'):  5, ord('e'):  6, ord('r'):  7,
    ord('a'):  8, ord('s'):  9, ord('d'): 10, ord('f'): 11,
    ord('z'): 12, ord('x'): 13, ord('c'): 14, ord('v'): 15,
}
KEY_MAP_32 = {
    ord('1'):  0, ord('2'):  1, ord('3'):  2, ord('4'):  3,
    ord('5'):  4, ord('6'):  5, ord('7'):  6, ord('8'):  7,
    ord('q'):  8, ord('w'):  9, ord('e'): 10, ord('r'): 11,
    ord('y'): 13, ord('u'): 14, ord('i'): 15,
    ord('a'): 16, ord('s'): 17, ord('d'): 18, ord('f'): 19,
    ord('g'): 20, ord('h'): 21, ord('j'): 22, ord('k'): 23,
    ord('z'): 24, ord('x'): 25, ord('c'): 26, ord('v'): 27,
    ord('n'): 29, ord(','): 30, ord('.'): 31,
}

# ── header info panel ────────────────────────────────────────────
PANEL_W = 13       # content width inside the bordered panel


class App:
    def __init__(self, stdscr):
        self.scr = stdscr
        self.cube = Cube(width=15, height=9)

        # device state
        self.scale    = 1       # Ionian
        self.octave   = 2
        self.root     = 0       # C
        self.channel  = 1
        self.bank     = 0
        self.mode     = 0
        self.flip_x   = False
        self.flip_y   = False
        self.koala    = False
        self.vel_curve = 1

        # layout
        self.device = 'T16'
        self.cols   = 4
        self.rows   = 4

        # sim presses  { grid_idx: timestamp }
        self.pressed   = {}
        self.press_dur = 0.30

        # connection
        self.connected   = False
        self.pulse_phase = 0.0

        # curses setup
        curses.curs_set(0)
        curses.start_color()
        curses.use_default_colors()
        curses.init_pair(1, curses.COLOR_GREEN, -1)        # default green
        curses.init_pair(2, curses.COLOR_BLACK, curses.COLOR_GREEN)  # pressed
        curses.init_pair(3, curses.COLOR_RED, -1)          # disconnected
        curses.init_pair(4, curses.COLOR_YELLOW, -1)       # values
        self.scr.nodelay(True)
        self.scr.timeout(50)

    # ── main loop ────────────────────────────────────────────────
    def run(self):
        while True:
            self._input()
            self._render()

    # ── input ────────────────────────────────────────────────────
    def _input(self):
        key = self.scr.getch()
        if key == -1:
            return
        if key == 27:               # ESC
            raise SystemExit(0)

        km = KEY_MAP_32 if self.device == 'T32' else KEY_MAP_16
        if key in km:
            self.pressed[km[key]] = time.time()
            return

        if key == curses.KEY_UP:
            self.scale = (self.scale + 1) % len(SCALE_NAMES)
        elif key == curses.KEY_DOWN:
            self.scale = (self.scale - 1) % len(SCALE_NAMES)
        elif key == curses.KEY_RIGHT:
            self.octave = min(self.octave + 1, 10)
        elif key == curses.KEY_LEFT:
            self.octave = max(self.octave - 1, 0)
        elif key == ord(']'):
            self.root = (self.root + 1) % 12
        elif key == ord('['):
            self.root = (self.root - 1) % 12
        elif key == ord('b'):
            self.bank = (self.bank + 1) % 4
        elif key == ord('m'):
            self.mode = (self.mode + 1) % len(MODE_NAMES)
        elif key == ord('t'):
            if self.device == 'T16':
                self.device, self.cols = 'T32', 8
            else:
                self.device, self.cols = 'T16', 4

    def _is_pressed(self, idx):
        if idx in self.pressed:
            if time.time() - self.pressed[idx] < self.press_dur:
                return True
            del self.pressed[idx]
        return False

    # ── safe draw ────────────────────────────────────────────────
    def _put(self, y, x, text, attr=0):
        h, w = self.scr.getmaxyx()
        if y < 0 or y >= h or x >= w:
            return
        text = text[:max(0, w - x)]
        if not text:
            return
        try:
            self.scr.addstr(y, x, text, attr)
        except curses.error:
            pass

    # ── render ───────────────────────────────────────────────────
    def _render(self):
        self.scr.erase()
        my, mx = 1, 2                               # margins

        self._header(my, mx)

        gy = my + self.cube.height + 1
        self._grid(gy, mx)

        grid_h = self.rows * 2 + 1
        iy = gy + grid_h + 1
        self._info(iy, mx)

        info_rows = 7
        cy = iy + info_rows + 2 + 1                  # +2 borders +1 gap
        self._controls(cy, mx)

        self.scr.refresh()

    # ── header: cube + info panel ────────────────────────────────
    def _header(self, y, x):
        g     = curses.color_pair(1)
        dim   = g | curses.A_DIM
        bold  = g | curses.A_BOLD

        # cube
        for i, line in enumerate(self.cube.render()):
            self._put(y + i, x, line, dim)

        # info panel
        px = x + self.cube.width + 3
        pw = PANEL_W
        border = CORNER + dashed(pw) + CORNER
        sep    = V_SEP + dots(pw) + V_SEP

        now = time.localtime()
        name = ' '.join(self.device)

        self.pulse_phase += 0.12
        pulse = (math.sin(self.pulse_phase) + 1) / 2
        if self.connected:
            dot  = '\u25cf' if pulse > 0.5 else '\u25cb'
            stat = f"{dot} CONNECTED"
            sattr = bold if pulse > 0.5 else dim
        else:
            dot  = '\u25cf' if pulse > 0.5 else '\u25cb'
            stat = f"{dot} WAITING"
            sattr = curses.color_pair(3) | (curses.A_BOLD if pulse > 0.5 else curses.A_DIM)

        rows = [
            (border,                                           dim),
            (V_SEP + f" {name}".ljust(pw) + V_SEP,            bold),
            (sep,                                              dim),
            (V_SEP + f" {time.strftime('%Y-%m-%d', now)}".ljust(pw) + V_SEP, g),
            (V_SEP + f" {time.strftime('%H:%M:%S', now)}".ljust(pw) + V_SEP, g),
            (sep,                                              dim),
            (V_SEP + f" {stat}".ljust(pw) + V_SEP,            None),  # special
            (border,                                           dim),
        ]

        for i, (text, attr) in enumerate(rows):
            if attr is None:
                # status line: borders dim, content with sattr
                self._put(y + i, px, V_SEP, dim)
                self._put(y + i, px + 1, f" {stat}".ljust(pw), sattr)
                self._put(y + i, px + 1 + pw, V_SEP, dim)
            else:
                self._put(y + i, px, text, attr)

    # ── note grid ────────────────────────────────────────────────
    def _grid(self, y, x):
        g      = curses.color_pair(1)
        dim    = g | curses.A_DIM
        noteg  = g | curses.A_BOLD
        pressd = curses.color_pair(2) | curses.A_BOLD

        nmap = compute_note_map(
            self.scale, self.root,
            self.flip_x, self.flip_y,
            self.cols, self.rows,
        )

        h_line = CORNER + (dashed(CELL_W) + CORNER) * self.cols

        for row in range(self.rows + 1):
            by = y + row * 2
            self._put(by, x, h_line, dim)

            if row >= self.rows:
                break

            cy = by + 1
            cx = x
            for col in range(self.cols):
                idx  = row * self.cols + col
                midi = final_midi(nmap[idx], self.octave, self.koala)
                note = format_note(midi).center(CELL_W)
                hit  = self._is_pressed(idx)

                ba = pressd if hit else dim
                ta = pressd if hit else noteg

                self._put(cy, cx, V_SEP, ba)
                cx += 1
                self._put(cy, cx, note, ta)
                cx += CELL_W

            self._put(cy, cx, V_SEP, dim)

    # ── info table ───────────────────────────────────────────────
    def _info(self, y, x):
        g    = curses.color_pair(1)
        dim  = g | curses.A_DIM
        lbl  = g
        val  = curses.color_pair(4) | curses.A_BOLD

        lw, vw = 9, 13
        border = CORNER + dashed(lw) + CORNER + dashed(vw) + CORNER

        rows = [
            ("SCALE",    SCALE_NAMES[self.scale]),
            ("ROOT",     NOTE_NAMES[self.root]),
            ("OCTAVE",   str(self.octave)),
            ("CHANNEL",  str(self.channel)),
            ("BANK",     f"{self.bank + 1} / 4"),
            ("MODE",     MODE_NAMES[self.mode]),
            ("VELOCITY", VEL_CURVES[self.vel_curve]),
        ]

        self._put(y, x, border, dim)
        for i, (label, value) in enumerate(rows):
            ry = y + 1 + i
            self._put(ry, x, V_SEP, dim)
            self._put(ry, x + 1, label.ljust(lw), lbl)
            self._put(ry, x + 1 + lw, V_SEP, dim)
            self._put(ry, x + 2 + lw, value.ljust(vw), val)
            self._put(ry, x + 2 + lw + vw, V_SEP, dim)
        self._put(y + 1 + len(rows), x, border, dim)

    # ── controls footer ──────────────────────────────────────────
    def _controls(self, y, x):
        dim = curses.color_pair(1) | curses.A_DIM
        lines = [
            "[1-4/qwer/asdf/zxcv] play    [\u2191\u2193] scale   [\u2190\u2192] octave",
            "[[ ]] root   [b] bank   [m] mode   [t] T16/T32   [ESC] quit",
        ]
        for i, ln in enumerate(lines):
            self._put(y + i, x, ln, dim)


def main(stdscr):
    App(stdscr).run()


if __name__ == '__main__':
    try:
        curses.wrapper(main)
    except (SystemExit, KeyboardInterrupt):
        pass
