#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

void cpulim (int signal) {
    struct timeval now;
    gettimeofday(&now, NULL);
    printf("Recieved signal %u at %u.%06u\n", signal, now.tv_sec, now.tv_usec);
    return;
}

int main (int c, char *argv[]) {
    struct rlimit blah;

    void *ret = signal(SIGXCPU, cpulim);
    if (ret == SIG_ERR) {
        printf("Error: Failed setting signal handler for SIGXCPU: %s\n", strerror(errno));
        return 1;
    }

    if (getrlimit(RLIMIT_CPU, &blah)) {
        printf("Error: failed to get RLIMIT_CPU: %s\n", strerror(errno));
        return 2;
    }
    printf("soft: %d, hard: %d\nsetting to 2 and 5 seconds...\n\n", blah.rlim_cur, blah.rlim_max);

    blah.rlim_cur = 2;
    blah.rlim_max = 5;

    if (setrlimit(RLIMIT_CPU, &blah)) {
        printf("Error: failed to set RLIMIT_CPU: %s\n", strerror(errno));
        return 3;
    }

    double val = 1;
    while (1) { val *= 1.000001; }
    
    return 0;
}

