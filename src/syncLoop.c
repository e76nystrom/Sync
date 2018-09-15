#include "stm32f1xx_hal.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "sync.h"
#include "serialio.h"
#include "lclcmd.h"
#include "remcmd.h"

enum RUN_STATE {ST_IDLE, ST_WAIT_RPM, ST_WAIT_DONE};

void runControl();
int runState;

#if PIN_DISPLAY
void pinDisplay();

typedef struct
{
 char *name;
 GPIO_TypeDef *port;
 int pin;
} T_PINDEF, *P_PINDEF;

#define PIN(name, pin) {#name, pin ## _GPIO_Port, pin ## _Pin}
#define DBGPIN(name) {#name, name ## _GPIO_Port, name ## _Pin}

T_PINDEF pinDef[] =
{
#include "dbgPin.h"
};

#endif

#define DATA_SIZE 1

#if DATA_SIZE

unsigned int getSP();

extern char __bss_start__;
extern char __bss_end__;
extern char __data_start__;
extern char __data_end__;
extern char __stack;
extern char __Main_Stack_Limit;
#endif

void syncLoopSetup()
{
 flushBuf();

 /* initialize timer 11 as index timer */

#if 0

 unsigned int tmrClkFreq = HAL_RCC_GetPCLK2Freq() * 2;

 indexTmrMax(65536);

 #ifdef USEC_SHARED_INDEX
 indexTmrScl((tmrClkFreq / 1000000U) - 1); /* load scaler */
 idxFreq = 1000000U;
 #else
 indexTmrScl(0);
 idxFreq = tmrClkFreq;
 #endif

 idxTrkFreq = idxFreq * 6;
 indexTmrClrIF();
 indexTmrSetIE();
 indexTmrStart();
#endif
}

#define LED_DELAY 500

int16_t syncLoop(void)
{
 uint32_t ledUpdTime;
 unsigned char ch;
 uint32_t extInt[] =
 {
  EXTI0_IRQn,
  EXTI1_IRQn,
  EXTI2_IRQn,
  EXTI3_IRQn,
  EXTI4_IRQn,
  EXTI9_5_IRQn,
  EXTI15_10_IRQn
 };

#if 0
 DBGMCU->APB1FZ = DBGMCU_APB1_FZ_DBG_IWDG_STOP; /* stop wd on debug */
#endif

 uint32_t *p = extInt;		/* external interrupt list */
 int i = sizeof(extInt) / sizeof(uint32_t); /* sizeof list */
 while (--i >= 0)		/* while not at end of list */
  HAL_NVIC_DisableIRQ(*p++);	/* disable external interrupt */

#ifdef REM_ISR
 initRem();
#else
 HAL_NVIC_DisableIRQ(REMOTE_IRQn);
#endif

 initCharBuf();

 putstr("start sync loop\n");
 putstr1("start remcmd\n");

 syncLoopSetup();
 
 #if DATA_SIZE
 unsigned int bss = (unsigned int) (&__bss_end__ - &__bss_start__);
 unsigned int data = (unsigned int) (&__data_end__ - &__data_start__);
 printf("data %u bss %u total %u\n", data, bss, data + bss);
 printf("stack %08x stackLimit %08x sp %08x\n",
	(unsigned int) &__stack, (unsigned int) &__Main_Stack_Limit,
	getSP());
 #endif

 uint32_t clockFreq = HAL_RCC_GetHCLKFreq();
 uint32_t FCY = HAL_RCC_GetPCLK2Freq();
 cfgFcy = FCY;
 clocksMin = (uint64_t) FCY * 60;
 printf("clock frequency %lu FCY %lu %x\n",
	clockFreq, FCY, (unsigned int) &cfgFcy);
 printf("sysTick load %d\n", (int) SysTick->LOAD);

#if PIN_DISPLAY
 pinDisplay();
#endif

 HAL_NVIC_EnableIRQ(EXTI15_10_IRQn); /* encoder interrupts */
 EXTI->PR = A_Pin | B_Pin;

 ledUpdTime = millis();
 ledSet();
 readySet();
 while (1)			/* main loop */
 {
  newline();
  while (1)			/* input background loop */
  {
   runControl();		/* call run control state machine */

   uint32_t t = millis();
   if ((t - ledUpdTime) > LED_DELAY) /* if time to flash led */
   {
    ledUpdTime = t;
    if (led())
     ledClr();
    else
     ledSet();
   }

#if 0
   if (encoder())		/* if encoder set */
   {
    if (encTmo <= 0)		/* if timed out */
    {
     encoderClr();		/* clear biy */
    }
    else			/* if not timed out */
    {
     encTmo -= 1;		/* count another interval */
    }
   }
   else				/* if not set */
   {
    encTmo = 2;			/* reset timer */
   }
#endif   
   pollBufChar();		/* check for data to output */
   if (chRdy())			/* if character available */
   {
    ch = chRead();		/* return it */
    if (ch == 0x11)		/* if xon */
     continue;			/* no echo */
    if (ch == 0x13)		/* if xoff */
     continue;			/* no echo */
    putBufChar(ch);		/* echo input */
    break;
   }

#if REM_ISR == 0
   if (chRdy1())		/* if character on remote link */
   {
    ch = chRead1();		/* read character */
    if (ch == 1)		/* if control a */
    {
     remcmd();			/* process remote command */
    }
   }
   else
   {
    pollBufChar();		/* check for data to output */
    if (chRdy())		/* if character available */
    {
     ch = chRead();		/* return it */
#if 0
     if (ch == 3)		/* if control c */
     {
      setupDone = 1;		/* force setup done */
     }
#endif
    }
   }
#endif
  }

  flushBuf();
  lclcmd(ch);			/* local commands */
  flushBuf();
#if REM_ISR == 0
  if (remcmdTimeout < UINT_MAX)
   remcmdUpdateTime = millis();
#endif
 }
}

void runControl()
{
 switch(runState)
 {
 case ST_IDLE:			/* 0 idle */
  if (startEQ0())		/* if time to start */
  {
   encoderMeasure();		/* start rpm measurement */
   runState = ST_WAIT_RPM;	/* wait for measurement */
  }
  break;

 case ST_WAIT_RPM:		/* 1 wait for rpm measruement */
  if (cmpTmr.measure == 0)	/* if measurement complete */
  {
   encoderCalculate();		/* calculate actual prescaler */
   encoderStart();		/* start encoder */
   printf("PA3 %d\n", ((XFlag_Pin & XFlag_GPIO_Port->ODR) != 0));
   printf("readyClr()\n");
//   xFlagClr();
   readyClr();			/* set ready bit */
   printf("PA3 %d\n", ((XFlag_Pin & XFlag_GPIO_Port->ODR) != 0));
   runState = ST_WAIT_DONE;	/* wait for done */
  }
  break;

 case ST_WAIT_DONE:		/* 2 wait for start cleared */
  if (startNE0())		/* if start bit cleared */
  {
   printf("PA3 %d\n", ((XFlag_Pin & XFlag_GPIO_Port->ODR) != 0));
   printf("readySet()\n");
//   xFlagSet();
   readySet();			/* clear ready bit */
   printf("PA3 %d\n", ((XFlag_Pin & XFlag_GPIO_Port->ODR) != 0));
   runState = ST_IDLE;		/* return to idle state */
  }
  break;
 }
}

#if PIN_DISPLAY

#define CON_SIZE (sizeof(conDef) / sizeof(T_CONDEF))
#define CON_PINS (CON_SIZE / 2)

void pinDisplay()
{
 P_PINDEF pin = pinDef;
 for (unsigned int i = 0; i < (sizeof(pinDef) / sizeof(T_PINDEF)); i++)
 {
  char t0[8];
  T_PIN_NAME val;
  val = pinName(t0, pin->port, pin->pin);
//  printf("port %08x pin %08x gpio %c pin %2d\n",
//	 (unsigned int) pin->port, (unsigned int) pin->pin,
//	 val.port, val.num);
   
  P_CONDEF con = conDef;
  char *connector = "";
  unsigned int j;
  for (j = 0; j < CON_SIZE; j++)
  {
   if ((val.port == con->port)
   &&  (val.num == con->pin))
   {
    connector = j < CON_PINS ? "L" : "R";
    if (j > CON_PINS)
     j -= CON_PINS;
    j += 1;
    break;
   }
   con++;
  }

  char t1[6];
  char t2[40];
  sprintf(t1, "%s %d", connector, j);
  printf("%-9s %-5s %-4s %s\n",
	 pin->name, t0, t1, gpioStr(t2, sizeof(t2), &val));
  pin++;
 }
}

#endif

void hard_fault_handler_c (unsigned int * hardfault_args)
{
 unsigned int stacked_r0;
 unsigned int stacked_r1;
 unsigned int stacked_r2;
 unsigned int stacked_r3;
 unsigned int stacked_r12;
 unsigned int stacked_lr;
 unsigned int stacked_pc;
 unsigned int stacked_psr;
 
 stacked_r0 = ((unsigned long) hardfault_args[0]);
 stacked_r1 = ((unsigned long) hardfault_args[1]);
 stacked_r2 = ((unsigned long) hardfault_args[2]);
 stacked_r3 = ((unsigned long) hardfault_args[3]);
 
 stacked_r12 = ((unsigned long) hardfault_args[4]);
 stacked_lr = ((unsigned long) hardfault_args[5]);
 stacked_pc = ((unsigned long) hardfault_args[6]);
 stacked_psr = ((unsigned long) hardfault_args[7]);
 
 dbgBuffer = 0;
 
 printf("\n\n[Hard fault handler - all numbers in hex]\n");
 printf("R0 = %x\n", stacked_r0);
 printf("R1 = %x\n", stacked_r1);
 printf("R2 = %x\n", stacked_r2);
 printf("R3 = %x\n", stacked_r3);
 printf("R12 = %x\n", stacked_r12);
 printf("LR [R14] = %x  subroutine call return address\n", stacked_lr);
 printf("PC [R15] = %x  program counter\n", stacked_pc);
 printf("PSR = %x\n", stacked_psr);
 printf("BFAR = %x\n", (*((volatile unsigned int *) (0xE000ED38))));
 printf("CFSR = %x\n", (*((volatile unsigned int *) (0xE000ED28))));
 printf("HFSR = %x\n", (*((volatile unsigned int *) (0xE000ED2C))));
 printf("DFSR = %x\n", (*((volatile unsigned int *) (0xE000ED30))));
 printf("AFSR = %x\n", (*((volatile unsigned int *) (0xE000ED3C))));
 printf("SCB_SHCSR = %x\n", (unsigned int) SCB->SHCSR);
  
 while (1);
}

#if 0
 flushBuf();
 P_DBG d = dbgDef;
 for (unsigned int i = 0; i < (sizeof(dbgDef) / sizeof(T_DBG)); i++)
 {
  P_GPIO g = gpio;
  for (unsigned int j = 0; j < (sizeof(gpio) / sizeof(T_GPIO)); j++)
  {
   if (d->port == g->port)
   {
    int mask = 1;
    int k;
    for (k = 0; k < 15; k++)
    {
     if (mask == d->pin)
     {
      printf("dbg%d p%s%d\n", i, g->name, k);
      break;
     }
     mask <<= 1;
    }
    break;
   }
   g++;
  }
  d++;
 }
#endif
