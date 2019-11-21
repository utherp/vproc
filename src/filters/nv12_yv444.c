#include <stdlib.h>
#include <stdio.h>
#include <xmmintrin.h>

void nv12_yv444 (unsigned char *dst, unsigned char *src, int w, int h) {
    unsigned int y;
    unsigned int plane_size = w * h;
    unsigned int c = plane_size >> 4;

/*
    __m128 *msrc = (__m128*)src;
    __m128 *mdst = (__m128*)dst;

    for (c = plane_size >> 4; c; c--) {
        *mdst++ = *msrc++;
    }
    dst = (unsigned char*)mdst;
    src = (unsigned char*)msrc;
*/
    asm volatile (
        "movl   %0,  %%eax               \n"
        "movl   %1,  %%ebx               \n"
        "movl   %2,  %%ecx               \n"
        "nv12yv444.cpy_Y_top:            \n"
        "    movups  (%%eax),%%xmm0      \n"
        "    movups  %%xmm0, (%%ebx)     \n"
        "    addl    $16,    %%eax       \n"
        "    addl    $16,    %%ebx       \n"
        "    loop    nv12yv444.cpy_Y_top \n"
        "movl   %%eax,  %0               \n"
        "movl   %%ebx,  %1               \n"
        :// "=a" (src), "=b" (dst)
        : "m" (src), "m" (dst), "m" (c)
        : "eax", "ebx", "ecx"
    );

    unsigned char *vdst = dst + plane_size;

    h >>= 1;    // divide height by 2 as we handle 2 UV rows each loop

    /*********************************************
     * NOTE: need to use r?x registers on 64 bit
     * machines to make it compatible!! (how?)
     */
    asm volatile (
        "movl   %0,     %%eax                       # eax == *src            \n"    // 1
        "movl   %1,     %%ebx                       # ebx == *udst_odd       \n"    // 2
        "movl   %2,     %%edx                       # edx == *vdst_odd       \n"    // 3

        "movl   %3,     %%edi                       # esi and edi are refs   \n"    // 4
        "movl   %%edi,  %%esi                       # to even rows           \n"    // 5
        "addl   %%ebx,  %%esi                       # esi == *udst_even      \n"    // 6
        "addl   %%edx,  %%edi                       # edi == *vdst_even      \n"    // 7

        "nv12yv444.cpy_UV_top:                # row loop top                 \n"

        "    movl    %3,     %%ecx                  # load width into ecx    \n"    // 8
        "    shrl    $3,     %%ecx                  # divide width by 8      \n"    // 9

        "    nv12yv444.cpy_UV_inner:          # column loop top              \n"

        "        movq      (%%eax),%%mm1            # mm1: uvUVwxWX          \n"    
        "        addl      $8,     %%eax            # src += 8               \n"    // 10

        "        pxor      %%mm2,  %%mm2            # mm2: 00000000 (clear)  \n"
        "        pxor      %%mm3,  %%mm3            # mm3: 00000000 (clear)  \n"    // 11

        "        por       %%mm1,  %%mm2            # mm2: uvUVwxWX (==mm1)  \n"
        "        por       %%mm1,  %%mm3            # mm3: uvUVwxWX (==mm1)  \n"    // 12

        "        punpckhbw %%mm3,  %%mm1            # mm1: uuvvUUVV (unpack bytes to words) \n"
        "        punpcklbw %%mm3,  %%mm2            # mm2: wwxxWWXX (unpack bytes to words) \n"     // 13

        "        pxor      %%mm3,  %%mm3            # mm3: 00000000 (clear)  \n"
        "        pxor      %%mm4,  %%mm4            # mm4: 00000000 (clear)  \n"    // 14

        "        por       %%mm1,  %%mm3            # mm3: uuvvUUVV (==mm1)  \n"
        "        por       %%mm2,  %%mm4            # mm4: wwxxWWXX (==mm2)  \n"    // 15

        "        psllq     $32,    %%mm1            # mm1: UUVV0000 (mm1<<32)\n"
        "        psllq     $32,    %%mm2            # mm2: WWXX0000 (mm2<<32)\n"    // 16

        "        punpckhwd %%mm3,  %%mm1            # mm1: uuUUvvVV (unpack words to dwords) \n"
        "        punpckhwd %%mm4,  %%mm2            # mm2: wwWWxxXX (unpack words to dwords) \n"    // 17

        "        pxor      %%mm3,  %%mm3            # mm3: 00000000 (clear)  \n"
        "        pxor      %%mm4,  %%mm4            # mm4: 00000000 (clear)  \n"    // 18

        "        por       %%mm1,  %%mm3            # mm3: uuUUvvVV (==mm1)  \n"
        "        por       %%mm2,  %%mm4            # mm4: wwWWxxXX (==mm2)  \n"    // 19



        "        punpckldq %%mm3,  %%mm4            # mm4: uuUUwwWW (unpack dwords to quads) \n"

        "        movq      %%mm4,  (%%ebx)          # write V bytes to vdst_odd        \n"  // 20
        "        addl      $8,     %%ebx            # vdst_odd += 8                    \n"

        "        punpckhdq %%mm1,  %%mm2            # mm2: vvVVxxXX (unpack dwords to quads) \n"  // 21
        "        movq      %%mm4,  (%%esi)          # write V bytes to vdst_even       \n"
        "        addl      $8,     %%esi            # vdst_even += 8                   \n"  // 22

        "        movq      %%mm2,  (%%edx)          # write U bytes to udst_odd        \n"
        "        addl      $8,     %%edx            # udst_odd += 8                    \n"  // 23

        "        movq      %%mm2,  (%%edi)          # write U bytes to udst_even       \n"
        "        addl      $8,     %%edi            # udst_even += 8                   \n"  // 24

        "        decl      %%ecx                    # decrement column loop counter    \n"  // 25
        "        jnz     nv12yv444.cpy_UV_inner     # goto column loop top if not zero \n"  // 26
         

        "    decl    %4                             # decrement height loop counter    \n"  // 27
        "    jz      nv12yv444.cpy_UV_bottom        # goto bottom if height counter is zero \n" // 28

        "    movl    %%esi, %%ebx           # move odd row addrs to even row addrs     \n"
        "    movl    %%edi, %%edx                                                      \n"

        "    addl    %3, %%esi              # add width to odd row addrs               \n"
        "    addl    %3, %%edi                                                         \n"
        
        "    jmp     nv12yv444.cpy_UV_top   # jump back to row loop                    \n"

        "nv12yv444.cpy_UV_bottom:           # finished                                 \n"
 
        : /* no output operands */
        : "m" (src), "m" (dst), "m" (vdst), "m" (w), "m" (h)
        : "eax", "ebx", "ecx", "edx", "edi"
    );

    return;
}
