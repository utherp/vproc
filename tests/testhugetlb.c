#include <stdio.h>
#include <stdlib.h>
int main () {
    char buf [120];
    printf ("Huge page text segment"
            "map:\n");
    sprintf (buf, "grep 00600000- "
            "/proc/%d/maps | "
            "sed -e \"s/ \\{26\\}//\"",
            getpid ());
    system (buf);
    return 0;
}
