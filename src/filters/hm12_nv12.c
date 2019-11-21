#include <xmmintrin.h>

void hm12_nv12(unsigned char* dst,unsigned char* src,int w,int h) {
    unsigned int y;
    unsigned int dstride = w;
    register __m128 *msrc = (__m128*) src;

    h += h>>1;
    // descramble plane
    for (y=0; y<h; y+=16) {
        unsigned int x;
        unsigned char *rdst = dst + (dstride * y);// (__m128*) mstride dst + (y * mstride);
        for (x=0; x<w; x+=16) {
            unsigned int i;
            unsigned char *cdst = rdst + x;
            for (i=0;i<16;i++) {
                *(__m128*)cdst = *msrc++;
                cdst += dstride;
            }
        }
    }
}


