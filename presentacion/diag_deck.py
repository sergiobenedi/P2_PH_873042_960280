"""Dibuja los diagramas de las dos maquinas de estados de la P2 con Pillow."""
import math, sys
from PIL import Image, ImageDraw, ImageFont

OUT = sys.argv[1]
AZUL = (0x22, 0x3D, 0x74)
GRIS = (0x1F, 0x24, 0x2E)
FONT = "C:/Windows/Fonts/calibri.ttf"
FONTB = "C:/Windows/Fonts/calibrib.ttf"
S = 2  # escala (para que se vea nitido)

def fnt(sz, bold=False):
    return ImageFont.truetype(FONTB if bold else FONT, sz * S)

class Lienzo:
    def __init__(self, w, h):
        self.img = Image.new("RGB", (w * S, h * S), "white")
        self.d = ImageDraw.Draw(self.img)

    def caja(self, cx, cy, texto, sub=None, w=150, h=56):
        x0, y0, x1, y1 = (cx - w / 2) * S, (cy - h / 2) * S, (cx + w / 2) * S, (cy + h / 2) * S
        self.d.rounded_rectangle([x0, y0, x1, y1], radius=12 * S, outline=AZUL, width=3 * S, fill=(0xE9, 0xEE, 0xF6))
        f = fnt(17, True)
        ty = cy - (8 if sub else 0)
        self.d.text((cx * S, ty * S), texto, font=f, fill=AZUL, anchor="mm")
        if sub:
            self.d.text((cx * S, (cy + 13) * S), sub, font=fnt(13), fill=GRIS, anchor="mm")
        return (cx, cy, w, h)

    def texto(self, x, y, t, sz=15, anchor="mm", color=GRIS, bold=False):
        for i, linea in enumerate(t.split("\n")):
            self.d.text((x * S, (y + i * (sz + 3)) * S), linea, font=fnt(sz, bold), fill=color, anchor=anchor)

    def flecha(self, pts, color=GRIS):
        pts = [(x * S, y * S) for x, y in pts]
        self.d.line(pts, fill=color, width=2 * S, joint="curve")
        (x0, y0), (x1, y1) = pts[-2], pts[-1]
        a = math.atan2(y1 - y0, x1 - x0)
        L = 12 * S
        p1 = (x1 - L * math.cos(a - 0.4), y1 - L * math.sin(a - 0.4))
        p2 = (x1 - L * math.cos(a + 0.4), y1 - L * math.sin(a + 0.4))
        self.d.polygon([(x1, y1), p1, p2], fill=color)

    def guardar(self, nombre):
        self.img.save(f"{OUT}/{nombre}")


# ---------- 1. Antirrebotes ----------
c = Lienzo(820, 470)
E = c.caja(150, 90, "ESPERANDO", "EINT habilitadas")
P = c.caja(650, 90, "RETARDO_TRP", "timer0 = trp")
Q = c.caja(650, 360, "ENCUESTA", "timer0 = 50 ms")
D = c.caja(150, 360, "RETARDO_TRD", "timer0 = trd")

c.flecha([(225, 90), (575, 90)])
c.texto(400, 62, "T1: flanco de bajada (EINT6/7 o EINT1)\nenmascarar entradas, guardar botón")

c.flecha([(650, 118), (650, 332)])
c.texto(660, 200, "T2a: fin trp y\nsigue pulsado\naviso «pulsado»", anchor="lm")

c.flecha([(600, 118), (200, 332)])
c.texto(330, 185, "T2b: fin trp y suelto\n(pulso espurio, sin aviso)")

c.flecha([(725, 345), (790, 345), (790, 395), (725, 395)])
c.texto(700, 425, "T3: sigue pulsado,\nreprogramar 50 ms", anchor="mm")

c.flecha([(575, 360), (225, 360)])
c.texto(400, 385, "T4: botón suelto\naviso «soltado»")

c.flecha([(150, 332), (150, 118)])
c.texto(140, 200, "T5: fin trd\nborrar EXTINTPND\ne I_ISPC y luego\ndesenmascarar", anchor="rm")
c.guardar("diag_antirrebotes.png")

# ---------- 2. Entrada de la jugada ----------
c = Lienzo(900, 345)
I = c.caja(90, 150, "INICIO", "8led apagado", w=130)
F = c.caja(320, 150, "FILA", "8led = F, luego n", w=150)
C = c.caja(560, 150, "COLUMNA", "8led = C, luego n", w=150)
K = c.caja(800, 150, "CONFIRMAR", "led izq. parpadea", w=150)

c.flecha([(155, 150), (245, 150)])
c.texto(200, 125, "E1: cualquier\nbotón", sz=15)
c.flecha([(395, 150), (485, 150)])
c.texto(440, 125, "E2: der\n(n ≠ 0)", sz=15)
c.flecha([(635, 150), (725, 150)])
c.texto(680, 125, "E3: der\n(n ≠ 0)", sz=15)

# bucles de izq en FILA y COLUMNA
for x in (320, 560):
    c.flecha([(x - 30, 122), (x - 30, 60), (x + 30, 60), (x + 30, 122)])
c.texto(440, 30, "izq: n = n % 8 + 1  ·  mantenido 0,5 s: autoincremento cada 300 ms (timer1)", sz=15)

# vuelta de CONFIRMAR a FILA
c.flecha([(780, 178), (780, 260), (340, 260), (340, 178)])
c.texto(560, 245, "E4a: cualquier botón → jugada cancelada (id 8)", sz=15)
c.flecha([(820, 178), (820, 320), (300, 320), (300, 178)])
c.texto(560, 305, "E4b: 18 conmutaciones de 167 ms (3 s) → jugada lista para reversi8 (id 7)", sz=15)
c.guardar("diag_entrada.png")
print("ok")
