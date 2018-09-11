#if !defined(INCLUDE)
#include <stdint.h>          /* For uint16_t definition                       */
#define EXT
#endif

#include "remvardef.h"

typedef struct
{
 void *p;			/* pointer to parameter */
 char size;			/* parameter size */
} T_PARM, *P_PARM;

#define PARM(x) {(void *) &x,sizeof(x)}
