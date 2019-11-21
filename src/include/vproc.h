
/* common include definitions */
#ifndef __LOADED_COMMON_INC__
    #define __LOADED_COMMON_INC__
    #include <stdint.h>
    #include <errno.h>
#endif

#ifdef _VPROC_MAIN_
  #ifndef __VPROC_MAIN_DEFINED__
	#define __VPROC_MAIN_DEFINED__

	/* everything that should be defined in the 'main' source file goes here */
	int vproc_init();

  #endif
#endif

/* debugging header */
#ifndef __LOADED_DEBUG__
    #define __LOADED_DEBUG__
    #include "debug.h"
#endif

/* inclusions.h decides what headers you need based on definitions */
#include "inclusions.h"

/* if from a vproc system source, require these things... */
#ifdef __VPROC_SYS__
#ifndef __LOADED_VPROC_SYS__
    #define __LOADED_VPROC_SYS__
    #include "vproc_mm.h"
#endif

#endif

