#if !defined(INCLUDE)
#include "stm32f1xx_hal.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "remvar.h"
#include "serialio.h"
#include "sync.h"
#define EXT
#endif

#define REM_ISR 1		/* remote port using isr */

#if REM_ISR
#define putrem putRem
#define putstrrem putstrRem
#define sndhexrem sndhexRem
#define getrem getRem
#define gethexrem gethexRem
#define getnumrem getnumRem
#define getstrrem getstrRem
#else
#define putrem putx1
#define putstrrem putstr1
#define sndhexrem sndhex1
#define getrem get1
#define gethexrem gethex1
#define getnumrem getnum1
#define getstrrem getstr1
#endif

#define DBG_LOAD 1

void remcmd();

EXT int16_t tmpval;

#include "cmdList.h"
#include "parmList.h"

#if !defined(INCLUDE)

#include "remparm.h"

void loadVal()
{
 gethexrem();			/* read the parameter number */
 int parm = valRem;		/* save it */

 if (parm < SYNC_MAX_PARM)	/* if in range */
 {
  P_PARM valptr = &remparm[parm]; /* pointer to parameter info */
  p = valptr->p;		/* destination pointer */
  int size = valptr->size;	/* value size */

  int type = getnumrem();	/* get the value */
  if (type == 1)		/* if integer */
  {
#if DBG_LOAD
   printf("w %2x %d (%08x) =  %8x\n",
	  parm, size, (unsigned) p, (unsigned) valRem);
#endif
   if (size == 4)		/* if a long word */
   {
    *(int32_t *) p = valRem;	/* save as a long word */
   }
   else if (size == 1)		/* if a character */
   {
    *p = valRem;		/* save the character */
   }
   else if (size == 2)		/* if a short integer */
   {
    *(int16_t *) p = valRem;	/* save the value */
   }
  }
  else if (type == 2)		/* if floating value */
  {
#if DBG_LOAD
   printf("w %2x f (%08x) =  %8.4f\n", parm, (unsigned) p, fValRem);
#endif
   *(float *) p = fValRem;	/* save as a float */
  }
 }
}

void remcmd()
{
 P_PARM valptr;
 int parm;

 remcmdUpdateTime = millis();
 remcmdTimeout = REMCMD_TIMEOUT;

 putrem('-');
 gethexrem();			/* read parameter */
 parm = valRem;
 switch (parm)
 {
 case SYNC_SETUP:
  encoderSetup();
  break;

 case SYNC_START:
  encoderStart();
  break;

 case SYNC_STOP:
  cmpTmr.stop = 1;		/* set stop flag */
  break;
  
 case SYNC_LOADVAL:		/* load a local parameter */
  loadVal();
  break;

 case SYNC_LOADMULTI:		/* load multiple parameters */
 {
  gethexrem();
  int count = valRem;
  while (--count >= 0)
   loadVal();
 }
  break;

 case SYNC_READVAL:		/* read a local parameter */
  gethexrem();			/* save the parameter number */
  parm = valRem;		/* save it */
  valptr = &remparm[parm];	/* pointer to parameters */
  if (DBG_LOAD)
  {
   unsigned char *p = valptr->p;
   char size;
   unsigned int tmp = 0;
   size = valptr->size;
   if (size == 4)
   {
    tmp = *(int32_t *) p;
   }
   else if (size == 1)
   {
    tmp = *p;
   }
   else if (size == 2)
   {
    tmp = *(int16_t *) p;
   }
   printf("r %2x %d (%08x) =  %8x\n", 
	  (unsigned int) parm, size, (unsigned int) p, tmp);
  }
  sndhexrem(valptr->p, valptr->size); /* send the response */
  break;
 }

#if REM_ISR
 while (1)
 {
  int tmp = getRem();
  if (tmp < 0)
  {
   printf("end of buffer\n");
   break;
  }
  if (tmp == '\r')
   break;
  printf("extra char %d\n", tmp);
  flushBuf();
 }
#endif

 putrem('*');
}

#endif
