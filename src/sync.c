/******************************************************************************/
#if !defined(INCLUDE)
#include "stm32f1xx_hal.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <limits.h>

#include "remvar.h"
#include "serialio.h"
#define EXT
#endif

#include "config.h"
#include "dbg.h"

#define DBG_CMP 1		/* debug capture timer */
#define DBG_CMP_TIME 1		/* debug capture interrupt timing */
#define DBG_INT 1		/* debug internal timer */
#define DBG_COUNT 1		/* debug count input and output pulses */

#define ARRAY 1			/* use array for cycle length */
#define ARRAY_LEN 2048		/* size of array */
#define START_DELAY 2		/* delay internal start */

#define CALC_STEP_WIDTH(x) ((uint16_t) ((cfgFcy * x) / 1000000l))

typedef uint8_t boolean;

#include "dbgtrk.h"

unsigned int millis();

#define DIR_POS      1           /* 0x01 positive direction */
#define DIR_NEG      -1          /* 0x-1 negative direction */

#define MAX_TIMEOUT UINT_MAX
#define REMCMD_TIMEOUT 1000U
#define INDEX_TIMEOUT 1500U

EXT unsigned int remcmdUpdateTime;
EXT unsigned int remcmdTimeout;

EXT uint32_t cfgFcy;
EXT uint64_t clocksMin;		/* timer clocks per minute */

EXT int16_t encTmo;		/* encoder pulse timeout */

typedef struct
{
 uint32_t encCount;		/* encoder counts */
 uint32_t intCount;		/* internal counts */
 uint32_t cycleCount;		/* encoder cycle counter */
 int missedStart;		/* start flag missed */
 
 int16_t encCycLen;		/* encoder cycle length */
 int16_t encPulse;		/* encoder pulse number */
 uint16_t lastEnc;		/* last encoder capture */
 uint32_t encClocks;		/* clocks in current encoder cycle */
 uint32_t cycleClocks;		/* estimated clocks in cycle */

 uint16_t preScale;		/* counter pre scaler */

 int intCycLen;			/* internal cycle length */
 int intPulse;			/* internal pulse number */
 uint32_t intClocks;		/* clocks in current internal cycle */

 boolean startInt;		/* start internal timer */
 boolean measure;		/* measure flag */
 boolean stop;			/* stop flag */

#if START_DELAY
 int16_t startDelay;		/* initial delay */
#endif

#if ARRAY
 uint16_t delta[ARRAY_LEN];	/* saved delta values */
#endif
} T_CMP_TMR, *P_CMP_TMR;

EXT T_CMP_TMR cmpTmr;

void encoderSetup();
void encoderMeasure();
void encoderCalculate();
void encoderStart();
void encoderStop();

#if ENCODER_TEST

EXT uint32_t testPulseMin;
EXT uint16_t testEncPreScaler;
EXT uint16_t testEncCount;
EXT int rpm;
EXT char encRun;
EXT int encRunCount;
EXT int encCounter;
EXT int encLines;
EXT int encPulse;
EXT int encRevCounter;
EXT char encState;
EXT char encRev;

void testEncoderStart();
void testEncoderStop();

#endif

typedef struct
{
 union
 {
  struct
  {
   char port;
   char num;
  };
  struct
  {
   uint16_t pinName;
  };
 };
} T_PIN_NAME;

T_PIN_NAME pinName(char *buf, GPIO_TypeDef *port, int pin);
char *gpioStr(char *buf, int size, T_PIN_NAME *pinInfo);
void gpioInfo(GPIO_TypeDef *gpio);
void tmrInfo(TIM_TypeDef *tmr);
void extiInfo();
void usartInfo(USART_TypeDef *usart, char *str);
void i2cInfo(I2C_TypeDef *i2c, char *str);

#if !defined(INCLUDE)

extern uint32_t uwTick;

unsigned int millis()
{
 return((unsigned int) uwTick);
}

void encoderSetup()
{
 cmpTmr.encCycLen = syncCycle;
 cmpTmr.encCycLen = syncOutput;
 cmpTmr.preScale = syncPrescaler;
}

void encoderMeasure()
{
 printf("encoderMeasure\n");
 encoderStop();			/* stop encoder */

#if ARRAY
 cmpTmr.encCycLen = ARRAY_LEN;	/* initialize cycle length */
 cmpTmr.cycleClocks = 0;	/* clear cycle clocks */
 cmpTmr.lastEnc = 0;		/* clear last encoder vale */
 memset(&cmpTmr.delta, 0, sizeof(cmpTmr.delta)); /* clear delta array */
#else
 cmpTmr.encCycLen = 2048;	/* init cycle length */
#endif

 cmpTmr.encPulse = cmpTmr.encCycLen; /* set number to count */
 cmpTmr.measure = 1;		/* set measurement flag */

 cmpTmrClrIE();			/* disable update interrupts */
 cmpTmrClr();			/* clear counter */
 cmpTmrSet(0xffff);		/* set count to maximum */
 cmpTmrScl(0);			/* set prescaler */
 cmpTmrCap1EnaSet();		/* enable capture from encoer */
 cmpTmrCap1SetIE();		/* enable capture interrupt */
 cmpTmr.intClocks = 0;		/* clear clocks in current cycle */
 cmpTmrStart();			/* start capture timer */
}

void encoderCalculate()
{
 printf("encoderCalculate\n");
 uint64_t n = clocksMin * cmpTmr.encCycLen;
 uint64_t d = ((uint64_t) cmpTmr.cycleClocks * syncEncoder);
 uint16_t rpm = (uint16_t) (n / d);

 uint32_t pulseMinIn = syncEncoder * rpm;
 uint32_t pulseMinOut = (pulseMinIn * syncOutput) / syncCycle;
 uint32_t clocksPulse = (uint32_t) (clocksMin / pulseMinOut);
 syncPrescaler = clocksPulse >> 16;
 printf("n %lld d %lld rpm %d preScaler %d\n",
	n, d, rpm, syncPrescaler);
 syncPrescaler += 1;
}

void encoderStart()
{
 encoderStop();			/* stop encoder */

 cmpTmr.encCycLen = syncCycle;
 cmpTmr.intCycLen = syncOutput;
 cmpTmr.preScale = syncPrescaler;

 printf("encoderStart cycle %d output %d preScale %u\n",
	cmpTmr.encCycLen, cmpTmr.intCycLen, cmpTmr.preScale);
 intTmrClr();			/* clear counter */
#if START_DELAY == 0
 intTmrSet(0xffff);		/* set count to maximum */
#else
 cmpTmr.startDelay = (uint16_t) ((cfgFcy * START_DELAY) / 1000000l - 1);
 intTmrSet(cmpTmr.startDelay);	/* set to initial delay */
#endif
 intTmrScl(cmpTmr.preScale - 1); /* set prescaler */
 intTmrSetIE();			/* enable interrupts */

#if ARRAY
 cmpTmr.cycleClocks = 0;	/* clear cycle clocks */
 cmpTmr.lastEnc = 0;		/* clear last encoder vale */
 memset(&cmpTmr.delta, 0, sizeof(cmpTmr.delta)); /* clear delta array */
#endif

 cmpTmr.encClocks = 0;		/* clear clocks in current cycle */

 cmpTmr.encPulse = cmpTmr.encCycLen; /* initialize encoder counter */
 cmpTmr.intPulse = cmpTmr.intCycLen; /* initialize internal counter */

#if DBG_COUNT
 cmpTmr.encCount = 0;		/* clear counters */
 cmpTmr.intCount = 0;
 cmpTmr.cycleCount = 0;
 cmpTmr.missedStart = 0;	/* clear missed flag */
#endif

 cmpTmrClrIE();			/* disable update interrupts */
 cmpTmrClr();			/* clear counter */
 cmpTmrSet(0xffff);		/* set count to maximum */
 cmpTmrScl(cmpTmr.preScale - 1); /* set prescaler */
 cmpTmrCap1EnaSet();		/* enable capture from encoer */
 cmpTmrCap1SetIE();		/* enable capture interrupt */
 cmpTmr.intClocks = 0;		/* clear clocks in current cycle */
 cmpTmr.measure = 0;		/* clear measure flag */
 cmpTmr.stop = 0;		/* clear stop flag */
 cmpTmr.startInt = 1;		/* start internal timer */
 if (DBG_INT)
  dbgCycEndSet();
#if DBGTRK
 dbgTrk = true;
#endif
 cmpTmrStart();			/* start capture timer */

#if 0
 printf("internal timer\n");
 tmrInfo(INT_TIM);
 printf("compare timer\n");
 tmrInfo(CMP_TIM);
#endif
}

void encoderStop()
{
 printf("encoder stop\n");
 cmpTmrStop();			/* stop timer */
 cmpTmrClr();
 cmpTmrClrIE();			/* clear interrupts */
 cmpTmrClrIF();

 cmpTmrCap1ClrIE();		/* clear capture 1 interrupts */
 cmpTmrCap1ClrIF();
 cmpTmrOCP1Clr();

#if 0
 cmpTmrCap2ClrIE();		/* clear capture 2 interrupts */
 cmpTmrCap2ClrIF();
 cmpTmrOCP2Clr();
#endif

#if 0
 printf("compare timer\n");
 tmrInfo(CMP_TIM);
#endif

 intTmrStop();
 intTmrClr();
 intTmrClrIE();
 intTmrClrIF();
#if INT_TMR_PWM
 intTmrPWMDis();
#endif
}

#if ENCODER_TEST

void testEncoderStart()
{
 if (testEncCount != 0)
 {
  encState = 0;
  encCounter = 0;
  encRevCounter = 0;
  encRunCount = 0;
  encRun = 1;

  aTestClr();
  bTestClr();
  indexTestClr();
 
  encTestTmrScl(testEncPreScaler);
  encTestTmrMax(testEncCount);
  encTestTmrSetIE();
  encTestTmrStart();

  printf("test encoder start\n");
 }
 else
 {
  printf("encoder count not initialized\n");
 }
}

void testEncoderStop()
{
 encRun = 0;
 encTestTmrStop();
 encTestTmrClr();
 encTestTmrClrIE();
 encTestTmrClrIF();
 printf("encoder stop\n");
}

#endif
typedef struct
{
 GPIO_TypeDef *port;
 char name;
} T_GPIO, *P_GPIO;

T_GPIO gpio[] = 
{
 {GPIOA, 'A'},
 {GPIOB, 'B'},
 {GPIOC, 'C'},
 {GPIOD, 'D'},
 {GPIOE, 'E'},
};

char portName(GPIO_TypeDef *port)
{
 for (unsigned int j = 0; j < sizeof(gpio) / sizeof(T_GPIO); j++)
 {
  if (port == gpio[j].port)
  {
   return(gpio[j].name);
  }
 }
 return('*');
}

T_PIN_NAME pinName(char *buf, GPIO_TypeDef *port, int pin)
{
 char pName = portName(port);
 T_PIN_NAME val;
 val.port = pName;
 int pinNum = 0;
 while (pin != 0)
 {
  if (pin & 1)
   break;
  pin >>= 1;
  pinNum++;
 }
 sprintf(buf, "P%c%d", pName, pinNum);
 val.num = pinNum;
 return(val);
}

char modeInfo[] = {'I', 'O', 'F', 'A'};
char *typeInfo[] = {"PP", "OD", "  "};
char *speedInfo[] = {"LS", "MS", "HS", "VH", "  "};
char *pupdInfo[] = {"  ", "PU", "PD", "**"};

char *gpioStr(char *buf, int size, T_PIN_NAME *pinInfo)
{
 buf[0] = 0;
 for (unsigned int j = 0; j < sizeof(gpio) / sizeof(T_GPIO); j++)
 {
  if (gpio[j].name == pinInfo->port)
  {
#if 0
   GPIO_TypeDef *port = gpio[j].port;
   int pin = pinInfo->num;
//   printf("port  %08x %2d %c %2d\n", (unsigned int) port, pin,
//	  pinInfo->port, pinInfo->num);

   int mode = (port->MODER >> (pin << 1)) & 3;
//   printf("mode  %08x %d\n", (unsigned int) port->MODER, mode);

   int outType = (port->OTYPER >> pin) & 1;
//   printf("type  %08x %d\n", (unsigned int) port->OTYPER, outType);

   int outSpeed = (port->OSPEEDR >> (pin << 1)) & 3;
//   printf("speed %08x %d\n", (unsigned int) port->OSPEEDR, outSpeed);

   int pupd = (port->PUPDR >> (pin << 1)) & 3;
//   printf("pupd  %08x %d\n", (unsigned int) port->PUPDR, pupd);

   int afr = (port->AFR[pin >> 3] >> ((pin << 2) & 0x1f)) & 0xf;
   char interrupt = ' ';
   if (mode == GPIO_MODE_INPUT)
   {
    outType = (sizeof(typeInfo) / sizeof(char *)) - 1;
    outSpeed = (sizeof(speedInfo) / sizeof(char *)) - 1;

    if ((EXTI->IMR >> pin) & 1)
    {
     int exti = (SYSCFG->EXTICR[pin >> 2] >> ((pin << 2) & 0xf)) & 0xf;
     if ((pinInfo->port - 'A') == exti)
      interrupt = 'I';
//     printf("exti %2d pinInfo->port - 'A' %d pin >> 2 %d pin << 2 %d\n",
//	    exti, pinInfo->port - 'A', pin >> 2, pin << 2);
    }
   }

//   printf("afr   %08x %d (pin >> 3) %d ((pin << 2) & 0x1f) %2d\n",
//	  (unsigned int) port->AFR[pin >> 3], afr,
//	  (pin >> 3), ((pin << 2) & 0x1f));
//   flushBuf();

   snprintf(buf, size, "%c %c %2s %2s %2s %2d",
	    interrupt, modeInfo[mode], typeInfo[outType],
	    speedInfo[outSpeed], pupdInfo[pupd], afr);
#else
   snprintf(buf, size, "\n");
#endif
   break;
  }
 }
 return(buf);
}

void gpioInfo(GPIO_TypeDef *gpio)
{
 printf("gpio %x %c\n",(unsigned int) gpio, portName(gpio));
#if 0
 printf("MODER   %8x ",(unsigned int) gpio->MODER);
 printf("OTYPER  %8x\n",(unsigned int) gpio->OTYPER);
 printf("OSPEEDR %8x ",(unsigned int) gpio->OSPEEDR);
 printf("PUPDR   %8x\n",(unsigned int) gpio->PUPDR);
#else
 printf("CRL    %8x ",(unsigned int) gpio->CRL);
 printf("CRH    %8x\n",(unsigned int) gpio->CRH);
#endif
 printf("IDR     %8x ",(unsigned int) gpio->IDR);
 printf("ODR     %8x\n",(unsigned int) gpio->ODR);
 printf("BSRR    %8x ",(unsigned int) gpio->BSRR);
 printf("LCKR    %8x\n",(unsigned int) gpio->LCKR);
#if 0
 printf("AFR[0]  %8x ",(unsigned int) gpio->AFR[0]);
 printf("AFR[1]  %8x\n",(unsigned int) gpio->AFR[1]);
#endif
 int i;
 printf("         ");
 for (i = 0; i < 16; i++)
  printf(" %2d", i);

 int val;
 
#if 0
 printf("\nmoder    ");
 val = gpio->MODER;
 for (i = 0; i < 16; i++)
  printf(" %2d", (val >> (2 * i)) & 0x3);

 printf("\notyper   ");
 val = gpio->OTYPER;
 for (i = 0; i < 16; i++)
  printf(" %2d", (val >> i) & 0x1);

 printf("\nopspeedr ");
 val = gpio->OSPEEDR;
 for (i = 0; i < 16; i++)
  printf(" %2d", (val >> (2 * i)) & 0x3);

 printf("\npupdr    ");
 val = gpio->PUPDR;
 for (i = 0; i < 16; i++)
  printf(" %2d", (val >> (2 * i)) & 0x3);
#else
 printf("\nmode     ");
 val = gpio->CRL;
 for (i = 0; i < 8; i++)
  printf(" %2d", (val >> (4 * i)) & 0x3);

 val = gpio->CRH;
 for (i = 0; i < 8; i++)
  printf(" %2d", (val >> (4 * i)) & 0x3);

 printf("\ncnf      ");
 val = gpio->CRL;
 for (i = 0; i < 8; i++)
  printf(" %2d", (val >> ((4 * i) + 2)) & 0x3);

 val = gpio->CRH;
 for (i = 0; i < 8; i++)
  printf(" %2d", (val >> ((4 * i) + 2)) & 0x3);
#endif

 printf("\nidr      ");
 val = gpio->IDR;
 for (i = 0; i < 16; i++)
  printf(" %2d", (val >> i) & 0x1);

 printf("\nodr      ");
 val = gpio->ODR;
 for (i = 0; i < 16; i++)
  printf(" %2d", (val >> i) & 0x1);

#if 0
 printf("\nafr      ");
 val = gpio->AFR[0];
 for (i = 0; i < 8; i++)
  printf(" %2d", (val >> (4 * i)) & 0xf);
 val = gpio->AFR[1];
 for (i = 0; i < 8; i++)
  printf(" %2d", (val >> (4 * i)) & 0xf);
#endif
 printf("\n");
 flushBuf();
}

typedef struct
{
 TIM_TypeDef *tmr;
 char num;
} T_TIM, *P_TIM;

T_TIM tim[] =
{
 {TIM1,  1},
 {TIM2,  2},
 {TIM3,  3},
 {TIM4,  4},
#if 0
 {TIM5,  5},
#ifdef TIM6
 {TIM6,  6},
#endif
#ifdef TIM7
 {TIM7,  7},
#endif
#ifdef TIM8
 {TIM8,  8},
#endif
 {TIM9,  9},
 {TIM10, 10},
 {TIM11, 11},
#ifdef TIM12
 {TIM12, 12},
#endif
#ifdef TIM13
 {TIM13, 13},
#endif
#ifdef TIM14
 {TIM14, 14},
#endif
#endif
};

char timNum(TIM_TypeDef *tmr)
{
 for (unsigned int j = 0; j < sizeof(tim) / sizeof(T_TIM); j++)
 {
  if (tmr == tim[j].tmr)
  {
   return(tim[j].num);
  }
 }
 return(0);
}

void tmrInfo(TIM_TypeDef *tmr)
{
 printf("tmr %x TIM%d\n",(unsigned int) tmr, timNum(tmr));
 printf("CR1   %8x ",(unsigned int) tmr->CR1);
 printf("CR2   %8x\n",(unsigned int) tmr->CR2);
 printf("SMCR  %8x ",(unsigned int) tmr->SMCR);
 printf("DIER  %8x\n",(unsigned int) tmr->DIER);
 printf("SR    %8x ",(unsigned int) tmr->SR);
 printf("EGR   %8x\n",(unsigned int) tmr->EGR);
 printf("CCMR1 %8x ",(unsigned int) tmr->CCMR1);
 printf("CCMR2 %8x\n",(unsigned int) tmr->CCMR2);
 printf("CCER  %8x ",(unsigned int) tmr->CCER);
 printf("CNT   %8x\n",(unsigned int) tmr->CNT);
 printf("PSC   %8x ",(unsigned int) tmr->PSC);
 printf("ARR   %8x\n",(unsigned int) tmr->ARR);
 printf("RCR   %8x ",(unsigned int) tmr->RCR);
 printf("CCR1  %8x\n",(unsigned int) tmr->CCR1);
 printf("CCR2  %8x ",(unsigned int) tmr->CCR2);
 printf("CCR3  %8x\n",(unsigned int) tmr->CCR3);
 printf("CCR4  %8x ",(unsigned int) tmr->CCR4);
 printf("BDTR  %8x\n",(unsigned int) tmr->BDTR);
 printf("DCR   %8x ",(unsigned int) tmr->DCR);
 printf("OR    %8x\n",(unsigned int) tmr->OR);
 flushBuf();
}

void extiInfo()
{
 printf("EXTI %x\n",(unsigned int) EXTI);
 int i;
 printf("      ");
 for (i = 0; i <= 22; i++)
  printf(" %2d", i);

 printf("\nIMR   ");
 int val = EXTI->IMR;
 for (i = 0; i <= 22; i++)
  printf(" %2d", (val >> i) & 0x1);

 printf("\nEMR   ");
 val = EXTI->EMR;
 for (i = 0; i <= 22; i++)
  printf(" %2d", (val >> i) & 0x1);

 printf("\nRTSR  ");
 val = EXTI->RTSR;
 for (i = 0; i <= 22; i++)
  printf(" %2d", (val >> i) & 0x1);

 printf("\nFTSR  ");
 val = EXTI->FTSR;
 for (i = 0; i <= 22; i++)
  printf(" %2d", (val >> i) & 0x1);

 printf("\nSWIER ");
 val = EXTI->SWIER;
 for (i = 0; i <= 22; i++)
  printf(" %2d", (val >> i) & 0x1);

 printf("\nPR    ");
 val = EXTI->PR;
 for (i = 0; i <= 22; i++)
  printf(" %2d", (val >> i) & 0x1);

#if 0
 printf("\nSYSCFG %x\n",(unsigned int) SYSCFG);
 printf("      ");
 for (i = 0; i < 16; i++)
  printf(" %2d", i);
#endif

#if 0
 printf("\nEXTICR");
 int mask = EXTI->IMR;
 for (i = 0; i < 4; i++)
 {
  val = SYSCFG->EXTICR[i];
  int j;
  for (j = 0; j < 4; j++)
  {
   printf("  %c", (mask & 1) ? 'a' + ((val >> (4 * j)) & 0xf) : ' ');
   mask >>= 1;
  }
 }
#endif
 printf("\n");
 flushBuf();
}

void usartInfo(USART_TypeDef *usart, char *str)
{
 printf("usart %x %s\n",(unsigned int) usart, str);
 printf("SR   %8x ",(unsigned int) usart->SR);
 printf("DR   %8x\n",(unsigned int) usart->DR);
 printf("BRR  %8x ",(unsigned int) usart->BRR);
 printf("CR1  %8x\n",(unsigned int) usart->CR1);
 printf("CR2  %8x ",(unsigned int) usart->CR2);
 printf("CR3  %8x\n",(unsigned int) usart->CR3);
 printf("GTPR %8x\n",(unsigned int) usart->GTPR);
 flushBuf();
}

void i2cInfo(I2C_TypeDef *i2c, char *str)
{
 printf("i2c %x %s\n",(unsigned int) i2c, str);
 printf("CR1   %8x ",(unsigned int) i2c->CR1);
 printf("CR2   %8x\n",(unsigned int) i2c->CR2);
 printf("OAR1  %8x ",(unsigned int) i2c->OAR1);
 printf("OAR2  %8x\n",(unsigned int) i2c->OAR2);
 printf("SR1   %8x ",(unsigned int) i2c->SR1);
 printf("SR2   %8x\n",(unsigned int) i2c->SR2);
 printf("DR    %8x ",(unsigned int) i2c->DR);
 printf("CCR   %8x\n",(unsigned int) i2c->CCR);
 printf("TRISE %8x\n",(unsigned int) i2c->TRISE);
 flushBuf();
}

#endif
