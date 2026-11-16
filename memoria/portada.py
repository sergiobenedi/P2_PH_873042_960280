"""portada.py - Cambia "TITULO" y "ASIGNATURA" de la portada de la P1 por los de la P2.

La portada de la plantilla EUPT es una imagen: se borran los textos rellenando cada pixel con el
del fondo mas cercano y se escriben los nuevos en blanco, Arial negrita, centrados.
Uso: python portada.py   (lee recursos/portada_p1.png y escribe recursos/portada.png)
"""
import os
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy import ndimage

AQUI = os.path.dirname(os.path.abspath(__file__))
REC = os.path.join(AQUI, "recursos")
ESC = 3  # se trabaja a 3x para que el texto nuevo salga nitido

TITULO = ["PRÁCTICA 2", "Sistema de E/S y dispositivos básicos"]
ASIGNATURA = "PROYECTO HARDWARE"

img = Image.open(os.path.join(REC, "portada_p1.png")).convert("RGB")
img = img.resize((img.width * ESC, img.height * ESC), Image.LANCZOS)
a = np.asarray(img).astype(np.int16)

# zonas de texto (en pixeles de la imagen original 732x1123), entre las lineas blancas
zonas = [(250, 478, 470, 545), (270, 575, 450, 608)]
mask = np.zeros(a.shape[:2], bool)
for x0, y0, x1, y1 in zonas:
    x0, y0, x1, y1 = (v * ESC for v in (x0, y0, x1, y1))
    sub = a[y0:y1, x0:x1]
    claro = sub.min(axis=2) > 110          # el fondo es azul: el texto blanco tiene R alto
    mask[y0:y1, x0:x1] = claro
mask = ndimage.binary_dilation(mask, iterations=3 * ESC)
# solo dentro de las zonas
lim = np.zeros_like(mask)
for x0, y0, x1, y1 in zonas:
    lim[y0 * ESC:y1 * ESC, x0 * ESC:x1 * ESC] = True
mask &= lim

# relleno con el pixel de fondo mas cercano (conserva los bordes diagonales del fondo)
_, (iy, ix) = ndimage.distance_transform_edt(mask, return_indices=True)
a = a[iy, ix]
img = Image.fromarray(a.astype(np.uint8))
# suavizar solo lo rellenado
from PIL import ImageFilter
suave = img.filter(ImageFilter.GaussianBlur(2 * ESC))
m = Image.fromarray((mask * 255).astype(np.uint8))
img = Image.composite(suave, img, m)

d = ImageDraw.Draw(img)
B = "C:/Windows/Fonts/arialbd.ttf"
cx = 358 * ESC  # centro de las lineas blancas de la plantilla (65-651 px)


def centrado(texto, y, tam):
    f = ImageFont.truetype(B, tam * ESC)
    d.text((cx, y * ESC), texto, font=f, fill="white", anchor="mm")


centrado(TITULO[0], 497, 40)
centrado(TITULO[1], 535, 21)
centrado(ASIGNATURA, 592, 19)

img.save(os.path.join(REC, "portada.png"))
print("portada.png", img.size)
