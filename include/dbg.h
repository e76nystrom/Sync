#define DBG0 1
#define DBG1 1
#define DBG2 1
#define DBG3 1
#define DBG4 1
#define DBG5 1

#ifdef Dbg0_Pin
#define dbg0Ini()
#define dbg0Set() Dbg0_GPIO_Port->BSRR = Dbg0_Pin
#define dbg0Clr() Dbg0_GPIO_Port->BSRR = (Dbg0_Pin << 16)
#else
#define dbg0Ini()
#define dbg0Set()
#define dbg0Clr()
#endif

#ifdef Dbg1_Pin
#define dbg1Ini()
#define dbg1Set() Dbg1_GPIO_Port->BSRR = Dbg1_Pin
#define dbg1Clr() Dbg1_GPIO_Port->BSRR = (Dbg1_Pin << 16)
#else
#define dbg1Ini()
#define dbg1Set()
#define dbg1Clr()
#endif

#ifdef Dbg2_Pin
#define dbg2Ini()
#define dbg2Set() Dbg2_GPIO_Port->BSRR = Dbg2_Pin
#define dbg2Clr() Dbg2_GPIO_Port->BSRR = (Dbg2_Pin << 16)
#else
#define dbg2Ini()
#define dbg2Set()
#define dbg2Clr()
#endif

#ifdef Dbg3_Pin
#define dbg3Ini()
#define dbg3Set() Dbg3_GPIO_Port->BSRR = Dbg3_Pin
#define dbg3Clr() Dbg3_GPIO_Port->BSRR = (Dbg3_Pin << 16)
#else
#define dbg3Ini()
#define dbg3Set()
#define dbg3Clr()
#endif

#ifdef Dbg4_Pin
#define dbg4Ini()
#define dbg4Set() Dbg4_GPIO_Port->BSRR = Dbg4_Pin
#define dbg4Clr() Dbg4_GPIO_Port->BSRR = (Dbg4_Pin << 16)
#else
#define dbg4Ini()
#define dbg4Set()
#define dbg4Clr()
#endif

#ifdef Dbg5_Pin
#define dbg5Ini()
#define dbg5Set() Dbg5_GPIO_Port->BSRR = Dbg5_Pin
#define dbg5Clr() Dbg5_GPIO_Port->BSRR = (Dbg5_Pin << 16)
#else
#define dbg5Ini()
#define dbg5Set()
#define dbg5Clr()
#endif

/* toggle on capture input */
#ifdef Dbg0_Pin
#define dbgIntCSet() Dbg0_GPIO_Port->BSRR = Dbg0_Pin
#define dbgIntCClr() Dbg0_GPIO_Port->BSRR = (Dbg0_Pin << 16)
#else
#define dbgIntCSet()
#define dbgIntCClr()
#endif

/* toggle on output pulse */
#ifdef Dbg1_Pin
#define dbgIntPSet() Dbg1_GPIO_Port->BSRR = Dbg1_Pin
#define dbgIntPClr() Dbg1_GPIO_Port->BSRR = (Dbg1_Pin << 16)
#else
#define dbgIntPSet()
#define dbgIntPClr()
#endif

/* toggle on input cycle */
#ifdef Dbg2_Pin
#define dbgCycleSet() Dbg2_GPIO_Port->BSRR = Dbg2_Pin
#define dbgCycleClr() Dbg2_GPIO_Port->BSRR = (Dbg2_Pin << 16)
#else
#define dbgCycleSet()
#define dbgCycleClr()
#endif

/* end of internal pulse cycle */
#ifdef Dbg3_Pin
#define dbgCycEndSet() Dbg3_GPIO_Port->BSRR = Dbg3_Pin
#define dbgCycEndClr() Dbg3_GPIO_Port->BSRR = (Dbg3_Pin << 16)
#else
#define dbgCycEndSet()
#define dbgCycEndClr()
#endif

/* length of capture isr */
#ifdef Dbg4_Pin
#define dbgCapIsrSet() Dbg4_GPIO_Port->BSRR = Dbg4_Pin
#define dbgCapIsrClr() Dbg4_GPIO_Port->BSRR = (Dbg4_Pin << 16)
#else
#define dbgCapIsrSet()
#define dbgCapIsrClr()
#endif

/* length of internal isr */
#ifdef Dbg5_Pin
#define dbgIntIsrSet() Dbg5_GPIO_Port->BSRR = Dbg5_Pin
#define dbgIntIsrClr() Dbg5_GPIO_Port->BSRR = (Dbg5_Pin << 16)
#else
#define dbgIntIsrSet()
#define dbgIntIsrClr()
#endif

