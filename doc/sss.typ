#import "@local/informe:1.0.5": conf
#import "@preview/theorion:0.6.0": *
#import cosmos.fancy: *
#show: show-theorion
#import "@preview/lilaq:0.5.0" as lq
#import "@preview/cetz:0.4.2": canvas, draw
#import "@preview/cetz-plot:0.1.2": plot
#import "@preview/showybox:2.0.4": showybox
#set math.equation(
  numbering: (..nums) => numbering("(1)", ..nums),
  supplement: none,
)
#set-inherited-levels(1)
#set-theorion-numbering("1.1")

// Gruvbox Dark palette
#let gb-bg = rgb("#282828")
#let gb-bg1 = rgb("#3c3836")
#let gb-bg2 = rgb("#504945")
#let gb-fg = rgb("#ebdbb2")
#let gb-gray = rgb("#928374")
#let gb-red = rgb("#fb4934")
#let gb-green = rgb("#b8bb26")
#let gb-yellow = rgb("#fabd2f")
#let gb-blue = rgb("#83a598")
#let gb-purple = rgb("#d3869b")
#let gb-aqua = rgb("#8ec07c")
#let gb-orange = rgb("#fe8019")

#let terminal(body) = showybox(
  frame: (
    border-color: gb-bg2,
    body-color: gb-bg,
    thickness: 1pt,
    radius: 6pt,
    inset: (x: 12pt, y: 10pt),
  ),
  {
    set text(
      font: "DejaVu Sans Mono",
      size: 8pt,
      fill: gb-fg,
    )
    set par(leading: 0.6em)
    body
  },
)

// Prompt: user@host:dir$
#let ps1(user: "user@host", dir: "~") = {
  text(fill: gb-green, weight: "bold", user)
  text(fill: gb-fg, ":")
  text(fill: gb-blue, weight: "bold", dir)
  text(fill: gb-orange, "$ ")
}

// A prompt followed by a command
#let cmd(user: "user@host", dir: "~", it) = {
  ps1(user: user, dir: dir)
  it
  linebreak()
}

// Plain output line
#let out(it) = {
  it
  linebreak()
}

// Dimmed / comment line
#let note(it) = {
  text(fill: gb-gray, it)
  linebreak()
}

// Block cursor
#let cursor = box(width: 5pt, height: 9pt, fill: gb-fg, baseline: 1pt)

#let colores = (
  rgb("#FFD6E0"),
  rgb("#FFE5B4"),
  rgb("#FFF9B0"),
  rgb("#C8F2C2"),
  rgb("#BDE7F5"),
  rgb("#D7CCF7"),
  rgb("#F9C9E8"),
  rgb("#CFF5E7"),
  rgb("#FBD5C0"),
  rgb("#D0E1FF"),
)

#let p(x) = (
  54321
    - 120 * x
    + 2544 * calc.pow(x, 2)
    - 329 * calc.pow(x, 3)
    + 85 * calc.pow(x, 4)
    - 18 * calc.pow(x, 5)
    + calc.pow(x, 6)
)

#let monigote(color) = {
  let trazo = 1.2pt + color
  box(width: 1.2cm, height: 1.8cm, {
    place(top + left, dx: 0.38cm, circle(radius: 0.22cm, stroke: trazo))
    place(top + left, dx: 0.6cm, dy: 0.44cm, line(
      end: (0cm, 0.7cm),
      stroke: trazo,
    ))
    place(top + left, dx: 0.2cm, dy: 0.64cm, line(
      end: (0.8cm, 0cm),
      stroke: trazo,
    ))
    place(top + left, dx: 0.6cm, dy: 1.14cm, line(
      end: (-0.3cm, 0.66cm),
      stroke: trazo,
    ))
    place(top + left, dx: 0.6cm, dy: 1.14cm, line(
      end: (0.3cm, 0.66cm),
      stroke: trazo,
    ))
  })
}

#show: conf.with(
  titulo: [Shamir's Secret Sharing],
  asignatura: [Fundamentos de Criptografía y Seguridad Informática],
  autores: (
    ("Rodrigo Alcántara Fernández", "rodrigo.alcantara@estudiante.uam.es"),
  ),
  // pareja: [1],
  // grupo: [2301],
  fecha: [1 de noviembre de 2026],
  //resumen: [
  //],
  // marca_de_agua: [Borrador]
)

= Introducción
El esquema de compartición de secretos es un protocolo definido en 1979 por Adi Shamir @shamir (uno de los inventores de RSA).
Busca dividir un secreto entre un grupo de personas de tal manera que se pueda recuperar si y solo si un subgrupo de tamaño prefijado de los integrantes del grupo se ponen de acuerdo.
Por ejemplo, se puede compartir entre un grupo de 10 personas y solo se puede recuperar el secreto si 7 de ellas así lo quieren.

= El polinomio interpolador de Lagrange<sec:interpol>

_Para desarrollar este apartado me he basado en los apuntes de la asignatura de_ Cálculo Numérico II @interpolacion.

Sea $KK$ un cuerpo (e.g. $RR$ o $ZZ_p$).
Sean $N in NN$, $x_0, x_1, ..., x_N in KK$ puntos distintos dos a dos y $y_0, y_1, ..., y_N in KK$.
El problema de interpolación de Lagrange consiste en buscar un polinomio $q(x) in KK[x]$ de grado menor o igual a $N$ tal que $q(x_i) = y_i quad forall i in {0, 1, ..., N}$.

#definition[Nodos de interpolación][A los puntos $x_i$ los llamamos "nodos de interpolación".]

#definition[Polinomios de Lagrange][
  Dada una colección ${x_i}_(i = 0)^N$ de nodos de interpolación, definimos el i-ésimo polinomio de Lagrange como:
  $
    l_i (x) = product_(k = 0,\ i != k)^N (x - x_k) / (x_i - x_k) = ((x - x_0)...(x - x_(i-1))(x - x_(i+1))...(x - x_N))/((x_i - x_0)...(x_i - x_(i-1))(x_i - x_(i+1))...(x_i - x_N)).
  $<eq:lagrange>
]

#remark[
  Los denominadores en @eq:lagrange nunca se anulan porque los nodos son distintos dos a dos.
]

#proposition[Valor de los polinomios de Lagrange en los nodos interpoladores][
  $
    l_i(x_j) = delta_(i,j) = cases(1 "si" i = j, 0 "si" i != j).
  $
]

#proof[
  #set math.equation(numbering: none)
  - $i = j$:
    $
      l_i(x_i) = product_(k = 0,\ i != k)^N (x_i - x_k) / (x_i - x_k) = product_(k = 0,\ i != k)^N 1 = 1. quad checkmark
    $

  - $i != j$: el numerador contiene el factor $(x_j - x_j) = 0$, por lo que el producto es 0:
    $
      l_i(x_j) &= product_(k = 0,\ i != k)^N (x_j - x_k) / (x_i - x_k) \
      &= ((x_j - x_0)...cancel((x_j - x_j))...(x_j - x_(i-1))(x_j - x_(i+1))...(x_j - x_N))/((x_i - x_0)...(x_i - x_(i-1))(x_i - x_(i+1))...(x_i - x_N)) \
      &= 0. quad checkmark
    $
]

#theorem[Polinomio Interpolador de Lagrange][
  Dados $N + 1$ nodos de interpolación y $N+1$ valores $y_0, y_1, ..., y_N$, el polinomio:
  $
    q(x) = sum_(i=0)^N l_i (x) y_i,
  $
  es un polinomio de grado menor o igual a $N$ tal que $q(x_i) = y_i quad forall i in {0,1,...,N}$.
]<the:exis>

#proof[
  #set math.equation(numbering: none)
  - Es un polinomio de grado menor o igual a $N$:\
    Está construido como la suma de polinomios Lagrange, que a su vez son el producto de $N$ factores lineales, luego el grado de $q(x)$ es, a lo sumo, $N$. #sym.checkmark
    #colbreak()
  - $q(x_i) = y_i$:\
    $
      q(x_i) &= sum_(j = 0)^N l_j (x_i)y_j \
      &= cancel(l_0 (x_i)) y_0 + ... + cancel(l_(i-1) (x_i)) y_(i-1) + l_i (x_i) y_i + cancel(l_(i+1) (x_i)) y_(i+1) + ... + cancel(l_N (x_i)) y_N\
      &= 0 dot y_0 + ... + 0 dot y_(i-1) + 1 dot y_i + 0 dot y_(i+1) + ... + 0 dot y_N\
      &= y_i. quad checkmark
    $
]

#lemma[
  Un polinomio $q(x)$ no nulo de grado menor o igual que $N$ sobre un cuerpo $KK$ tiene como mucho $N$ raíces.
]<the:raices>

#proof[
  Por inducción en $N$.

  - Si $N=0$, $q(x)$ es constante (no nulo) y por lo tanto no tiene raíces. #sym.checkmark

  Suponemos ahora que es cierto para $N-1$, y sea $q(x) != 0$ de grado $<= N$.

  Si $q$ no tiene raíces, hemos terminado.
  Si $a in KK$ es una raíz, dividiendo por $(x-a)$ obtenemos $q(x)=(x-a)s(x)$ con $s(x)$ de grado menor o igual a $N - 1$ y $s != 0$.

  Si $b != a$ es otra raíz, entonces $0=q(b)=(b-a)s(b)$, y como $KK$ es un cuerpo y $b-a != 0$, entonces $s(b)=0$.
  Por hipótesis de inducción, $s$ tiene como mucho $N-1$ raíces, luego $q$ tiene como mucho $N$. #sym.checkmark
]

#theorem[Unicidad del polinomio interpolador][
  Dados $N + 1$ puntos $x_0, x_1, ..., x_N in KK$ distintos dos a dos y $N + 1$ valores $y_0, y_1, ..., y_N in KK$, existe un único polinomio $q(x) in KK[x]$ de grado menor o igual a $N$ tal que:

  $
    q(x_i) = y_i quad forall i in {0, 1, ..., N}.
  $
]<the:unic>

#pagebreak()
#proof[
  #set math.equation(numbering: none)
  - La existencia del polinomio la tenemos por el @the:exis. #sym.checkmark
  - Unicidad: supongamos que existen dos polinomios $p(x)$ y $q(x)$, de grado menor o igual a $N$,  tales que cumplen:
    $
      p(x_i) = q(x_i) = y_i quad forall i in {0,1,...,N}.
    $
    Definimos $r(x) := p(x) - q(x)$, que por ser la resta de dos polinomios de grado menor o igual a $N$ debe ser también de grado menor o igual a $N$.
    Sin embargo,
    $
      r(x_i) = p(x_i) - q(x_i) = y_i - y_i = 0 quad forall i in {0,1,...,N},
    $
    por lo que $r(x)$ tiene al menos $N+1$ raíces, pero es de grado menor o igual que $N$, luego por el @the:raices solo puede ser el polinomio idénticamente igual a 0.
    $
      r(x) = 0 = p(x) - q(x) => p(x) = q(x) => "el polinomio interpolador es único". quad checkmark
    $
]


#example[
  Calcular el polinomio cúbico que pasa por los puntos $(1,1), (2,1), (3,2), (4,6)$.
]

#[
  #set math.equation(numbering: none)
  En primer lugar definimos los polinomios de Lagrange:
  $
    l_0(x) = ((x-2)(x-3)(x-4))/((1-2)(1-3)(1-4)) = (x^3 - 9x^2 + 26x -24)/(-6) = -1/6 x^3 + 3/2 x^2 - 13/3 x +4,
  $
  $
    l_1(x) = ((x-1)(x-3)(x-4))/((2-1)(2-3)(2-4)) = (x^3 - 8x^2 + 19x -12)/(2) = 1/2 x^3 -4x^2 +19/2 x -6,
  $
  $
    l_2(x) = ((x-1)(x-2)(x-4))/((3-1)(3-2)(3-4)) = (x^3 - 7x^2 + 14x -8)/(-2) = -1/2 x^3 +7/2 x^2 -7x +4,
  $
  $
    l_3(x) = ((x-1)(x-2)(x-3))/((4-1)(4-2)(4-3)) = (x^3 - 6x^2 + 11x -6)/(6) = 1/6 x^3 -x^2 +11/6 x -1.
  $

  #pagebreak()
  Ahora calculamos $q(x)$:
  $
    q(x) & = sum_(i=0)^3 l_i (x) y_i = l_0 (x) + l_1 (x) + 2 l_2 (x) + 6 l_3 (x) \
         & = (-1/6 x^3 + 3/2 x^2 - 13/3 x +4) + (1/2 x^3 -4x^2 +19/2 x -6) \
         & quad + 2(-1/2 x^3 +7/2 x^2 -7x +4) + 6(1/6 x^3 -x^2 +11/6 x -1) \
         & = 1/3 x^3 -3/2 x^2 + 13/6 x.
  $

  Se puede comprobar fácilmente:
  $
    q(1) = 1/3 - 3/2 + 13/6 = 1, quad checkmark\
    q(2) = 1/3 dot 8 - 3/2 dot 4 + 13/6 dot 2= 1, quad checkmark\
    q(3) = 1/3 dot 27 - 3/2 dot 9 + 13/6 dot 3= 2, quad checkmark\
    q(4) = 1/3 dot 64 - 3/2 dot 16 + 13/6 dot 4= 6. quad checkmark
  $


  #let p(x) = 13 / 6 * x - 3 / 2 * calc.pow(x, 2) + 1 / 3 * calc.pow(x, 3)

  #figure(
    canvas({
      import draw: *

      plot.plot(
        size: (10, 7),
        x-label: $x$,
        y-label: $y$,
        x-tick-step: 1,
        y-tick-step: 1,
        x-min: 0,
        x-max: 5,
        y-min: -1,
        y-max: 7,
        legend: "inner-north-west",
        {
          plot.add(
            domain: (0, 4.5),
            samples: 200,
            p,
            style: (stroke: blue + 1.5pt),
            label: $q(x)$,
          )

          plot.add(
            ((1, 1), (2, 1), (3, 2), (4, 6)),
            style: (stroke: none),
            mark: "o",
            mark-size: 0.15,
            mark-style: (fill: red, stroke: red),
            label: $(x_i, y_i)$,
          )
        },
      )
    }),
    caption: "El polinomio interpolador y sus nodos de interpolación",
  )
]

= Cómo compartir un secreto

#remark[
  Sin pérdida de generalidad podemos asumir que el secreto que queremos compartir es (o se puede convertir en) un número entero.
  Además, vamos a trabajar en un cuerpo finito $FF_(p^m)$ con $p$ un número primo y $p^m$ mayor que el secreto que se quiere compartir.
  En la @the:sec veremos que esto es necesario para obtener secreto perfecto.
]<obs:num>

#definition[Umbral][
  El umbral del esquema de compartición se define como una tupla $(k, n)$ donde $n$ es el número de trozos en los que se ha repartido el secreto y $k$ es el número mínimo de trozos necesarios para recuperarlo.
  $n$ está limitado por el orden del cuerpo sobre el que estemos trabajando: $n < p^m$.
]


El primer paso es decidir el umbral de nuestro reparto, decidiendo cuántos trozos se van a repartir y cuántos van a ser capaces de reconstruir el secreto.
Una vez que ya tenemos $n$ y $k$ lo que vamos a buscar es convertir el secreto en un polinomio $q(x)$ de grado menor o igual a $k-1$.

Para ello, elegimos aleatoriamente $a_1, a_2, ..., a_(k-1) in FF$ de manera uniforme e independiente, y ponemos $a_0$ igual al secreto (que por la @obs:num es un número).
De esta manera, conocer el polinomio nos da el secreto, ya que basta evaluarlo en $x = 0$ para obtener su término independiente.

Teniendo el polinomio (de grado menor o igual a $k-1$) vamos a obtener $n$ puntos, por ejemplo: $q(1), q(2), q(3), ..., q(n)$.
Ahora podemos repartir estos puntos (que son los "trozos" del secreto).

Por lo visto en la @sec:interpol, con $k$ trozos podemos obtener un polinomio de grado menor o igual que $k-1$ que pase por esos $k$ puntos.
Además, por el @the:unic sabemos que el polinomio es único.
Como nuestro polinomio $q(x)$ era de grado menor o igual que $k-1$ y pasaba por esos puntos, necesariamente el polinomio interpolador es exactamente el polinomio $q(x)$.
Hemos recuperado el secreto a partir de $k$ trozos.

#pagebreak()
= Corrección y seguridad del sistema

Ya hemos visto que con $k$ trozos es posible recuperar el secreto.
Pero, ¿qué ocurre si más de $k$ trozos intentan reconstruir el secreto?
Y si conocemos $k' < k$ trozos, ¿tenemos algún ataque con posibilidad de recuperar el secreto?

#proposition[El sistema tiene corrección.]

#proof[
  Sean:
  - $S in FF$ el secreto que se quiere compartir,
  - $(k,n)$ el umbral del sistema,
  - $q(x) in FF [x]$ de grado menor o igual que $k - 1$ escogido como clave y tal que $q(0) = S$,
  - ${(x_i, q(x_i))}_(i in {1,...,n})$ los trozos repartidos.


  #heading([Caso 1: $k$ trozos], numbering: none, level: 3, outlined: false)
  Por el @the:unic sabemos que, dados $k$ trozos distintos cualesquiera, existe un único polinomio $r(x)$ de grado menor o igual que $k-1$ que pasa por esos puntos.
  Como el polinomio original $q(x)$ tiene grado menor o igual a $k-1$, y pasa por los $k$ trozos, sabemos que estos dos polinomios $q$ y $r$ deben ser iguales, sin importar qué trozos han sido escogidos.
  Por lo tanto, es posible recuperar el secreto. #sym.checkmark

  #heading(
    [Caso 2: $k'$ trozos, con $k' > k$],
    numbering: none,
    level: 3,
    outlined: false,
  )
  En el caso de tener más trozos, al calcular el polinomio interpolador nos va a salir un polinomio $r(x)$ de grado menor o igual que $k' - 1$ que pase por los $k'$ nodos.
  Como $q(x)$ tiene grado menor o igual a $k - 1$ en particular tiene grado menor que $k' - 1$, ya que $k < k'$.
  Además, $q(x)$ pasa por los $k'$ nodos, por lo que es un polinomio interpolador de estos nodos.
  Por el @the:unic el polinomio interpolador es único, luego $q$ y $r$ deben ser iguales.
  Por lo tanto, es posible recuperar el secreto. #sym.checkmark
]

#pagebreak()
#proposition[$forall FF$ cuerpo finito, si $|FF| > n$ el sistema tiene seguridad perfecta.]<the:sec>

#proof[
  #set math.equation(numbering: none)
  Sea $FF = FF_(p^m)$.
  Queremos ver que, aunque tengamos acceso a $k' < k$ trozos, no tenemos ninguna información extra acerca del secreto, es decir: $forall s in FF$
  $
    Pr[S = s] = Pr[S = s | "trozos"],
  $
  donde $S$ es una v.a. sobre los posibles secretos y $s$ un secreto.

  En primer lugar podemos ver que es suficiente con probarlo para $k - 1$ trozos, ya que cualquier conjunto de $k' < k$ está contenido en un conjunto de $k-1$ trozos, por lo que si el secreto es independiente de estos también lo es de cualquier subconjunto suyo.

  Sean $(x_1, y_1),...,(x_(k-1),y_(k-1))$ los trozos conocidos.
  Fijamos $s in FF$.
  Cada posible elección de los coeficientes $(a_1,...,a_(k-1)) in FF^(k-1)$ nos da un polinomio $q(x)$ de grado menor o igual a $k-1$ con $q(0) = s$, por lo que tenemos $(p^m)^(k-1)$ posibilidades, todas equiprobables con probabilidad $(p^m)^(-(k-1))$.

  Los trozos son los que hemos visto si y solo si $q(x_i) = y_i quad forall i in {1,2,...,k-1}$, es decir, si $q$ pasa por los $k$ puntos $(0,s)$, $(x_1,y_1)$,..., $(x_(k-1), y_(k-1))$.
  Por el @the:unic, existe exactamente un polinomio de grado menor o igual que $k-1$ que cumple esto.
  Por lo tanto, solo una de las elecciones de $(a_1,...,a_(k-1))$ es compatible, y
  $
    Pr["trozos" | S = s] = (p^m)^(-(k-1)) quad forall s in FF.
  $

  Por la fórmula de la probabilidad total:
  $
    Pr["trozos"] = sum_(s in FF) Pr["trozos" | S = s] Pr[S = s] = (p^m)^(-(k-1)) sum_(s in FF) Pr[S = s] = (p^m)^(-(k-1)).
  $
  Luego:
  $
    Pr[S = s | "trozos"] = (Pr["trozos" | S = s] Pr[S = s]) / Pr["trozos"] = ((p^m)^(-(k-1)) Pr[S = s])/((p^m)^(-(k-1))) = Pr[S = s]. quad checkmark
  $
]

= Ejemplo

#remark[
  En la realidad habría que hacerlo en un cuerpo finito ($ZZ_p$ o $FF_(p^n)$), pero por simplicidad lo voy a hacer en $RR$.
]
#example[
  Queremos repartir el secreto 54321 entre 10 personas, de tal manera que 7 de ellas sean capaces de recuperarlo.
]

#[
  #set math.equation(numbering: none)
  + Definimos el umbral: $(k,n) = (7,10)$.
  + Creamos un polinomio de grado $k-1$ con término independiente igual al secreto:
    $
      q(x) = 54321 - 120 x + 2544 x^2 - 329 x^3 + 85 x^4 -18 x^5 +x^6
    $

    #figure(
      canvas({
        import draw: *

        plot.plot(
          size: (10, 7),
          x-label: $x$,
          y-label: $y$,
          x-tick-step: 1,
          y-tick-step: 10000,
          x-min: 0,
          x-max: 12,
          y-min: 0,
          y-max: 100000,
          {
            plot.add(
              domain: (0, 12),
              samples: 200,
              p,
              style: (stroke: blue + 1.5pt),
            )
          },
        )
      }),
      caption: [El polinomio $q(x)$, que almacena la clave en $q(0)$],
    )
  + Ahora escogemos 10 puntos del polinomio:
    #figure(
      table(
        columns: 12,
        align: center + horizon,
        inset: (x: 10pt, y: 6pt),
        stroke: none,

        [*$x$*],
        ..range(0, 11).map(x => {
          let c = if x == 0 { red } else { black }
          text(fill: c, weight: if x == 0 { "bold" } else { "regular" })[$#x$]
        }),

        [*$q(x)$*],
        ..range(0, 11).map(x => {
          let c = if x == 0 { red } else { black }
          text(fill: c, weight: if x == 0 { "bold" } else {
            "regular"
          })[$#p(x)$]
        }),

        table.hline(y: 1, stroke: 1pt + black),
        table.vline(x: 1, stroke: 1pt + black),
        table.vline(x: 2, stroke: 1pt + black),
      ),
      caption: "Evaluación del polinomio en 10 puntos distintos del 0",
    )<tab:ev>

  + Ya tenemos todos los ingredientes, ahora solo hay que repartir los trozos.
    Cada trozo consiste en un par $(x_i,q(x_i))$, según la @tab:ev.

    #let persona(x) = {
      let c = colores.at(x - 1)
      let oscuro = c.darken(45%)
      rect(
        width: 100%,
        fill: c,
        stroke: (paint: oscuro, thickness: 1.3pt, dash: "dotted"),
        radius: 6pt,
        inset: 8pt,
        align(center, stack(
          dir: ttb,
          spacing: 8pt,
          monigote(oscuro),
          rect(
            fill: white,
            stroke: 0.6pt + oscuro,
            radius: 3pt,
            inset: (x: 6pt, y: 5pt),
            $(#x, #p(x))$,
          ),
        )),
      )
    }

    #grid(
      columns: (1fr,) * 5,
      gutter: 10pt,
      ..range(1, 11).map(persona),
    )
  + Supongamos ahora que 7 de ellos (e.g. 1, 3, 5, 6, 7, 8 y 10) quieren recuperar el secreto. Para ello, construyen el polinomio interpolador de sus nodos mediante el método de Lagrange que hemos visto:

    #figure(
      canvas({
        import draw: *

        plot.plot(
          size: (10, 7),
          x-label: $x$,
          y-label: $y$,
          x-tick-step: 1,
          y-tick-step: 10000,
          x-min: 0,
          x-max: 12,
          y-min: 0,
          y-max: 100000,
          {
            plot.add(
              domain: (0, 12),
              samples: 200,
              p,
              style: (stroke: blue + 1.5pt),
            )
            plot.add(
              (1, 3, 5, 6, 7, 8, 10).map(i => (i, p(i))),
              style: (stroke: none),
              mark: "o",
              mark-size: 0.15,
              mark-style: (fill: red, stroke: red),
            )
          },
        )
      }),
      caption: [El polinomio interpolador de los nodos 1, 3, 5, 6, 7, 8 y 10, que coincide con $q(x)$],
    )
  + Una vez que se conoce el polinomio interpolador, basta evaluarlo en $x=0$ para obtener el secreto: $q(0) = 54321$.

]

= En la práctica

A continuación expongo una serie de consideraciones sobre la aplicación práctica de este esquema de compartición:

+ Hemos demostrado que este sistema tiene seguridad perfecta cuando trabajamos sobre un cuerpo finito, lo que nos viene bien a la hora de implementar esto en un ordenador.
  Este cuerpo tiene que ser de orden $p^m$ mayor que el secreto y mayor que $n$ (para garantizar que a cada persona se le puede dar un trozo distinto).
  Es decir: $p^m > max(n, s)$.

+ Como normalmente lo que se quiere compartir es un documento o cualquier fichero binario en general, una opción bastante común es cifrar ese fichero con un sistema estándar y después compartir la clave de descifrado mediante este esquema de compartición de secretos.
  De tal manera que todo el mundo recibe el archivo cifrado y un trozo de la clave.

+ Se puede hacer un reparto desigual, dependiendo de las necesidades.
  Por ejemplo, podemos imaginar una empresa que quiere compartir un secreto (e.g. una clave criptográfica para firmar documentos) entre sus directivos, de tal manera que hagan falta 5 miembros de la junta para acceder a la clave.
  Sin embargo, también se quiere permitir que el presidente y un único directivo puedan tener la capacidad de acceder al secreto.
  Para lograr esto, basta con darle 4 trozos al presidente, y uno a cada directivo.

+ Este mecanismo tiene dos ventajas principales:
  - No hace falta disponer de todos los trozos para acceder al secreto, evitando así el riesgo de perder algún trozo (hasta $n-k$) o que pueda haber alguna persona que traicione al equipo y bloquee el descifrado.
  - Hace falta que haya varias personas ($k$) que quieran acceder de forma deshonesta al secreto, ya que hacen falta $k$ trozos para descifrarlo.
  En función de las necesidades, se pueden ajustar los parámetros del umbral para adecuarlo a la situación.

+ Algunas limitaciones de este esquema son:
  - Se necesita un agente confiable para llevar a cabo el proceso: existe la posibilidad de que el encargado de realizar los trozos se guarde algún trozo extra, o directamente el secreto, ya que conoce toda la información.
    También se debe confiar en que al descifrar el secreto, solo se usa de manera correcta.
    Por ejemplo, si el secreto es una clave RSA para firmar, la idea es que tras recuperarla se use y luego se borre, garantizando así que la próxima vez que se use sea porque al menos $k$ trozos están de acuerdo.
  - Un participante puede entregar un trozo falso, que según el método propuesto no se puede comprobar, y provocar que el secreto recuperado no sea el correcto.
    Esto lo soluciona Feldman en @feldman con su esquema verificable.

= Implementación

#note-box[
  #show link: underline
  #show link: set text(fill: blue)
  Para escribir el código he usado Inteligencia Artificial.

  Link al repositorio: #link("https://github.com/rodalc24/shamir-secret-sharing")

  Se puede compilar usando el comando `make`.
  El binario generado es `bin/sss` y para ejecutar los tests se puede usar `make test`.
]

Para ilustrar el funcionamiento de este esquema, he implementado una versión de _Shamir's Secret Sharing_ que permite compartir archivos binarios.
Para ello hace uso de la biblioteca _openssl_ y cifra el archivo con AES-256-GCM y después reparte la clave.

#terminal[
  #cmd(user: "shamir@host")[./sss -h]
  #out(
    "Usage:
  sss encrypt <file> -k <threshold> -n <shares> [-o <prefix>]
      Writes <prefix>.enc and <prefix>.share1 ... <prefix>.share<n>
      (prefix defaults to <file>). Any <threshold> shares can decrypt.
  sss decrypt <file.enc> <share>... [-o <output>]
      Each <share> is a share file, or the share text itself (SSS1-...).
      Output defaults to <file.enc> without .enc; use -o - for stdout.",
  )
]

#heading(level: 2, outlined: false, numbering: none, "Cifrado")
El proceso de cifrado es el siguiente:
1. Genera una clave de 32 _bytes_ aleatoria de manera segura (i.e. usando `RAND_bytes`).
2. Cifra el archivo con esa clave mediante AES-256-GCM y genera un `.enc`.
3. Aplica el esquema de _Shamir's Secret Sharing_ a la clave, de acuerdo con los parámetros de `n` y `k` proporcionados.
  La clave se divide _byte_ a _byte_, considerando cada uno de los 32 _bytes_ como un secreto.
  Para cada _byte_ se construye un polinomio aleatorio en $FF_(2^8)$, con el término constante igual al secreto.
  Después se evalúa el polinomio en $n$ puntos, asignándole uno a cada trozo.
4. Escribe cada uno de los trozos en un fichero.
5. Borra de manera segura la clave, el contenido del archivo y los trozos de la memoria (con `OPENSSL_cleanse`) para evitar posibles filtraciones.

#terminal[
  #cmd(user: "shamir@host")[ls -1]
  #out("file.pdf\nsss")
  #cmd(user: "shamir@host")[./sss encrypt file.pdf -k 3 -n 5]
  #out[Encrypted 'file.pdf' -> 'file.pdf.enc'
    Wrote 5 shares (file.pdf.share1 ... file.pdf.share5); any 3 of them can decrypt.]
  #cmd(user: "shamir@host")[ls -1]
  #out(
    "file.pdf
file.pdf.enc
file.pdf.share1
file.pdf.share2
file.pdf.share3
file.pdf.share4
file.pdf.share5
sss",
  )
  #cmd(user: "shamir@host")[cat file.pdf.share\*]
  #out(
    "SSS1-3-01f5455964317263336c4dfc08b6c3d4ef124cc6c81c572df5d566c9257e4ecce1-796e039b
SSS1-3-020856ad6d1e4ce4dd1f22a2ad97b103a355cbff26f1654a4dc5bf5bc967127e1f-30ebc40b
SSS1-3-03b39154388338cc1b888cd1812019277ef7144b5c26132517094db65f8b41e8e1-508abf8d
SSS1-3-046e2e077ca92cdf6f4154d44497b6d7cbbffaa61ab53f2733ddbf1468bb915eae-7ec6d5b0
SSS1-3-05d5e9fe293458f7a9d6faa768201ef3161d25126062494869114df9fe57c2c850-76571f90",
  )
]

Cada trozo está formado por las siguientes partes:

#set math.equation(numbering: none)
$
  underbrace("SSS1", "Versión")-underbrace(3, k)-underbrace(01, x)underbrace("f5", q_1(x))underbrace(45, q_2(x))underbrace(59, q_3(x)) ...underbrace("e1", q_32(x))-underbrace("796e039b", "checksum")
$

donde la $x$ es el punto asignado a cada trozo (coincide con su índice) empezando en 1.
Cada _byte_ posterior es el previamente llamado $y_i$, uno por cada _byte_ de la clave que se ha repartido.
El _checksum_ se usa para detectar errores y modificaciones del trozo (e.g. de la $k$), pero no protege frente a una modificación maliciosa.


#heading(level: 2, outlined: false, numbering: none, "Descifrado")
El proceso de descifrado es el siguiente:
1. Lee los trozos, comprobando el _checksum_ y que la $k$ coincide. También mira que la longitud es correcta (33 _bytes_) y que hay un número mayor o igual que $k$ de trozos con distinta $x$.
2. Reconstruye la clave a partir de los trozos.
3. Descifra el archivo con la clave usando AES-256-GCM.

#terminal[
  #cmd(user: "shamir@host")[ls -1]
  #out(
    "file.pdf
file.pdf.enc
file.pdf.share1
file.pdf.share2
file.pdf.share3
file.pdf.share4
file.pdf.share5
sss",
  )
  #cmd(
    user: "shamir@host",
  )[./sss decrypt file.pdf.enc file.pdf.share\* -o file2.pdf]
  #out[Decrypted 'file.pdf.enc' -> 'file2.pdf' (used 3 of 5 given shares)]
  #cmd(user: "shamir@host")[ls -1]
  #out(
    "file.pdf
file.pdf.enc
file.pdf.share1
file.pdf.share2
file.pdf.share3
file.pdf.share4
file.pdf.share5
file2.pdf
sss",
  )
  #cmd(user: "shamir@host")[diff -s file.pdf file2.pdf]
  #out[Files file.pdf and file2.pdf are identical]
]

#bibliography("biblio.yaml")

