# Shamir's Secret Sharing

Proyecto para la asignatura de _Criptografía y Seguridad Informática_ (doble grado en Ingeniería Informática y Matemáticas, Universidad Autónoma de Madrid).

## Introducción

El esquema de compartición de secretos es un protocolo definido en 1979 por Adi Shamir (uno de los inventores de RSA).
Busca dividir un secreto entre un grupo de personas de tal manera que se pueda recuperar si y solo si un subgrupo de tamaño prefijado de los integrantes del grupo se ponen de acuerdo.
Por ejemplo, se puede compartir entre un grupo de 10 personas y solo se puede recuperar el secreto si 7 de ellas así lo quieren.

Este proyecto se compone de dos partes, un estudio teórico y una implementación práctica.

## Estudio teórico

En el [estudio teórico](doc/sss.pdf) se explica (desde un punto de vista matemático) el esquema de _Shamir's Secret Sharing_ (SSS), demostrando su fundamento y que es seguro.
Cuenta con un ejemplo completo para ilustrar el funcionamiento de SSS.

Incluye también unas observaciones sobre la implementación práctica y la descripción del programa escrito en C.

## Implementación en C

> [!NOTE]
> La implementación se ha realizado usando Inteligencia Artificial

Para ilustrar lo visto en la parte teórica, he realizado una implementación en C que permite repartir archivos usando el algoritmo SSS.
La compilación se realiza usando el comando `make` en la raíz del repositorio, generando un directorio `bin/` con el ejecutable (`bin/sss`) y los tests.

1. Repartir un secreto:

```bash
$ ./sss encrypt -k 3 -n 5 file.pdf
```

donde `file.pdf` es el archivo (puede ser cualquier fichero).
Esto genera un `file.pdf.enc` con el archivo cifrado con AES-256-GCM y \(n\) trozos de la clave, guardados en `file.pdf.share`.

> [!WARNING]
> Si un archivo que se va a generar ya existe, el programa no lo sobreescribe y devuelve error.

2. Reconstruir el secreto:

```bash
$ ./sss decrypt file.pdf.enc file.pdf.share1  file.pdf.share2  file.pdf.share3
```

Para una descripción más detallada consultar el documento del estudio teórico.
