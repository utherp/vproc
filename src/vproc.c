#include "vproc.h"

int vproc_init() {
    _debugger("VP", "Initializing VProc", 0);
    _debugger("VP", "Initializing Memory Management Unit", 0);
    vproc_mm_init();
    _debugger("VP", "Initializing Stream Management Unit", 0);
    vproc_sm_init();
    return 0;
}
