#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct {
    char magic[4];       /* "P2DT" */
    char name[18];       /* Image/File name */
    char subtitle[8];    /* Subtitle string */
    char crlf0[2];       /* \r\n */
    char title[30];      /* Title string */
    char crlf1[2];       /* \r\n */
    char saver[30];      /* Converter/Saver name */
    char crlf2[2];       /* \r\n */
    char eof[1];         /* 0x1A (EOF marker) */
    char reserve0[1];    /* Reserved byte */
    short flag;          /* Compression / Format flags */
    short no;            /* Image number */
    long time;           /* Timestamp */
    long size;           /* Packed data size */
    short depth;         /* Bits per pixel (e.g., 4 or 8) */
    short x_aspect;      /* Pixel aspect ratio X */
    short y_aspect;      /* Pixel aspect ratio Y */
    short x_max;         /* Maximum X coordinate (Width = x_max + 1) */
    short y_max;         /* Maximum Y coordinate (Height = y_max + 1) */
    long reserve1;       /* Reserved field */
} PIC2_HEADER;
#pragma pack(pop)

/*
 * Loads a PIC2 (.p2) image file and converts it to a 24bpp packed RGB pixel array.
 * 
 * Prototype: unsigned char* LoadPIC2(const char *filename, int *out_w, int *out_h)
 * 
 * Returns:
 *   Pointer to dynamically allocated 24bpp RGB buffer (size = width * height * 3),
 *   or NULL on failure. Caller is responsible for calling free() on returned memory.
 */
unsigned char* LoadPIC2(const char *filename, int *out_w, int *out_h) {
    FILE *fp;
    PIC2_HEADER hdr;
    int width, height, num_colors;
    unsigned char palette[256][3];
    int total_pixels, pixel_idx;
    unsigned char *indexed_pixels = NULL;
    unsigned char *rgb_buffer = NULL;
    int i;

    if (!filename || !out_w || !out_h) {
        return NULL;
    }

    *out_w = 0;
    *out_h = 0;

    /* 1. Open File */
    fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    /* 2. Read Exact 114-Byte Header */
    if (fread(&hdr, 1, sizeof(PIC2_HEADER), fp) != sizeof(PIC2_HEADER)) {
        fclose(fp);
        return NULL;
    }

    /* 3. Validate "P2DT" Magic Bytes */
    if (memcmp(hdr.magic, "P2DT", 4) != 0) {
        fclose(fp);
        return NULL;
    }

    /* 4. Calculate Image Dimensions */
    width  = (int)hdr.x_max + 1;
    height = (int)hdr.y_max + 1;

    if (width <= 0 || height <= 0) {
        fclose(fp);
        return NULL;
    }

    /* 5. Determine Palette Size (4bpp = 16 colors, 8bpp = 256 colors) */
    num_colors = (hdr.depth <= 4) ? 16 : 256;
    total_pixels = width * height;

    /* 6. Read Palette Table (R, G, B bytes) */
    for (i = 0; i < num_colors; i++) {
        palette[i][0] = (unsigned char)fgetc(fp); /* Red */
        palette[i][1] = (unsigned char)fgetc(fp); /* Green */
        palette[i][2] = (unsigned char)fgetc(fp); /* Blue */
    }

    /* 7. Allocate Temporary Indexed Buffer */
    indexed_pixels = (unsigned char *)malloc(total_pixels);
    if (!indexed_pixels) {
        fclose(fp);
        return NULL;
    }

    /* 8. Decompress Bitstream / RLE Stream */
    pixel_idx = 0;
    while (pixel_idx < total_pixels) {
        int byte_in = fgetc(fp);
        if (byte_in == EOF) break;

        if (byte_in == 0x00) { /* RLE Repeat Marker */
            int count = fgetc(fp);
            int color = fgetc(fp);
            if (count == EOF || color == EOF) break;

            while (count-- > 0 && pixel_idx < total_pixels) {
                indexed_pixels[pixel_idx++] = (unsigned char)color;
            }
        } else {
            indexed_pixels[pixel_idx++] = (unsigned char)byte_in;
        }
    }

    fclose(fp);

    /* Fail if file ended before unpacking all pixels */
    if (pixel_idx < total_pixels) {
        free(indexed_pixels);
        return NULL;
    }

    /* 9. Allocate Final 24bpp RGB Buffer (width * height * 3) */
    rgb_buffer = (unsigned char *)malloc((size_t)total_pixels * 3);
    if (!rgb_buffer) {
        free(indexed_pixels);
        return NULL;
    }

    /* 10. Map Palette Indices to 24-bit RGB Triplets */
    for (i = 0; i < total_pixels; i++) {
        unsigned char idx = indexed_pixels[i];
        int out_idx = i * 3;

        rgb_buffer[out_idx + 0] = palette[idx][0]; /* Red */
        rgb_buffer[out_idx + 1] = palette[idx][1]; /* Green */
        rgb_buffer[out_idx + 2] = palette[idx][2]; /* Blue */
    }

    free(indexed_pixels);

    /* Output Dimensions & Return Buffer */
    *out_w = width;
    *out_h = height;

    return rgb_buffer;
}
