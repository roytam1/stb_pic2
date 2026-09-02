# stb_pic2

Single-file C library for loading PIC2 (`.p2`) image files, written in the style of [stb libraries](https://github.com/nothings/stb).

## Features

- **Single-file header-only library** — drop `stb_pic2.c` into your project and go
- **All four block formats** — P2SS (arithmetic), P2SF (fast), P2BM, P2BI
- **24bpp RGB output** — returns standard packed RGB pixel data
- **Big-endian PIC2 format support** — handles the multi-byte integer byte order correctly
- **Arithmetic, fast, and raw decoders** — full decompression support

## Usage

```c
#define STB_PIC2_IMPLEMENTATION
#include "stb_pic2.c"

int w, h;
unsigned char *pixels = LoadPIC2("image.p2", &w, &h);
if (pixels) {
    // pixels is w * h * 3 bytes of 24bpp RGB data
    // ...
    free(pixels);
}
```

## Building

```bash
# With GCC
gcc -O2 -o test test_pic2.c -lm

# With any C compiler
cc -O2 test_pic2.c -o test -lm
```

## Format

PIC2 is an image format used by the XV image viewer. The file structure:

- **124-byte header** — image dimensions, depth, flags
- **Optional palette** (if header flag & 1)
- **Comment data**
- **Blocks** starting at `header.size` — each block has a 26-byte header followed by compressed pixel data

Supported block types:

| ID   | Decoder      | Description                          |
|------|-------------|--------------------------------------|
| P2SS | Arithmetic  | Context-model arithmetic coding      |
| P2SF | Fast        | Cache-based fast decompression       |
| P2BM | Raw         | Uncompressed bitmap                  |
| P2BI | Raw         | Uncompressed interleaved bitmap      |

## Files

- `stb_pic2.c` — the library (single-file, `#define STB_PIC2_IMPLEMENTATION` before include)
- `test_pic2.c` — test/demo program that loads a PIC2 file and writes a BMP

## Test

```
LoadPIC2("ICVPIC.P2", &w, &h)
```

Verified against `ICVPIC.bmp` reference — **100% pixel-perfect match** on the 640×480 24bpp test image.

## License

Public domain. Based on the PIC2 format specification and xvpic2.c reference implementation from XV.
