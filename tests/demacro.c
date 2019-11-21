#include <stdio.h>
#include <string.h>
//#include <stdlib.h>
//#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <fdio.h>
#include <malign.h>
#include <hm12_nv12.h>
#include <nv12_yv444.h>

int main (int argc, char **argv) {
    unsigned int ysize, width, height, delay, dynamic_naming = 0;
    double fps, tmp;
    size_t bufsize[2];
    int fd, out, count, i;
    char *src, *scratch, *dst;
    char *input_fn, *output_fn;
    char output_fn_buf[1024] = { 0 };

    if (argc < 5) {
        printf(
            "Usage: %s input output width height [fps] [count]\n"
            "\t input:   input (hm12) filename\n"
            "\toutput:   output (yv444) filename (may contain %u representing frame #)\n"
            "\t width:   frame width\n"
            "\theight:   frame height\n"
            "\t   fps:   frames per second (default: 30000/1001)\n"
            "\t count:   frames to convert. -1 == forever (default: 1)\n"
            , argv[0]);
        return 1;
    }

    input_fn = argv[1];
    output_fn = argv[2];

    width = atoi(argv[3]);
    height= atoi(argv[4]);
    fps   = (argc>5)?atof(argv[5]):(30000/1001);
    count = (argc>4)?atoi(argv[4]):1;

    ysize = width * height;
    bufsize[1] = ysize * 3;
    bufsize[0] = bufsize[1] / 2;

    src = malign_alloc(16, (bufsize[0] * 2) + bufsize[1]); 
    scratch = src + bufsize[0];
    dst = scratch + bufsize[0];

    tmp = 1000000 / fps;
    delay = (unsigned int)tmp;

    if ((fd = open(input_fn, O_RDONLY)) == -1) {
        fprintf(stderr, "Error: failed opening input file '%s': %s\n", input_fn, strerror(errno));
        return 2;
    }

    for (i=0; i<strlen(output_fn)-1; i++) {
        if (output_fn[i] == '%' && output_fn[i+1] == 'u') {
            dynamic_naming = 1;
            break;
        }
    }

    if (!dynamic_naming) {
        if ((out = open(output_fn, O_RDWR | O_CREAT, 0644)) == -1) {
            fprintf(stderr, "Error: failed opening output file '%s': %s\n", output_fn, strerror(errno));
            return 3;
        }
    }

    for (i=0; (count < 0) || (i < count); i++) {
        read_bytes(fd, src, bufsize[0]);
        hm12_nv12(scratch, src, width, height);
        nv12_yv444(dst, scratch, width, height);
        if (dynamic_naming) {
            snprintf(output_fn_buf, 1024, output_fn, i);
            if ((out = open(output_fn, O_RDWR | O_CREAT, 0644)) == -1) {
                fprintf(stderr, "Error: failed opening output file '%s': %s\n", output_fn_buf, strerror(errno));
                return 3;
            }
            write_bytes(out, dst, bufsize[1]); 
            close(out);
        } else {
            write_bytes(out, dst, bufsize[1]);
        }
        usleep(delay);
        printf("one\n"); fflush(stdout);
    }

    if (!dynamic_naming) close(out);
    close(fd);

    free(src);
    return 0;
}

