#include "stm32f1xx_hal.h"

#include "sync.h"

int decoder[16] =
{
 0,		/* 0000 0 */
 DIR_NEG,	/* 0001 1 r0 !l_b & !l_a & !c_b &  c_1 */
 DIR_POS,	/* 0010 2 f0 !l_b & !l_a &  c_b & !c_1 */
 0,		/* 0011 3 */
 DIR_POS,	/* 0100 4 f1 !l_b &  l_a & !c_b & !c_1 */
 0,		/* 0101 5 */
 0,		/* 0110 6 */
 DIR_NEG,	/* 0111 7 r3 !l_b &  l_a &  c_b &  c_1 */
 DIR_NEG,	/* 1000 8 r1  l_b & !l_a & !c_b & !c_1 */
 0,		/* 1001 9 */
 0,		/* 1010 a */
 DIR_POS,	/* 1011 b f3  l_b & !l_a &  c_b &  c_1 */
 0,		/* 1100 c */
 DIR_POS,	/* 1101 d f2  l_b &  l_a & !c_b &  c_1 */
 DIR_NEG,	/* 1110 e r2  l_b &  l_a &  c_b & !c_1 */
 0,		/* 1111 f */
};

int lastDecode;
BITWORD decode;

void encoderISR(void)
{
 BITWORD tmp;
 encoderClr();
 if (EXTI->PR & (A_Pin | B_Pin)) /* if bit change */
 {
  if (EXTI->PR & A_Pin)
   EXTI->PR = A_Pin;
  if (EXTI->PR & B_Pin)
   EXTI->PR = B_Pin;

  tmp.w = (lastDecode >> 2);	/* shift last bits into position */
  tmp.w &= 3;			/* make sure only lower two bits present */
  if (a())			/* if encoder a */
   tmp.b2 = 1;			/* save in decode word */
  if (b())			/* if encoder b */
   tmp.b3 = 1;			/* save in decode word */
  lastDecode = tmp.w;		/* save for next interrupt */
  encoderSet();			/* set encoder output bit */

//  int val = decoder[tmp.w];	/* look up direction */
//  decode.w = tmp.w;
 }
}

#define toggle(cond, set, clr)			\
 if (cond) {set;} else {clr;}

void cmpTmrISR(void)
{
 uint16_t captureVal;
 uint16_t delta;
 if (DBG_CMP_TIME)
  dbgCapIsrSet();

 if (cmpTmrCap1IF())		/* if encoder input pulse */
 {
  captureVal = cmpTmrCap1();	/* read capture value */
  cmpTmrCap1ClrIF();		/* clear interrupt */
  cmpTmrOCP1Clr();		/* clear over capture flag */
  encoderClr();			/* clear encoder bit */
  delta = captureVal - cmpTmr.lastEnc; /* time since last pulse */
  cmpTmr.lastEnc = captureVal;	/* save time of last capture */
  cmpTmr.encPulse -= 1;		/* count off a pulse */
#if ARRAY
  uint32_t cycleClocks = cmpTmr.cycleClocks; /* get cycleclocks */
  uint16_t *p = &cmpTmr.delta[cmpTmr.encPulse]; /* get loc in history array */
  cycleClocks -= *p;		/* subtract old value */
  cycleClocks += delta;		/* add in new value */
  *p = delta;			/* save new value */
#else
  cmpTmr.encClocks += delta;	/* add to total in cycle */
  uint32_t cycleClocks = cmpTmr.encClocks + delta * cmpTmr.encPulse;
#endif
  cmpTmr.cycleClocks = cycleClocks; /* update clocks in a cycle */

  if (DBG_CMP)
  {
   if (DBGTRK2WL0)
   {
    dbgTrk2WL(cmpTmr.encPulse, delta, cycleClocks);
   }
  }

  if (cmpTmr.encPulse <= 0)	/* if end of cycle */
  {
   cmpTmr.encPulse = cmpTmr.encCycLen; /* reset cycle counter */
#if ARRAY == 0
   cmpTmr.encClocks = 0;	/* reset clock accumulator */
#endif
   if (cmpTmr.startInt)		/* if time to start internal timer */
   {
#if START_DELAY == 0
    syncOutSet();		/* set sync out */
    intTmrStart();		/* start timer */
    cmpTmr.startInt = 0;	/* clear flag */
    uint16_t ctr = cycleClocks / cmpTmr.intCycLen;
    cmpTmr.intClocks = ctr;	/* initialize clock counter */
    intTmrSet(ctr - 1);		/* set timer interval */
    syncOutClr();		/* clear sync out */
#else
    intTmrStart();		/* start timer */
    cmpTmr.startInt = 0;	/* clear flag */
#endif

    if (DBG_INT)
    {
     dbgCycEndClr();
#if START_DELAY == 0
     cmpTmr.intCount += 1;	/* count interrupt */
     toggle(cmpTmr.intCount  & 1, dbgIntPSet(), dbgIntPClr());
     if (DBGTRK2WL1)
     {
      dbgTrk2WL(cmpTmr.intPulse, ctr, cmpTmr.intClocks);
     }
#endif
    }
   }
   else if (cmpTmr.measure)	/* if measure flag set */
   {
    cmpTmr.measure = 0;		/* indicate measurement done */
    cmpTmrCap1ClrIE();		/* clear capture 1 interrupt enable */
    cmpTmrStop();		/* stop timer */
   }
   else if (cmpTmr.stop)	/* if time to stop */
   {
    cmpTmr.stop = 0;		/* clear stop flag */
    cmpTmrCap1ClrIE();		/* clear capture 1 interrupt enable */
    cmpTmrStop();		/* stop timer */
   }
   else
   {
    if (DBG_COUNT)
     cmpTmr.missedStart += 1;	/* count missed start */
   }

   if (DBG_COUNT)
   {
    cmpTmr.cycleCount += 1;
    if (DBG_CMP)
    {
     toggle(cmpTmr.cycleCount & 1, dbgCycleSet(), dbgCycleClr());
    }
   }
  }

  if (DBG_COUNT)
  {
   cmpTmr.encCount += 1;	/* count interrupt */
   if (DBG_CMP)
   {
    toggle(cmpTmr.encCount & 1, dbgIntCSet(), dbgIntCClr());
   }
  }
 }

#if 0
 if (cmpTmrIF())		/* if update interrupt */
 {
  cmpTmrClrIF();		/* clear interrupt flag */
 }

 if (cmpTmrCap2IF())		/* if encoder input pulse */
 {
  cmpTmrCap2ClrIF();		/* clear interrupt */
  cmpTmrOCP2Clr();		/* clear over capture flag */
 }
#endif
 if (DBG_CMP_TIME)
  dbgCapIsrClr();
}

void intTmrISR(void)
{
 syncOutSet();
 intTmrClrIF();			/* clear interrupt */
#if START_DELAY == 0
 cmpTmr.intPulse -= 1;		/* count a pulse in cycle */
#endif
 uint32_t ctr  = ((cmpTmr.cycleClocks - cmpTmr.intClocks) /
		  cmpTmr.intPulse);
#if START_DELAY != 0
 cmpTmr.intPulse -= 1;		/* count a pulse in cycle */
#endif
 cmpTmr.intClocks += ctr;	/* update count for next interrupt */

#if START_DELAY == 0
 if (cmpTmr.intPulse > 1)	/* if not last pulse */
 {
  intTmrSet(ctr - 1);		/* set timer interval */
 }
 else				/* if last pulse */
 {
  intTmrStop();			/* stop timer */
  intTmrSet(0xffff);		/* set to maximum */
  intTmrClr();			/* clear counter */
  cmpTmr.intPulse = cmpTmr.intCycLen; /* initialize counter to cycle len */
  cmpTmr.startInt = 1;		/* start on next encoder pulse */

  if (DBG_INT)
   dbgCycEndSet();
 }
#else
 if (cmpTmr.intPulse > 0)	/* if not done */
 {
  intTmrSet(ctr - 1);		/* set timer interval */
 }
 else
 {
  intTmrStop();			/* stop timer */
  intTmrSet(cmpTmr.startDelay);	/* set to start value */
  intTmrClr();			/* clear counter */
  cmpTmr.intPulse = cmpTmr.intCycLen; /* initialize counter to cycle len */
  cmpTmr.intClocks = 0;		/* clear clock counter */
  if (cmpTmr.stop == 0)		/* if time to stop */
   cmpTmr.startInt = 1;		/* start on next encoder pulse */

  if (DBG_INT)
   dbgCycEndSet();
 }
#endif 

 if (DBG_COUNT)
 {
  cmpTmr.intCount += 1;
  if (DBG_INT)
  {
   toggle(cmpTmr.intCount & 1, dbgIntPSet(), dbgIntPClr());
  }
 }

 if (DBG_INT)
 {
  if (DBGTRK2WL1)
  {
   dbgTrk2WL(1000 + cmpTmr.intPulse, ctr, cmpTmr.intClocks);
  }
 }
 syncOutClr();
}

#if ENCODER_TEST

void encTestTmrISR(void)
{
 encTestTmrClrIF();		/* clear interrupt */

 if (encRun)			/* if encoder running */
 {
  if (!encRev)			/* if forwared */
  {
   /* 00   01   11   10   */
   /* 0010 0100 1101 1011 */
   /*    2    4    d    b */
   switch (encState)		/* select on state */
   {
   case 0:			/* 00 */
    aTestSet();			/* 01 0100 4 */
    break;
   case 1:			/* 01 */
    bTestSet();			/* 11 1101 d */
    break;
   case 2:			/* 11 */
    aTestClr();			/* 10 1011 b */
    break;
   case 3:			/* 10 */
    bTestClr();			/* 00 0010 2 */
    break;
   }
  }
  else
  {
   /* 00   10   11   01   */
   /* 0001 1000 1110 0111 */
   /*    1    8    e    7 */
   switch (encState)		/* select on state */
   {
   case 0:			/* 00 */
    bTestSet();			/* 10 1000 8 */
    break;
   case 1:			/* 10 */
    aTestSet();			/* 11 1110 e */
    break;
   case 2:			/* 11 */
    bTestClr();			/* 01 0111 7 */
    break;
   case 3:			/* 01 */
    aTestClr();			/* 00 0001 1 */
    break;
   }
  }
  encState += 1;		/* update state */
  encState &= 0x3;		/* mask in range */

  if (encRunCount != 0)		/* if encoder counting */
  {
   if (--encRunCount == 0)	/* if count is now zero */
   {
    encTestTmrStop();
   }
  }

  encCounter += 1;		/* update counter */
  if (encCounter >= encPulse)	/* if at maximum */
  {
   indexTestSet();		/* set the sync bit */
   encCounter = 0;		/* reset */
   encRevCounter += 1;		/* count a revolution */
  }
  else				/* if not at maximum */
  {
   indexTestClr();		/* clear sync bit */
  }
 }
}

#endif
