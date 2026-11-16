"""Construye la presentacion de la P2 reutilizando la de la P1 como plantilla."""
import sys
from copy import deepcopy
from pptx import Presentation
from pptx.chart.data import CategoryChartData
from pptx.dml.color import RGBColor
from pptx.oxml.ns import qn
from pptx.util import Inches

SRC, OUT, DIR = sys.argv[1], sys.argv[2], sys.argv[3]
prs = Presentation(SRC)
S = {i + 1: s for i, s in enumerate(prs.slides)}  # diapositivas de la P1 por numero

NAVY, ORANGE, GRAY = RGBColor(0x22, 0x3D, 0x74), RGBColor(0xE0, 0x76, 0x1F), RGBColor(0x6B, 0x74, 0x84)
FECHA = "23/11/2026"


# ---------- utilidades de texto (conservan el formato de la plantilla) ----------
def _fill_txbody(txBody, paras):
    """paras: lista de parrafos. Cada parrafo: str, o (idx_plantilla, contenido),
    donde contenido es str o lista de (texto, idx_run_plantilla)."""
    ps = txBody.findall(qn("a:p"))
    tpls = [deepcopy(p) for p in ps]
    for p in ps:
        txBody.remove(p)
    for i, para in enumerate(paras):
        if isinstance(para, tuple) and isinstance(para[0], int):
            ti, content = para
        else:
            ti, content = min(i, len(tpls) - 1), para
        p = deepcopy(tpls[ti])
        runs = [deepcopy(r) for r in p.findall(qn("a:r"))]
        if not runs:
            r = p.makeelement(qn("a:r"), {})
            end = p.find(qn("a:endParaRPr"))
            rpr = deepcopy(end) if end is not None else p.makeelement(qn("a:rPr"), {})
            rpr.tag = qn("a:rPr")
            r.append(rpr)
            t = r.makeelement(qn("a:t"), {})
            r.append(t)
            runs = [r]
        for el in list(p):
            if el.tag in (qn("a:r"), qn("a:br"), qn("a:fld")):
                p.remove(el)
        items = [(content, 0)] if isinstance(content, str) else content
        end = p.find(qn("a:endParaRPr"))
        for text, ri in items:
            r = deepcopy(runs[min(ri, len(runs) - 1)])
            r.find(qn("a:t")).text = text
            if end is not None:
                end.addprevious(r)
            else:
                p.append(r)
        txBody.append(p)


def put(shape, *paras):
    _fill_txbody(shape.text_frame._txBody, list(paras))


def cell(c, text):
    _fill_txbody(c._tc.txBody, [text])


def sh(slide, name):
    for s in slide.shapes:
        if s.name == name:
            return s
    for s in slide.shapes:
        if s.name.startswith(name):
            return s
    raise KeyError(name)


def title(slide, text):
    put(slide.shapes.title, text)


def borrar(shape):
    el = shape._element
    el.getparent().remove(el)


def notas(slide, text):
    slide.notes_slide.notes_text_frame.text = text


def tabla(shape, filas):
    tbl = shape.table
    trs = tbl._tbl.findall(qn("a:tr"))
    for tr in trs[len(filas):]:
        tbl._tbl.remove(tr)
    for r, fila in enumerate(filas):
        for c, txt in enumerate(fila):
            cell(tbl.cell(r, c), txt)


def mover(shape, x=None, y=None, w=None, h=None):
    if x is not None: shape.left = Inches(x)
    if y is not None: shape.top = Inches(y)
    if w is not None: shape.width = Inches(w)
    if h is not None: shape.height = Inches(h)


# ---------- pie y portada en todas las diapositivas ----------
for s in prs.slides:
    for shp in s.shapes:
        if not shp.has_text_frame:
            continue
        for p in shp.text_frame.paragraphs:
            full = "".join(r.text for r in p.runs)
            nuevo = (full.replace("Reversi 8 en ARM7TDMI", "Reversi 8 en la placa S3CEV40")
                     .replace("Práctica 1", "Práctica 2").replace("19/10/2026", FECHA))
            if nuevo != full and p.runs:
                p.runs[0].text = nuevo
                for r in p.runs[1:]:
                    r.text = ""


def anchos(shape, ws, izquierda=()):
    from pptx.enum.text import PP_ALIGN
    tbl = shape.table
    for i, w in enumerate(ws):
        tbl.columns[i].width = Inches(w)
    shape.width = Inches(sum(ws))
    for r in range(len(tbl.rows)):
        for c in izquierda:
            for p in tbl.cell(r, c).text_frame.paragraphs:
                p.alignment = PP_ALIGN.LEFT

# 1. Portada
notas(S[1], "SERGIO (0:25). Nos presentamos. Práctica 2: el Reversi de la P1 se juega ya desde la placa, con los pulsadores y el display.")

# 2. Lo que hemos conseguido
s = S[2]
title(s, "Lo que hemos conseguido")
put(sh(s, "Resultado 1 cifra"), "30")
put(sh(s, "Resultado 1 etique"), "jugadas en la placa")
put(sh(s, "Resultado 1 texto"), "Partida completa: 31 blancas y 33 negras, igual casilla a casilla que la simulación")
put(sh(s, "Resultado 2 cifra"), "10/10")
put(sh(s, "Resultado 2 etique"), "pulsaciones limpias")
put(sh(s, "Resultado 2 texto"), "Con el antirrebotes: 10 avisos al pulsar, 10 al soltar y el pulso espurio descartado")
put(sh(s, "Resultado 3 cifra"), "3")
put(sh(s, "Resultado 3 etique"), "placas medidas")
put(sh(s, "Resultado 3 texto"), "Ráfagas de rebote de 41,5 a 67,7 ms: trp y trd acaban en 100 ms")
put(sh(s, "Logro 1"), "Toda la E/S por interrupciones, con una única inicialización de periféricos")
put(sh(s, "Logro 2"), "Opcional hecho y probado en placa: teclado matricial con la misma máquina antirrebotes")
put(sh(s, "Logro 3"), "Destapado y corregido un fallo que venía de la P1: char sin signo en voltear")
notas(s, "PABLO (0:40). 30 jugadas en placa, 10 de 10 pulsaciones limpias, rebotes medidos en 3 placas.")

# 3. Qué hemos hecho
s = S[3]
title(s, "Qué hemos hecho")
put(sh(s, "Contexto"), [("Partimos del Reversi de la P1 (", 0), ("patron_volteo_arm", 1), (" + ", 0),
                        ("color_ficha_thumb", 1), (") y lo hacemos jugable en la placa: la jugada se elige con los pulsadores y el display, sin depurador.", 0)])
put(sh(s, "Entorno texto"), "Entorno de trabajo", "Placa Embest S3CEV40 (S3C44B0X)",
    "Pulsadores EINT6/7, 8led y leds", "timer0, timer1 y timer2", "Eclipse + GCC 2011.03 + openOCD/JTAG")
put(sh(s, "Tarea 1 texto"), "Gestión de la E/S", "Arreglos de E/S e inicialización centralizada de los periféricos")
put(sh(s, "Tarea 2 texto"), "Pila de depuración", "150 eventos con la marca de tiempo del timer2, en dirección fija")
put(sh(s, "Tarea 3 texto"), "Antirrebotes", "Máquina de estados con el timer0: trp, encuesta cada 50 ms y trd")
put(sh(s, "Tarea 4 texto"), "Entrada de la jugada", "Botones y 8led con autoincremento (timer1) y, como opcional, el teclado matricial")
notas(s, "SERGIO (0:50). Punto de partida, entorno y las cuatro tareas.")

# 4. Gestión de la E/S (plantilla: optimizaciones, P1 s5)
s = S[5]
title(s, "Gestión de la E/S: cuatro arreglos")
put(sh(s, "Decisión 1 título"), "I_ISPC es de solo escritura")
put(sh(s, "Decisión 1 texto"), "Con |= se escribe lo leído, que es indefinido, y se pueden borrar peticiones de otras líneas.")
put(sh(s, "Decisión 1 código"), "rI_ISPC = BIT_EINT4567;")
put(sh(s, "Decisión 2 título"), "Solo los bits propios")
put(sh(s, "Decisión 2 texto"), "timer_init paraba el timer2 y Eint4567_init reconfiguraba todo el puerto G.")
put(sh(s, "Decisión 2 código"), "reg = (reg & ~mascara) | valor;")
put(sh(s, "Decisión 3 título"), "Cada bit de EXTINTPND")
put(sh(s, "Decisión 3 texto"), "El switch por igualdad no atendía EINT6 y EINT7 a la vez. Ahora se mira cada bit y se borra solo lo atendido.")
put(sh(s, "Decisión 3 código"), "if (pnd & PND_IZQ) ...")
put(sh(s, "Decisión 4 título"), "Una sola inicialización")
put(sh(s, "Decisión 4 texto"), "inicializar_perifericos() tras sys_init(). Los botones, lo último: desde ahí ya pueden llegar IRQ.")
put(sh(s, "Decisión 4 código"), "timer2 → leds → 8led → timers → EINT")
notas(s, "PABLO (1:00). Los cuatro arreglos de E/S del paso 2 y el orden de la inicialización.")

# 5. Pila de depuración (plantilla: timer2, P1 s9)
s = S[9]
title(s, "Pila de depuración: 150 eventos")
put(sh(s, "timer2 configuraci"), "Formato de cada evento",
    "id del evento en una palabra + instante de timer2_leer() en µs: 8 B por evento",
    "Dirección fija con #define PILA_DEP_BASE: siempre en el mismo sitio de la vista Memory")
put(sh(s, "Fórmula"), "0x0C7FE000 – 0x0C7FE4AF")
put(sh(s, "Fórmula nota"), "Índice static volatile dentro de lista.c. Sin exclusión mutua: solo la llaman RSI, que no se anidan.")
tabla(sh(s, "Tabla calibración"), [
    ["Momento", "indice", "Entrada 0", "Fuera"],
    ["Tras inicializar", "0", "{0, 0}", "DEADBEEF"],
    ["150 eventos", "0", "{10, 1000}", "DEADBEEF"],
    ["Evento 151", "1", "{11, 151000}", "DEADBEEF"],
])
anchos(sh(s, "Tabla calibración"), [1.35, 0.65, 1.25, 1.15], izquierda=(0,))
put(sh(s, "Calibración conclu"), "1200 B",
    "El evento 151 pisa al más antiguo y la palabra siguiente sigue a 0xDEADBEEF: no se escribe fuera")
notas(s, "SERGIO (1:00). Formato, dirección fija, prueba de la vuelta en el simulador.")

# 6. Antirrebotes (plantilla: Thumb, P1 s13)
s = S[13]
title(s, "Antirrebotes: máquina de estados")
for n in ("Bus título", "ARM etiqueta", "ARM acceso 1", "ARM acceso 1 texto", "ARM acceso 2", "ARM acceso 2 texto",
          "Thumb etiqueta", "Thumb acceso 1", "Thumb acceso 1 texto", "Bus nota"):
    borrar(sh(s, n))
s.shapes.add_picture(f"{DIR}/diag_antirrebotes.png", Inches(0.5), Inches(1.32), width=Inches(5.3))
put(sh(s, "Datos texto"), "Medido en la placa 4", "aviso de pulsado", "60,0 ms", "encuesta y trd", "50,1 ms")
put(sh(s, "Veneer texto"), "El orden de T5 importa",
    "Primero EXTINTPND, luego I_ISPC y solo después se desenmascara: si no, un rebote de la suelta da una IRQ falsa.")
hip = sh(s, "Hipótesis texto")
put(hip, "Pulso espurio sin perder nada",
    "Si al acabar trp el botón ya está a 1, no se avisa: pasa por trd y el siguiente flanco abre otro ciclo.")
hip.text_frame.paragraphs[0].runs[0].font.color.rgb = NAVY
notas(s, "PABLO (1:15). Estados y transiciones T1-T5; un solo timer0 para todas las entradas.")

# 7. Rebotes por placa (plantilla: resultados 6 combinaciones, P1 s12)
s = S[12]
title(s, "Rebotes: cada placa es distinta")
ch = sh(s, "Gráfico").chart
cd = CategoryChartData()
cd.categories = ["P12 pulsar", "P12 soltar", "P4 pulsar", "P4 soltar", "P7 pulsar", "P7 soltar"]
cd.add_series("Ráfaga (ms)", (41.5, 12.4, 28.5, 38.2, 67.7, 59.3))
ch.replace_data(cd)
_fill_txbody(ch.chart_title.text_frame._txBody, ["Ráfaga de rebotes más larga por placa (ms)"])
for i, pt in enumerate(ch.plots[0].series[0].points):
    pt.format.fill.solid()
    pt.format.fill.fore_color.rgb = ORANGE if i == 4 else (NAVY if i % 2 == 0 else GRAY)
va = ch.value_axis
va.maximum_scale, va.minimum_scale, va.major_unit = 80, 0, 20
dl = ch.plots[0].data_labels
dl.number_format, dl.number_format_is_linked = "0.0", False
for nf in ch._chartSpace.iter(qn("c:numFmt")):
    if nf.getparent().tag in (qn("c:dLbls"), qn("c:dLbl")):
        nf.set("formatCode", "0.0"); nf.set("sourceLinked", "0")
for ser in ch._chartSpace.iter(qn("c:ser")):
    for dls in ser.findall(qn("c:dLbls")):
        if dls.find(qn("c:numFmt")) is None:
            nf = dls.makeelement(qn("c:numFmt"), {"formatCode": "0.0", "sourceLinked": "0"})
            dls.insert(0, nf)
put(sh(s, "Dato tiempo"), "67,7 ms", "la peor ráfaga, en la placa 7 (casos sueltos de hasta 119,6 ms)")
put(sh(s, "Dato tamaño"), "100 ms", "trp = trd: cubren todas las ráfagas de la placa 7")
put(sh(s, "Dato ns"), "Coste", "los toques de menos de 100 ms se descartan: de 53, 67, 80 y 116 ms solo pasa el último")
put(sh(s, "Leyenda"), "Medido con TEST_REBOTES = 1: la RSI solo apila un evento por flanco")
notas(s, "SERGIO (1:10). Código de test, tres placas, por qué 100 ms y lo que cuesta.")

# 8. Entrada de la jugada (plantilla: ensamblador frente al compilador, P1 s14)
s = S[14]
title(s, "Entrada de la jugada: máquina de estados")
g = sh(s, "Gráfico")
rid = g._element.find(".//" + qn("c:chart")).get(qn("r:id"))
borrar(g)
s.part.drop_rel(rid)
s.shapes.add_picture(f"{DIR}/diag_entrada.png", Inches(0.6), Inches(1.4), width=Inches(8.8))
mover(sh(s, "Dato O3"), x=0.5, y=5.0, w=2.9, h=1.25)
put(sh(s, "Dato O3"), "0,5 s", "pulsación larga: desde ahí, autoincremento cada 300 ms (timer1)")
os_ = sh(s, "Dato Os")
os_._element.replace(os_._element.find(qn("p:txBody")), deepcopy(sh(s, "Dato O3")._element.find(qn("p:txBody"))))
mover(sh(s, "Dato Os"), x=3.55, y=5.0, w=2.9, h=1.25)
put(sh(s, "Dato Os"), "3 s", "de parpadeo: 18 conmutaciones de 167 ms; cualquier botón cancela")
mover(sh(s, "Aviso tarjeta"), x=6.6, y=4.95, w=2.9, h=1.35)
mover(sh(s, "Aviso"), x=6.75, y=5.0, w=2.6, h=1.25)
put(sh(s, "Aviso"), "Sin variables compartidas", "reversi8() pide la jugada con entrada_jugada_lista()")
sh(s, "Aviso").text_frame.paragraphs[0].runs[0].font.color.rgb = NAVY
notas(s, "PABLO (1:00). INICIO, FILA, COLUMNA, CONFIRMAR; timer1 para autoincremento y parpadeo.")

# 9. Así se introduce una jugada (P1 s11)
s = S[11]
title(s, "Así se introduce una jugada")
put(sh(s, "Paso 1 texto"), "Empezar", "Cualquier botón con el display apagado: aparece F (fila)")
put(sh(s, "Paso 2 texto"), "Elegir la fila", "Izquierdo: 1, 2… 8 y vuelta a 1; mantenido, autoincremento. Derecho: confirma y aparece C")
put(sh(s, "Paso 3 texto"), "Elegir la columna", "Igual que la fila. Al confirmar, el led izquierdo parpadea 3 veces por segundo")
put(sh(s, "Paso 4 texto"), "Confirmar o cancelar", "3 s sin tocar nada: la jugada pasa a reversi8 (id 7). Cualquier botón la cancela (id 8)")
put(sh(s, "Esperado título"), "Qué tiene que verse")
put(sh(s, "Esperado subtítulo"), "tablero tras fila 3, columna 4:")
put(sh(s, "Esperado tablero"), [("fila 2  ", 0), ("00 00 01 02 00 00 00 00", 1)],
    [("fila 3  ", 0), ("00 00 00 01 02 00 00 00", 1)], [("fila 4  ", 0), ("00 00 00 02 01 00 00 00", 1)])
put(sh(s, "Esperado leyenda"), "02 = negra en [2][3]; la máquina responde en [2][2]")
put(sh(s, "Esperado tiempo"), "pila de depuración", "id 7 a 3,006 s")
put(sh(s, "Esperado comparaci"), "18 tics del timer1 de 167,008 ms: el parpadeo dura lo que debe")
notas(s, "SERGIO (0:50). Demostración de una jugada: F, número, C, número, parpadeo.")

# 10. Cómo verificamos (P1 s8). El tablero de la P1 coincide con el de la jugada [2][3] de la P2
s = S[8]
title(s, "Cómo verificamos que funciona")
put(sh(s, "Tablero pie"), "Tras la negra en [2][3] (fila 3, columna 4 en el display) y la respuesta blanca en [2][2]")
tabla(sh(s, "Tabla verificación"), [
    ["Prueba", "Esperado", "Resultado"],
    ["Botón en INICIO", "Muestra F", "Correcto"],
    ["Izquierdo varias veces", "1…8 y vuelta a 1", "Correcto"],
    ["Izquierdo mantenido", "Autoincremento", "Correcto"],
    ["Derecho con número", "F → C → parpadeo", "Correcto"],
    ["Esperar 3 s", "Jugada hecha (id 7)", "Correcto"],
    ["Botón al parpadear", "Cancelada (id 8)", "Correcto"],
])
anchos(sh(s, "Tabla verificación"), [1.85, 1.95, 1.1], izquierda=(0, 1, 2))
put(sh(s, "Método texto"), "Partida completa",
    "30 jugadas comparadas con una simulación en Python tras la 10, la 20 y la 30: 0 diferencias. Tablero lleno y E en el display")
notas(s, "PABLO (0:50). Pruebas de la entrada y partida completa contra la simulación.")

# 11. Marco de pila de la RSI (P1 s6)
s = S[6]
title(s, "Marco de pila de la RSI de los botones")
pila = [("[fp,#8]", "cima de la pila IRQ"), ("[fp,#4]", "r12 original (push {ip})"), ("[fp]", "pc"),
        ("[fp,#-4]", "lr_irq"), ("[fp,#-8]", "ip = sp de entrada"), ("[fp,#-12]", "fp anterior"),
        ("[fp,#-16]", "r3"), ("[fp,#-20]", "r2"), ("[fp,#-24]", "pnd (local)  ← sp")]
desp = [x for x in s.shapes if x.name.startswith("Pila desplazamiento")]
for i, (d, t) in enumerate(pila):
    put(desp[i], d)
    put(sh(s, f"Pila texto {i}"), t)
put(sh(s, "Prólogo código"), "Eint4567_ISR (interrupt(\"IRQ\"))", "push  {ip}", "mov   ip, sp",
    "push  {r2, r3, fp, ip, lr, pc}", "sub   fp, ip, #4", "...", "subs  pc, lr, #4")
put(sh(s, "Pila explicación"), "36 B de los 256 B de la pila IRQ",
    "Simulador con la entrada forzada (CPSR = 0xD2) y r2, r3, r11 y r12 marcados: al volver, todos recuperan su valor.",
    "Guarda r2, r3 y r12 porque la IRQ llega en cualquier instrucción. En la versión final llama a funciones: estimamos ~130 B, sin medir en placa.")
notas(s, "SERGIO (1:15). Marco de la RSI frente al de una función normal; pila IRQ.")

# 12. Rutina de interrupción (P1 s10)
s = S[10]
title(s, "Rutina de interrupción de los botones")
put(sh(s, "ISR código"), "Generado por GCC con interrupt(\"IRQ\")", "push  {ip}", "mov   ip, sp",
    "push  {r2, r3, fp, ip, lr, pc}", "sub   fp, ip, #4",
    (5, [("...   ", 0), ("/* pnd = EXTINTPND; apilar; borrar */", 1)]),
    "sub   sp, fp, #20", "ldm   sp, {r2, r3, fp, sp, lr}", "pop   {ip}", "subs  pc, lr, #4")
put(sh(s, "ISR diferencias"), "Frente a una función normal",
    "• Guarda también r2, r3 e ip: la IRQ llega en cualquier instrucción",
    "• lr_irq = instrucción interrumpida + 4: se vuelve con lr − 4",
    "• subs pc copia SPSR en CPSR: vuelve a SVC y reactiva las IRQ",
    "• No guarda el SPSR: las RSI no se anidan")
put(sh(s, "Camino título"), "Camino de la interrupción (EINT6 y EINT7 comparten la línea EINT4567)")
for i, t in enumerate(["Vector IRQ 0x18", "Controlador vectorizado", "HandlerEINT4567", "pISR_EINT4567", "Eint4567_ISR"]):
    put(sh(s, f"Camino {i} texto"), t)
put(sh(s, "ISPC nota"), "Las RSI no comparten variables con Main: se comunican por funciones (boton_timer0, entrada_boton_pulsado…)")
notas(s, "PABLO (1:00). Camino de la IRQ y en qué se diferencia la RSI de una función normal.")

# 13. Problemas (P1 s15)
s = S[15]
title(s, "Problemas encontrados")
prob = [("char sin signo en voltear", "Fallo de la P1: −1 valía 255 y no volteaba hacia arriba ni a la izquierda. Lo destapó la partida completa.",
         "→ signed char en los desplazamientos"),
        ("La placa 7 rebota mucho más", "Con los tiempos de otras placas daba pulsaciones dobles o perdidas.",
         "→ medir otra vez y subir a 100 ms"),
        ("Placa ejecutando basura", "Antes de Main, PC en timer2_parar (que nunca se llama) y marco a 0xea000052.",
         "→ apagar, encender y volver a cargar"),
        ("Tablero lleno sin final", "Si el humano ponía la última ficha, el programa esperaba una jugada 31.",
         "→ contador vacias y break antes de elegir_mov")]
for i, (t, x, so) in enumerate(prob, 1):
    put(sh(s, f"Problema {i} título"), t)
    put(sh(s, f"Problema {i} texto"), x)
    put(sh(s, f"Problema {i} solució"), so)
notas(s, "SERGIO (1:00). Cuatro problemas; el de voltear venía de la P1.")

# 14. Conclusiones (P1 s16)
s = S[16]
put(sh(s, "Puntos fuertes lis"), "• Partida completa en placa, igual que la simulación",
    "• Toda la E/S por interrupciones, sin variables compartidas", "• La pila de depuración permite medir, no suponer")
put(sh(s, "Puntos débiles lis"), "• trp = 100 ms descarta toques cortos", "• Pila IRQ sin medir (~130 B estimados)",
    "• Sin «paso»: no se puede pasar turno")
put(sh(s, "Mejoras lista"), "• Dos lecturas a 1 antes de dar el botón por soltado", "• Medir la pila IRQ con 0xDEADBEEF",
    "• Mostrar el ganador al acabar")
put(sh(s, "Mensaje final"), "Con hardware real los tiempos dependen de la placa: hay que medirlos. La pila de depuración nos dejó medir en vez de suponer, y encontró un fallo que venía de la P1.")
notas(s, "PABLO (0:40). Fuertes, débiles, mejoras y mensaje final. Preguntas.")

# 15. Respaldo (P1 s17)
put(sh(S[17], "Respaldo subtítulo"), "Código, rebotes por placa y otros problemas para las preguntas")

# 16. Código (P1 s18)
s = S[18]
title(s, "Código: RSI de los botones y antirrebotes")
izq = ["Eint4567_ISR y T1 · button.c",
       "static void empezar_retardo_trp(int b,",
       "                                uint8_t ev)",
       "{",
       [("  boton_activo = b;", 0)],
       [("  rINTMSK |= LINEAS_ENTRADA;  ", 0), ("/* T1 */", 1)],
       "  apilar(ev);",
       "  estado = RETARDO_TRP;",
       "  timer0_programar(TRP_MS);",
       "}",
       "void Eint4567_ISR(void)",
       "{",
       "  unsigned int pnd = rEXTINTPND",
       "                   & (PND_IZQ | PND_DER);",
       "  if (pnd && estado == ESPERANDO) {",
       "    if (pnd & PND_IZQ)",
       "      empezar_retardo_trp(BOTON_IZQ, ...);",
       "    else",
       "      empezar_retardo_trp(BOTON_DER, ...);",
       "  }",
       [("  rEXTINTPND = pnd;      ", 0), ("/* lo atendido */", 1)],
       [("  rI_ISPC = BIT_EINT4567;", 0)],
       "}"]
der = ["boton_timer0 · RSI del timer0",
       "case RETARDO_TRP:",
       [("  if (boton_pulsado()) {     ", 0), ("/* T2a */", 1)],
       "    estado = ENCUESTA;",
       "    timer0_programar(ENCUESTA_MS);",
       "    entrada_boton_pulsado(boton);",
       [("  } else {                   ", 0), ("/* T2b */", 1)],
       "    estado = RETARDO_TRD;",
       "    timer0_programar(TRD_MS);",
       "  }  break;",
       "case ENCUESTA:",
       [("  if (boton_pulsado())       ", 0), ("/* T3 */", 1)],
       "    timer0_programar(ENCUESTA_MS);",
       [("  else {                     ", 0), ("/* T4 */", 1)],
       "    estado = RETARDO_TRD;",
       "    timer0_programar(TRD_MS);",
       "    entrada_boton_soltado(boton);",
       "  }  break;",
       [("case RETARDO_TRD:            ", 0), ("/* T5 */", 1)],
       "  estado = ESPERANDO;",
       "  rEXTINTPND = PND_IZQ | PND_DER;",
       "  rI_ISPC    = LINEAS_ENTRADA;",
       "  rINTMSK   &= ~LINEAS_ENTRADA;"]


def codigo(shape, lineas):
    # plantilla de linea de codigo: el primer parrafo con dos runs (codigo + comentario)
    k = next(i for i, p in enumerate(shape.text_frame.paragraphs) if len(p.runs) >= 2)
    paras = [(0, lineas[0])]
    for l in lineas[1:]:
        paras.append((k, l if isinstance(l, list) else [(l, 0)]))
    put(shape, *paras)


codigo(sh(s, "ARM código"), izq)
codigo(sh(s, "Thumb código"), der)
notas(s, "Respaldo: solo si preguntan.")

# 17. Tabla de rebotes (P1 s21)
s = S[21]
title(s, "Tabla completa: rebotes por placa")
tabla(sh(s, "Tabla combinaciones"), [
    ["Placa", "Muestras", "Pulsar (peor)", "Soltar (peor)", "Caso suelto", "trp / trd", "Resultado"],
    ["12 (3-nov)", "Pocas pulsaciones", "41,5 ms", "12,4 ms", "—", "60 / 30 ms", "Entrada OK"],
    ["4 (9-nov)", "4 × 1 s + toque", "28,5 ms", "38,2 ms", "93,7 ms", "60 / 50 ms", "10/10 pulsaciones"],
    ["7 (10-nov)", "15 × 1 s + 5 toques", "67,7 ms", "59,3 ms", "119,6 ms", "100 / 100 ms", "60/50 fallaba"],
])
anchos(sh(s, "Tabla combinaciones"), [1.15, 1.85, 1.2, 1.2, 1.1, 1.3, 1.2], izquierda=(0, 1))
mover(sh(s, "Tabla nota"), y=3.55)
put(sh(s, "Tabla nota"),
    "Ráfaga = del primer al último evento de un flanco (separados menos de 30 ms), leída de la pila con TEST_REBOTES = 1. Volcados de la placa 7 analizados con pruebas/rafagas.py.",
    "Con el antirrebotes activo (placa 4): aviso de pulsado a 60,0 ms, encuesta cada 50,1 ms y trd de 50,1 ms, medidos con timer2.")
notas(s, "Respaldo: solo si preguntan.")

# 18. Teclado (P1 s20)
s = S[20]
title(s, "Opcional: teclado matricial, probado en placa")
tabla(sh(s, "Tabla CPSR"), [
    ["Tecla", "En FILA / COLUMNA", "En CONFIRMAR", "Evento"],
    ["1 – 8", "Elige el valor con una pulsación", "Se ignora", "id 9"],
    ["OK", "Confirma (como el botón derecho)", "Se ignora", "id 9"],
    ["NO", "Vuelve a pedir la fila", "Cancela la jugada", "id 9 y 8"],
])
anchos(sh(s, "Tabla CPSR"), [1.2, 3.5, 2.4, 1.9], izquierda=(0, 1, 2, 3))
put(sh(s, "Veneer texto"), "Misma máquina antirrebotes",
    "Banco 3 (0x0600_0000): al leer 0x060000FD/FB/F7/EF se pone a 0 una fila y se leen las columnas en D3-D0. Cualquier tecla da un flanco en EINT1.",
    "La RSI de EINT1 solo entra en la máquina; la tecla se escanea al acabar trp, ya sin rebotes. Mientras tanto, todas las entradas enmascaradas.",
    "Probado en placa (16-nov): jugada completa solo con el teclado; NO cancela, y OK durante el parpadeo y las teclas sin función se ignoran.")
notas(s, "Respaldo: solo si preguntan.")

# 19. Otros problemas (P1 s24)
put(sh(S[24], "Otros problemas"),
    "• rI_ISPC |= … leía basura: todas las RSI pasan a rI_ISPC = BIT_x",
    "• timer_init paraba el timer2: lectura-máscara-escritura sobre sus bits",
    "• El timer2 quedaba parado: timer2_empezar() en la inicialización",
    "• Main no se ejecuta en el simulador: la inicialización se prueba en placa",
    "• La RSI no salta en el simulador: entrada forzada desde GDB (CPSR, sp, lr, pc)",
    "• El contador de vacías restaba con jugadas sobre casillas ocupadas",
    "• undefined reference a entrada_*: faltaba entrada.c en el proyecto",
    "• OK y NO estaban en la tercera fila del teclado: movidos a la última, como en el guion")
notas(S[24], "Respaldo: solo si preguntan.")


# ---------- diapositivas nuevas del opcional: teclado y traza real de la pila ----------
from pptx.enum.shapes import MSO_SHAPE
from pptx.util import Pt
from pptx.enum.text import PP_ALIGN, MSO_ANCHOR
SRCP = Presentation(SRC)          # P1 sin tocar, para copiar formas con su formato
SO = {i + 1: x for i, x in enumerate(SRCP.slides)}


def nueva(base):
    """Diapositiva nueva con el diseno, el titulo y el pie de la diapositiva `base` (ya convertida)."""
    sl = prs.slides.add_slide(S[base].slide_layout)
    for x in list(sl.shapes):
        borrar(x)
    for x in S[base].shapes:
        if x.is_placeholder or x.name.startswith("Google Shape;315"):
            sl.shapes._spTree.append(deepcopy(x._element))
    return sl


def copiar(sl, origen, nombre, x=None, y=None, w=None, h=None):
    el = deepcopy(sh(origen, nombre)._element)
    sl.shapes._spTree.append(el)
    nueva_forma = sl.shapes[-1]
    mover(nueva_forma, x, y, w, h)
    return nueva_forma


def tabla_a_medida(shape, ncols, filas, anchos_in, plantilla_filas=(1, 2), destacar=None, fila_destacada=None,
                   alto_fila=None):
    """Recorta/amplia una tabla copiada a ncols columnas y len(filas) filas."""
    tbl = shape.table._tbl
    grid = tbl.tblGrid
    for gc in grid.findall(qn("a:gridCol"))[ncols:]:
        grid.remove(gc)
    trs = tbl.findall(qn("a:tr"))
    for tr in trs:
        for tc in tr.findall(qn("a:tc"))[ncols:]:
            tr.remove(tc)
    trs = tbl.findall(qn("a:tr"))
    plantillas = [deepcopy(trs[i]) for i in plantilla_filas]
    destacada = deepcopy(trs[fila_destacada]) if fila_destacada is not None else None
    for tr in trs[1:]:
        tbl.remove(tr)
    for i in range(1, len(filas)):
        tpl = destacada if (destacar is not None and i == destacar) else plantillas[(i - 1) % len(plantillas)]
        tbl.append(deepcopy(tpl))
    tabla(shape, filas)
    anchos(shape, anchos_in, izquierda=tuple(range(ncols)))
    if alto_fila:
        for tr in tbl.findall(qn("a:tr")):
            tr.set("h", str(Inches(alto_fila)))
            for tc in tr.findall(qn("a:tc")):
                tcPr = tc.find(qn("a:tcPr"))
                if tcPr is not None:
                    tcPr.set("marT", "36000"); tcPr.set("marB", "36000")


def ids_unicos(sl):
    for n, el in enumerate(sl.shapes._spTree.iter(qn("p:cNvPr")), start=2):
        el.set("id", str(n))


# A. Opcional: teclado matricial
A = nueva(8)
title(A, "Opcional: teclado matricial")
teclas = [["1", "2", "3", "4"], ["5", "6", "7", "8"], ["", "", "", ""], ["OK", "", "", "NO"]]
for f in range(4):
    for c in range(4):
        t = teclas[f][c]
        caja = A.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, Inches(0.6 + c * 0.88), Inches(1.5 + f * 0.88),
                                  Inches(0.78), Inches(0.78))
        caja.name = f"Tecla {f},{c}"
        caja.adjustments[0] = 0.18
        caja.line.fill.background()
        caja.shadow.inherit = False
        caja.fill.solid()
        caja.fill.fore_color.rgb = NAVY if t.isdigit() else (ORANGE if t else RGBColor(0xE9, 0xEE, 0xF6))
        tf = caja.text_frame
        tf.vertical_anchor = MSO_ANCHOR.MIDDLE
        p = tf.paragraphs[0]
        p.alignment = PP_ALIGN.CENTER
        r = p.add_run()
        r.text = t
        r.font.name, r.font.size, r.font.bold = "Calibri", Pt(20), True
        r.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)
pie_t = copiar(A, SO[8], "Tablero pie", x=0.5, y=5.05, w=3.75, h=1.1)
put(pie_t, "10 teclas habilitadas (figura 2 del guion). Fila de arriba = 0xFD y columna izquierda = dato 0x7 (tabla 6.1 de Entradasalida.pdf)")
tt = copiar(A, SO[20], "Tabla CPSR", x=4.45, y=1.45)
tabla_a_medida(tt, 3, [["Tecla", "En FILA / COLUMNA", "En CONFIRMAR"],
                       ["1 – 8", "Elige el valor", "Se ignora"],
                       ["OK", "Confirma (botón derecho)", "Se ignora"],
                       ["NO", "Vuelve a pedir la fila", "Cancela la jugada"]],
               [0.9, 2.3, 1.85], plantilla_filas=(1,))
copiar(A, SO[13], "Veneer tarjeta", x=4.45, y=3.55, w=5.05, h=1.6)
mt = copiar(A, SO[13], "Veneer texto", x=4.6, y=3.6, w=4.8, h=1.5)
put(mt, "Misma máquina antirrebotes",
    "La RSI de EINT1 solo entra en la máquina (id 9). La tecla se escanea al acabar trp, ya sin rebotes; las no habilitadas cuentan como «no pulsada» y se descartan solas (T2b).")
st = copiar(A, SO[12], "Dato tiempo", x=4.45, y=5.3, w=5.05, h=1.0)
put(st, "Probado en placa", "jugada completa solo con el teclado; NO cancela; OK al parpadear y las teclas sin función se ignoran")
ids_unicos(A)
notas(A, "PABLO (0:50). El opcional: 10 teclas, misma máquina antirrebotes, y probado en placa con una jugada completa.")

# B. Lo que guarda la pila: traza real de la prueba del teclado (NO durante el parpadeo)
B = nueva(8)
title(B, "Lo que guarda la pila: NO durante el parpadeo")
copiar(B, SO[10], "ISR caja", x=0.5, y=1.4, w=3.55, h=3.96)
raw = copiar(B, SO[10], "ISR código", x=0.6, y=1.3, w=3.4, h=4.0)
volcado = ["c7fe428: 00000009 0afbb5a4", "c7fe430: 00000006 0afd344c", "c7fe438: 00000004 0afd3c6c",
           "c7fe440: 00000008 0afd3cf5", "c7fe448: 00000004 0afe0094", "c7fe450: 00000004 0afec45e",
           "c7fe458: 00000004 0aff8027", "c7fe460: 00000004 0b004bf1", "c7fe468: 00000003 0b010fbc",
           "c7fe470: 00000005 0b02967f"]
put(raw, *(["Memoria en la placa (id | instante en µs)"] + [(1, l) for l in volcado]))
traza = copiar(B, SO[21], "Tabla combinaciones", x=4.25, y=1.4)
tabla_a_medida(traza, 3, [
    ["Δt (ms)", "Evento", "Qué ha pasado"],
    ["0", "9 · Teclado", "T1: tecla NO, entradas enmascaradas"],
    ["98,0", "6 · Timer1", "El led sigue parpadeando"],
    ["100,0", "4 · Timer0", "Fin de trp: la tecla sigue pulsada"],
    ["100,2", "8 · Cancelada", "entrada_tecla(NO) en CONFIRMAR"],
    ["150,3", "4 · Timer0", "Encuesta: sigue pulsada"],
    ["200,4", "4 · Timer0", "Encuesta"],
    ["248,5", "4 · Timer0", "Encuesta"],
    ["300,6", "4 · Timer0", "Encuesta"],
    ["350,7", "3 · Soltado", "T4: la tecla vuelve a 1"],
    ["450,8", "5 · Fin de trd", "T5: entradas reactivadas"],
], [0.8, 1.4, 3.05], plantilla_filas=(1, 2), destacar=4, fila_destacada=6, alto_fila=0.36)
for p_ in raw.text_frame.paragraphs:
    p_.line_spacing = Pt(25.9)
    p_.space_before = p_.space_after = Pt(0)
raw.text_frame.margin_top = Inches(0.06)
nota_b = copiar(B, SO[21], "Tabla nota", x=0.5, y=5.6, w=9.0, h=0.75)
put(nota_b, "Cada evento son 8 B: el id en una palabra y el instante de timer2_leer() en µs (0x0afbb5a4 = 184 268 196 µs). Se ven trp = 100 ms y la encuesta cada 50 ms medidos por la propia placa.")
ids_unicos(B)
notas(B, "SERGIO (0:50). Traza real leída de la placa al pulsar NO durante el parpadeo: cada transición de la máquina deja su evento con el instante.")

# ---------- orden final y diapositivas que sobran ----------
orden = [1, 2, 3, 5, 9, 13, 12, 14, 11, 25, 26, 8, 6, 10, 15, 16, 17, 18, 21, 24]
lst = prs.slides._sldIdLst
ids = list(lst)
for el in ids:
    lst.remove(el)
for n in orden:
    lst.append(ids[n - 1])
for n in set(range(1, len(ids) + 1)) - set(orden):
    prs.part.drop_rel(ids[n - 1].get(qn("r:id")))
prs.save(OUT)
print("OK", OUT, len(orden), "diapositivas")
