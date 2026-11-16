"""md2eupt.py - Genera la memoria con el formato de la memoria de la P1 (plantilla EUPT de Google Docs).

Uso: python md2eupt.py memoria.md salida.docx

Formato (medido en MEMORIA_P1_873042_960280.pdf):
  portada = imagen a pagina completa; cabecera con el logo de la UZ; pie con el logo de la EUPT y el n.o de pagina
  cuerpo Arial 12 justificado; H1 Arial negrita 16 #073763 centrado; H2 Arial negrita 16 #0b5394;
  H3 Arial negrita cursiva 15 #6fa8dc; tablas con cabecera #9fc5e8 y primera columna #cfe2f3;
  pies de tabla/figura Arial cursiva 11 #666666 centrados; codigo Consolas 10 #188038 sobre #f1f3f4.
Markdown: # ## ###, parrafos, - lista, 1. lista, ```codigo```, tablas |, ![pie](img.png),
lineas que empiezan por "Tabla N." o "Figura N" = pie centrado.
"""
import os, re, sys
from docx import Document
from docx.enum.section import WD_SECTION
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_CELL_VERTICAL_ALIGNMENT
from docx.enum.text import WD_ALIGN_PARAGRAPH, WD_BREAK
from docx.enum.style import WD_STYLE_TYPE
from docx.oxml import OxmlElement
from docx.oxml.ns import qn
from docx.shared import Cm, Pt, RGBColor

AQUI = os.path.dirname(os.path.abspath(__file__))
REC = os.path.join(AQUI, "recursos")
ANCHO_TEXTO = 21.0 - 1.8 - 1.7  # cm

C_H1, C_H2, C_H3 = "073763", "0b5394", "6fa8dc"
C_CAB, C_COL1, C_COD, C_TXTCOD, C_PIE = "9fc5e8", "cfe2f3", "f1f3f4", "188038", "666666"


def rgb(h):
    return RGBColor.from_string(h.upper())


def fuente(run, nombre="Arial", tam=12, negrita=None, cursiva=None, color=None):
    run.font.name = nombre
    rpr = run._element.get_or_add_rPr()
    rf = rpr.find(qn("w:rFonts"))
    if rf is None:
        rf = OxmlElement("w:rFonts"); rpr.insert(0, rf)
    for a in ("w:ascii", "w:hAnsi", "w:cs", "w:eastAsia"):
        rf.set(qn(a), nombre)
    run.font.size = Pt(tam)
    if negrita is not None:
        run.bold = negrita
    if cursiva is not None:
        run.italic = cursiva
    if color:
        run.font.color.rgb = rgb(color)


def estilo_fuente(st, nombre, tam, negrita=False, cursiva=False, color="000000"):
    st.font.name = nombre
    rpr = st.element.get_or_add_rPr()
    rf = rpr.find(qn("w:rFonts"))
    if rf is None:
        rf = OxmlElement("w:rFonts"); rpr.insert(0, rf)
    for a in list(rf.attrib):
        del rf.attrib[a]
    for a in ("w:ascii", "w:hAnsi", "w:cs", "w:eastAsia"):
        rf.set(qn(a), nombre)
    st.font.size = Pt(tam)
    st.font.bold = negrita
    st.font.italic = cursiva
    st.font.color.rgb = rgb(color)


def sombra(elem_pr, color):
    shd = OxmlElement("w:shd")
    shd.set(qn("w:val"), "clear"); shd.set(qn("w:color"), "auto"); shd.set(qn("w:fill"), color)
    elem_pr.append(shd)


def texto_inline(p, texto, tam=12, color=None, base_negrita=False, cursiva=False):
    """**negrita** y *cursiva*; los `codigo` van como texto normal (asi lo hace la P1)."""
    texto = texto.replace("\\_", "_").replace("\\*", "\x00")
    partes = re.split(r"(\*\*[^*]+\*\*|\*[^*\s][^*]*\*|`[^`]+`)", texto)
    for parte in partes:
        if not parte:
            continue
        neg, cur = base_negrita, cursiva
        if parte.startswith("**") and parte.endswith("**"):
            parte, neg = parte[2:-2], True
        elif parte.startswith("`") and parte.endswith("`"):
            parte = parte[1:-1]
        elif parte.startswith("*") and parte.endswith("*") and len(parte) > 2:
            parte, cur = parte[1:-1], True
        r = p.add_run(parte.replace("\x00", "*"))
        fuente(r, tam=tam, negrita=neg, cursiva=cur, color=color)


def preparar_estilos(doc):
    n = doc.styles["Normal"]
    estilo_fuente(n, "Arial", 12)
    pf = n.paragraph_format
    pf.alignment = WD_ALIGN_PARAGRAPH.JUSTIFY
    pf.line_spacing = 1.15
    pf.space_before = Pt(0)
    pf.space_after = Pt(11)

    for nivel, (tam, col, cur, ali, sb, sa, ind) in {
        1: (16, C_H1, False, WD_ALIGN_PARAGRAPH.CENTER, 18, 14, 0),
        2: (16, C_H2, False, WD_ALIGN_PARAGRAPH.LEFT, 14, 12, 1.23),
        3: (15, C_H3, True, WD_ALIGN_PARAGRAPH.LEFT, 10, 10, 0),
    }.items():
        st = doc.styles[f"Heading {nivel}"]
        estilo_fuente(st, "Arial", tam, negrita=True, cursiva=cur, color=col)
        pf = st.paragraph_format
        pf.alignment = ali
        pf.space_before, pf.space_after = Pt(sb), Pt(sa)
        pf.left_indent = Cm(ind)
        pf.keep_with_next = True
        pf.line_spacing = 1.15

    for nivel, (ind, neg) in {1: (0.3, True), 2: (1.0, False), 3: (1.7, False)}.items():
        nombre = f"toc {nivel}"
        try:
            st = doc.styles[nombre]
        except KeyError:
            st = doc.styles.add_style(nombre, WD_STYLE_TYPE.PARAGRAPH)
        st.base_style = doc.styles["Normal"]
        estilo_fuente(st, "Arial", 11, negrita=neg)
        pf = st.paragraph_format
        pf.left_indent = Cm(ind)
        pf.space_after = Pt(3)
        pf.alignment = WD_ALIGN_PARAGRAPH.LEFT
        pf.tab_stops.add_tab_stop(Cm(ANCHO_TEXTO - 0.3), alignment=2, leader=1)  # derecha, puntos


def campo(p, instr, texto="", tam=12, negrita=False):
    def fc(t):
        r = p.add_run(); fuente(r, tam=tam, negrita=negrita)
        e = OxmlElement("w:fldChar"); e.set(qn("w:fldCharType"), t); r._element.append(e)
    fc("begin")
    r = p.add_run(); fuente(r, tam=tam, negrita=negrita)
    it = OxmlElement("w:instrText"); it.set(qn("xml:space"), "preserve"); it.text = instr; r._element.append(it)
    fc("separate")
    r = p.add_run(texto); fuente(r, tam=tam, negrita=negrita)
    fc("end")


def sin_bordes(tabla):
    tblPr = tabla._tbl.tblPr
    b = OxmlElement("w:tblBorders")
    for lado in ("top", "left", "bottom", "right", "insideH", "insideV"):
        e = OxmlElement(f"w:{lado}"); e.set(qn("w:val"), "nil"); b.append(e)
    tblPr.append(b)


def portada_y_marco(doc):
    s1 = doc.sections[0]
    s1.page_width, s1.page_height = Cm(21.0), Cm(29.7)
    for m in ("left_margin", "right_margin", "top_margin", "bottom_margin"):
        setattr(s1, m, Cm(0))
    s1.header_distance = s1.footer_distance = Cm(0)
    p = doc.paragraphs[0] if doc.paragraphs else doc.add_paragraph()
    pf = p.paragraph_format
    pf.space_after = pf.space_before = Pt(0); pf.line_spacing = 1.0
    pf.alignment = WD_ALIGN_PARAGRAPH.LEFT
    r = p.add_run(); fuente(r, tam=1)
    r.add_picture(os.path.join(REC, "portada.png"), width=Cm(21.0), height=Cm(29.65))

    s2 = doc.add_section(WD_SECTION.NEW_PAGE)
    s2.left_margin, s2.right_margin = Cm(1.8), Cm(1.7)
    s2.top_margin, s2.bottom_margin = Cm(2.45), Cm(2.6)
    s2.header_distance, s2.footer_distance = Cm(0.62), Cm(0.15)

    cab = s2.header; cab.is_linked_to_previous = False
    hp = cab.paragraphs[0]
    hp.paragraph_format.left_indent = Cm(-1.34)
    hp.paragraph_format.space_after = Pt(0)
    hp.add_run().add_picture(os.path.join(REC, "logo_uz.png"), width=Cm(4.23))

    pie = s2.footer; pie.is_linked_to_previous = False
    t = pie.add_table(rows=1, cols=2, width=Cm(ANCHO_TEXTO))
    sin_bordes(t)
    c0, c1 = t.rows[0].cells
    c0.width, c1.width = Cm(8), Cm(ANCHO_TEXTO - 8)
    p0 = c0.paragraphs[0]
    p0.paragraph_format.left_indent = Cm(-0.8)
    p0.paragraph_format.space_after = Pt(0)
    p0.add_run().add_picture(os.path.join(REC, "logo_eupt.png"), width=Cm(5.54))
    p1 = c1.paragraphs[0]
    p1.alignment = WD_ALIGN_PARAGRAPH.RIGHT
    p1.paragraph_format.space_after = Pt(0)
    campo(p1, "PAGE", "2")
    c1.vertical_alignment = WD_CELL_VERTICAL_ALIGNMENT.TOP
    if pie.paragraphs:
        pie.paragraphs[0].paragraph_format.space_after = Pt(0)
        f = pie.paragraphs[0].runs[0] if pie.paragraphs[0].runs else pie.paragraphs[0].add_run()
        fuente(f, tam=1)

    # Pagina 2: tabla de contenidos
    p = doc.add_paragraph(); p.paragraph_format.space_after = Pt(0)
    p = doc.add_paragraph(); p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_before = Pt(24); p.paragraph_format.space_after = Pt(26)
    r = p.add_run("Tabla de contenidos"); fuente(r, tam=18, negrita=True)
    p = doc.add_paragraph(); p.alignment = WD_ALIGN_PARAGRAPH.LEFT
    campo(p, 'TOC \\o "1-3" \\h \\z \\u', "(actualizar la tabla de contenidos)", tam=11)
    p.add_run().add_break(WD_BREAK.PAGE)


def pie_figura(doc, texto):
    p = doc.add_paragraph(); p.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p.paragraph_format.space_after = Pt(14)
    texto_inline(p, texto, tam=11, color=C_PIE, cursiva=True)


def bloque_codigo(doc, lineas):
    n = len(lineas)
    for i, l in enumerate(lineas):
        p = doc.add_paragraph()
        pf = p.paragraph_format
        pf.alignment = WD_ALIGN_PARAGRAPH.LEFT
        pf.left_indent = Cm(0.35); pf.right_indent = Cm(0.35)
        pf.space_before = Pt(0)
        pf.space_after = Pt(10) if i == n - 1 else Pt(0)
        pf.line_spacing = Pt(17)
        pf.keep_together = True
        if i < n - 1:
            pf.keep_with_next = n <= 25
        ppr = p._element.get_or_add_pPr()
        bdr = OxmlElement("w:pBdr")
        for lado in ("top", "left", "bottom", "right"):
            e = OxmlElement(f"w:{lado}")
            e.set(qn("w:val"), "single"); e.set(qn("w:sz"), "4")
            e.set(qn("w:space"), "8" if lado in ("top", "bottom") else "6"); e.set(qn("w:color"), C_COD)
            bdr.append(e)
        ppr.append(bdr)
        sombra(ppr, C_COD)
        r = p.add_run(l if l else " ")
        fuente(r, "Consolas", 10, color=C_TXTCOD)


def anchos_columnas(filas):
    ncol = len(filas[0])
    largo = [max(len(re.sub(r"[*`]", "", f[c])) for f in filas) for c in range(ncol)]
    deseado = [max(1.9, l * 0.21 + 0.6) for l in largo]
    total = ANCHO_TEXTO - 0.6
    if sum(deseado) <= total:
        return deseado
    lo, hi = 0.0, max(deseado)
    for _ in range(50):
        c = (lo + hi) / 2
        if sum(min(d, c) for d in deseado) > total:
            hi = c
        else:
            lo = c
    return [min(d, lo) for d in deseado]


def tabla(doc, filas):
    ncol = max(len(f) for f in filas)
    filas = [f + [""] * (ncol - len(f)) for f in filas]
    anchos = anchos_columnas(filas)
    t = doc.add_table(rows=len(filas), cols=ncol)
    t.alignment = WD_TABLE_ALIGNMENT.CENTER
    tblPr = t._tbl.tblPr
    b = OxmlElement("w:tblBorders")
    for lado in ("top", "left", "bottom", "right", "insideH", "insideV"):
        e = OxmlElement(f"w:{lado}")
        e.set(qn("w:val"), "single"); e.set(qn("w:sz"), "6"); e.set(qn("w:color"), "000000")
        b.append(e)
    tblPr.append(b)
    lay = OxmlElement("w:tblLayout"); lay.set(qn("w:type"), "fixed"); tblPr.append(lay)
    mar = OxmlElement("w:tblCellMar")
    for lado, v in (("top", 85), ("left", 110), ("bottom", 85), ("right", 110)):
        e = OxmlElement(f"w:{lado}"); e.set(qn("w:w"), str(v)); e.set(qn("w:type"), "dxa"); mar.append(e)
    tblPr.append(mar)
    for gc, a in zip(t._tbl.tblGrid.findall(qn("w:gridCol")), anchos):
        gc.set(qn("w:w"), str(int(a * 567)))
    for ri, fila in enumerate(filas):
        row = t.rows[ri]
        trPr = row._tr.get_or_add_trPr()
        cs = OxmlElement("w:cantSplit"); trPr.append(cs)
        if ri == 0:
            th = OxmlElement("w:tblHeader"); trPr.append(th)
        for ci, txt in enumerate(fila):
            cell = row.cells[ci]
            cell.width = Cm(anchos[ci])
            cell.vertical_alignment = WD_CELL_VERTICAL_ALIGNMENT.CENTER
            tcPr = cell._tc.get_or_add_tcPr()
            if ri == 0:
                sombra(tcPr, C_CAB)
            elif ci == 0:
                sombra(tcPr, C_COL1)
            p = cell.paragraphs[0]
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            p.paragraph_format.space_after = Pt(0)
            p.paragraph_format.line_spacing = 1.0
            texto_inline(p, txt.strip().replace("\\|", "|"), tam=12, base_negrita=(ri == 0))
            # tablas cortas: no partirlas entre paginas y mantenerlas con su pie
            if len(filas) <= 6 or ri == len(filas) - 1:
                p.paragraph_format.keep_with_next = True
    return t


def lista(doc, marca, texto):
    p = doc.add_paragraph()
    pf = p.paragraph_format
    pf.left_indent = Cm(1.27); pf.first_line_indent = Cm(-0.63)
    pf.space_after = Pt(10)
    pf.tab_stops.add_tab_stop(Cm(1.27))
    r = p.add_run(marca + "\t"); fuente(r, color="1f2328" if marca == "●" else None)
    texto_inline(p, texto)


def convertir(md, salida):
    doc = Document()
    preparar_estilos(doc)
    portada_y_marco(doc)
    base = os.path.dirname(os.path.abspath(md))
    lineas = open(md, encoding="utf-8").read().splitlines()
    i = 0
    if lineas and lineas[0].strip() == "---":
        i = lineas.index("---", 1) + 1
    while i < len(lineas):
        l = lineas[i]
        s = l.strip()
        if not s:
            i += 1; continue
        if s.startswith("```"):
            j = i + 1; cod = []
            while not lineas[j].strip().startswith("```"):
                cod.append(lineas[j].rstrip()); j += 1
            bloque_codigo(doc, cod); i = j + 1; continue
        if s.startswith("|"):
            filas = []
            while i < len(lineas) and lineas[i].strip().startswith("|"):
                fila = [c for c in re.split(r"(?<!\\)\|", lineas[i].strip())[1:-1]]
                if not all(re.fullmatch(r"\s*:?-+:?\s*", c) for c in fila):
                    filas.append(fila)
                i += 1
            tabla(doc, filas)
            if i < len(lineas) and not lineas[i].strip():
                pass
            continue
        m = re.match(r"^(#{1,3})\s+(.*)$", s)
        if m:
            h = doc.add_paragraph(style=f"Heading {len(m.group(1))}")
            h.add_run(m.group(2).strip())
            i += 1; continue
        m = re.match(r"^!\[(.*)\]\((.*)\)$", s)
        if m:
            p = doc.add_paragraph(); p.alignment = WD_ALIGN_PARAGRAPH.CENTER
            p.paragraph_format.keep_with_next = True
            p.paragraph_format.space_after = Pt(4)
            p.add_run().add_picture(os.path.join(base, m.group(2)), width=Cm(16.5))
            pie_figura(doc, m.group(1)); i += 1; continue
        if re.match(r"^(Tabla|Figura)\s+\d+[.:]", s):
            pie_figura(doc, s); i += 1; continue
        if s.startswith("- "):
            lista(doc, "●", s[2:]); i += 1; continue
        m = re.match(r"^(\d+)\.\s+(.*)$", s)
        if m:
            lista(doc, m.group(1) + ".", m.group(2)); i += 1; continue
        # parrafo (une lineas seguidas)
        par = [s]; i += 1
        while i < len(lineas) and lineas[i].strip() and not re.match(r"^(#|- |\d+\. |\||```|!\[)", lineas[i].strip()):
            par.append(lineas[i].strip()); i += 1
        p = doc.add_paragraph()
        texto_inline(p, " ".join(par))
    doc.save(salida)
    print("DOCX:", salida)


if __name__ == "__main__":
    convertir(sys.argv[1], sys.argv[2])
