#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define STB_PIC2_IMPLEMENTATION
#include "stb_pic2.c"

int main(void)
{
    int w, h;
    unsigned char *pix;

    pix = LoadPIC2("ICVPIC.P2", &w, &h);
    if (!pix) {
        fprintf(stderr, "Failed to load ICVPIC.P2\n");
        return 1;
    }
    fprintf(stderr, "Loaded: %dx%d\n", w, h);

    /* Write a simple BMP for visual verification */
    {
        FILE *fp = fopen("ICVPIC_out.bmp", "wb");
        int row, pad;
        unsigned char hdr[54];
        unsigned char zero[3] = {0,0,0};

        if (!fp) { free(pix); return 1; }

        memset(hdr, 0, 54);
        pad = (4 - (w * 3) % 4) % 4;

        hdr[0] = 'B'; hdr[1] = 'M';
        *(int*)&hdr[2] = 54 + (w*3+pad)*h;
        *(int*)&hdr[10] = 54;
        *(int*)&hdr[14] = 40;
        *(int*)&hdr[18] = w;
        *(int*)&hdr[22] = h;
        *(short*)&hdr[26] = 1;
        *(short*)&hdr[28] = 24;
        *(int*)&hdr[34] = (w*3+pad)*h;

        fwrite(hdr, 1, 54, fp);

        for (row = h - 1; row >= 0; row--) {
            unsigned char *p = pix + row * w * 3;
            unsigned char *rowbuf = (unsigned char *)malloc(w * 3);
            int i;
            for (i = 0; i < w; i++) {
                rowbuf[i*3+0] = p[i*3+2]; /* B */
                rowbuf[i*3+1] = p[i*3+1]; /* G */
                rowbuf[i*3+2] = p[i*3+0]; /* R */
            }
            fwrite(rowbuf, 1, w * 3, fp);
            free(rowbuf);
            if (pad) fwrite(zero, 1, pad, fp);
        }
        fclose(fp);
        fprintf(stderr, "Wrote ICVPIC_out.bmp\n");
    }

    free(pix);
    return 0;
}
