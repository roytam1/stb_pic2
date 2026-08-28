#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Corrected PIC2 (.p2) decoder handling variable-length comment strings.
 * Outputs a 24bpp BGR DWORD-aligned GDI pixel array.
 */
unsigned char* load_pic2_to_gdi_bgr24(const char *filename, int *out_w, int *out_h, int *out_stride) {
    FILE *fp;
    char magic[4];
    int c;
    unsigned char meta[5];
    int width, height, bpp, num_colors;
    unsigned char palette[256][3];
    int total_pixels, pixel_idx;
    unsigned char *indexed_pixels = NULL;
    
    int row_stride;
    long gdi_buf_size;
    unsigned char *gdi_buffer = NULL;
    int y, x;

    if (!filename || !out_w || !out_h || !out_stride) {
        return NULL;
    }

    /* 1. Open File */
    fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    /* 2. Validate "P2DT" Magic Identifier */
    if (fread(magic, 1, 4, fp) != 4 || memcmp(magic, "P2DT", 4) != 0) {
        fclose(fp);
        return NULL;
    }

    /* 3. Skip Variable-Length Comment Section (Null-terminated ASCII string) */
    while ((c = fgetc(fp)) != EOF) {
        if (c == 0x00) {
            break; /* Reached end of comment string */
        }
    }

    if (c == EOF) {
        fclose(fp);
        return NULL;
    }

    /* 4. Read Metadata: Width (2 bytes), Height (2 bytes), BPP (1 byte) */
    if (fread(meta, 1, 5, fp) != 5) {
        fclose(fp);
        return NULL;
    }

    width  = (int)meta[0] | ((int)meta[1] << 8);
    height = (int)meta[2] | ((int)meta[3] << 8);
    bpp    = (int)meta[4];
    num_colors = (bpp == 4) ? 16 : 256;

    total_pixels = width * height;
    if (width <= 0 || height <= 0 || total_pixels <= 0) {
        fclose(fp);
        return NULL;
    }

    /* 5. Load Palette (3 bytes per color: Red, Green, Blue) */
    for (x = 0; x < num_colors; x++) {
        palette[x][0] = (unsigned char)fgetc(fp); /* Red */
        palette[x][1] = (unsigned char)fgetc(fp); /* Green */
        palette[x][2] = (unsigned char)fgetc(fp); /* Blue */
    }

    /* 6. Allocate Indexed Pixel Buffer */
    indexed_pixels = (unsigned char *)malloc(total_pixels);
    if (!indexed_pixels) {
        fclose(fp);
        return NULL;
    }

    /* 7. Unpack RLE / Bitstream */
    pixel_idx = 0;
    while (pixel_idx < total_pixels) {
        int byte_in = fgetc(fp);
        if (byte_in == EOF) {
            break;
        }

        if (byte_in == 0x00) { /* RLE repeat sequence marker */
            int count = fgetc(fp);
            int color = fgetc(fp);
            if (count == EOF || color == EOF) {
                break;
            }

            while (count-- > 0 && pixel_idx < total_pixels) {
                indexed_pixels[pixel_idx++] = (unsigned char)color;
            }
        } else {
            indexed_pixels[pixel_idx++] = (unsigned char)byte_in;
        }
    }

    fclose(fp);

    if (pixel_idx < total_pixels) {
        free(indexed_pixels);
        return NULL;
    }

    /* 8. Prepare GDI Buffer (DWORD alignment & bottom-up row ordering) */
    row_stride = ((width * 3) + 3) & ~3;
    gdi_buf_size = (long)row_stride * height;

    gdi_buffer = (unsigned char *)malloc(gdi_buf_size);
    if (!gdi_buffer) {
        free(indexed_pixels);
        return NULL;
    }

    memset(gdi_buffer, 0, gdi_buf_size);

    /* 9. Convert to 24bpp BGR & Flip Vertically for GDI */
    for (y = 0; y < height; y++) {
        int src_row = y;
        int dst_row = (height - 1) - y;

        unsigned char *dst_line = gdi_buffer + ((long)dst_row * row_stride);
        const unsigned char *src_line = indexed_pixels + (src_row * width);

        for (x = 0; x < width; x++) {
            unsigned char idx = src_line[x];
            int dst_pixel = x * 3;

            dst_line[dst_pixel + 0] = palette[idx][2]; /* Blue */
            dst_line[dst_pixel + 1] = palette[idx][1]; /* Green */
            dst_line[dst_pixel + 2] = palette[idx][0]; /* Red */
        }
    }

    free(indexed_pixels);

    *out_w = width;
    *out_h = height;
    *out_stride = row_stride;

    return gdi_buffer;
}
