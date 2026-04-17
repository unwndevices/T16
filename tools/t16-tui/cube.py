"""ASCII wireframe cube renderer"""

import math


class Cube:
    VERTS = [
        (-1, -1, -1), ( 1, -1, -1), ( 1,  1, -1), (-1,  1, -1),
        (-1, -1,  1), ( 1, -1,  1), ( 1,  1,  1), (-1,  1,  1),
    ]
    EDGES = [
        (0, 1), (1, 2), (2, 3), (3, 0),   # back
        (4, 5), (5, 6), (6, 7), (7, 4),   # front
        (0, 4), (1, 5), (2, 6), (3, 7),   # depth
    ]

    def __init__(self, width=15, height=9):
        self.width = width
        self.height = height
        self.ax = 0.0
        self.ay = 0.0
        self.az = 0.0
        self.x_scale = width / 4.5
        self.y_scale = height / 4.5

    def _rotate(self, x, y, z):
        cx, sx = math.cos(self.ax), math.sin(self.ax)
        y, z = y * cx - z * sx, y * sx + z * cx
        cy, sy = math.cos(self.ay), math.sin(self.ay)
        x, z = x * cy + z * sy, -x * sy + z * cy
        cz, sz = math.cos(self.az), math.sin(self.az)
        x, y = x * cz - y * sz, x * sz + y * cz
        return x, y, z

    def _project(self, x, y, _z):
        sx = int(self.width / 2 + x * self.x_scale)
        sy = int(self.height / 2 - y * self.y_scale)
        return sx, sy

    def render(self):
        buf = [[' '] * self.width for _ in range(self.height)]

        proj = []
        for vx, vy, vz in self.VERTS:
            rx, ry, rz = self._rotate(vx, vy, vz)
            proj.append(self._project(rx, ry, rz))

        for i, j in self.EDGES:
            self._line(buf, proj[i], proj[j])

        for sx, sy in proj:
            if 0 <= sy < self.height and 0 <= sx < self.width:
                buf[sy][sx] = 'o'

        self.ax += 0.04
        self.ay += 0.03
        self.az += 0.02

        return [''.join(row) for row in buf]

    def _line(self, buf, p0, p1):
        x0, y0 = p0
        x1, y1 = p1
        dx = abs(x1 - x0)
        dy = abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1

        if dx == 0 and dy == 0:
            ch = 'o'
        elif dx > dy * 2:
            ch = '-'
        elif dy > dx * 2:
            ch = '|'
        elif (x1 - x0) * (y1 - y0) < 0:
            ch = '/'
        else:
            ch = '\\'

        err = dx - dy
        while True:
            if 0 <= y0 < self.height and 0 <= x0 < self.width:
                if buf[y0][x0] == ' ':
                    buf[y0][x0] = ch
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x0 += sx
            if e2 < dx:
                err += dx
                y0 += sy
