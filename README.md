# Single-Header OpenGL Text Rendering Library

This is a single-header library for rendering text in OpenGL. It depends on
[FreeType](https://freetype.org/) and optionally the C standard library. You can
remove the standard library by redefining `GLT_REALLOC` to your own realloc
function. See `example.c` for an example on how to use this library or look at
the usage below.

<p align="center">
  <img src="screenshot.png" alt="Example Screenshot">
</p>

## Usage

As this is a single-header library, you must define `GLT_IMPL` in at least one
source file and link with FreeType.

```c
#define GLT_IMPL
#include "gltext.h"
...
```

### Drawing Text

Before drawing any text, you must create and bind a font.
Currently, the only way to create a font is to load a font from a file using
FreeType.
You can use `gltCreateFont` to create the font and `gltBindFont` to bind it.

```c
GLuint font = gltCreateFont("/path/to/font.ttf");
gltBindFont(font);
```

After binding the font, we can draw some text. There are two ways to draw text.
The first is to use the functions `glDrawText` or `glDrawnText` to immediately
draw text. These create one vertex and index buffer for every call. You can
batch multiple calls into a single draw call by managing your own buffer using
`GLTbuffer` and the `gltPushText`, `gltPushnText` and `gltDraw` functions.
The `gltDrawText` just combines `gltPushText` and `gltDraw` into a single
function.

```c
gltDrawText(0, 0, "Hello, world!");

GLTbuffer b = {0};
gltPushText(&b, 0, 0, "Hello, world!");
gltPushText(&b, 0, 50, "Hello, world!");
gltDraw(b);
```

### Custom Transform Matrix

You can change the transform matrix using the `gltSetTransform` function.
Most of the time, you will need an orthographic projection matrix to draw text
in 2d. You can use `gltOrtho` to create such a matrix. If the transform was not
set before drawing text, then the library will create an orthographic projection
matrix from the viewport parameters.

```c
gltOrtho(0, viewportWidth, 0, viewportHeight, -1, 1);

/* for column-major matrices */
float transform[4][4] = ...;
gltSetTransform(transform, GL_FALSE);

/* for row-major matrices */
float transform[4][4] = ...;
gltSetTransform(transform, GL_TRUE);
```

### Removing Inclusion of the Standard Library

By default, the library uses `realloc` for dynamically allocating the vertex and
index buffer. You can use your own reallocation function by redefining the
`GLT_REALLOC` macro.

```
#define GLT_REALLOC(p, sz) (my_realloc(p, sz))
```
