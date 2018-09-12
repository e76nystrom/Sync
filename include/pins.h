/* encoder pulse output */
#define encoderSet() Encoder_GPIO_Port->BSRR = Encoder_Pin
#define encoderClr() Encoder_GPIO_Port->BSRR = (Encoder_Pin << 16)
#define encoder() ((Encoder_Pin & Encoder_GPIO_Port->ODR) != 0)

/* sync signal output */
#define syncOutSet() SyncOut_GPIO_Port->BSRR = SyncOut_Pin
#define syncOutClr() SyncOut_GPIO_Port->BSRR = (SyncOut_Pin << 16)
#define syncOut() ((SyncOut_Pin & SyncOut_GPIO_Port->ODR) != 0)

/* start encoder */
#define zFlag() ((ZFlag_Pin & ZFlag_GPIO_Port->IDR) != 0)

/* encoder ready */
#define xFlagSet() XFlag_GPIO_Port->BSRR = XFlag_Pin
#define xFlagClr() XFlag_GPIO_Port->BSRR = (XFlag_Pin << 16)
#define xFlag() ((XFlag_Pin & XFlag_GPIO_Port->ODR) != 0)

/* led output */
#define ledSet() Led_GPIO_Port->BSRR = Led_Pin
#define ledClr() Led_GPIO_Port->BSRR = (Led_Pin << 16)
#define led() ((Led_Pin & Led_GPIO_Port->ODR) != 0)

/* a encoder input */
#define a() ((A_Pin & A_GPIO_Port->IDR) != 0)

/* b encoder input */
#define b() ((B_Pin & B_GPIO_Port->IDR) != 0)

/* a test signal */
#define aTestSet() ATest_GPIO_Port->BSRR = ATest_Pin
#define aTestClr() ATest_GPIO_Port->BSRR = (ATest_Pin << 16)
#define aTest() ((ATest_Pin & ATest_GPIO_Port->ODR) != 0)

/* b test signal */
#define bTestSet() BTest_GPIO_Port->BSRR = BTest_Pin
#define bTestClr() BTest_GPIO_Port->BSRR = (BTest_Pin << 16)
#define bTest() ((BTest_Pin & BTest_GPIO_Port->ODR) != 0)

/* sync test signal */
#define indexTestSet() IndexTest_GPIO_Port->BSRR = IndexTest_Pin
#define indexTestClr() IndexTest_GPIO_Port->BSRR = (IndexTest_Pin << 16)
#define indexTest() ((IndexTest_Pin & IndexTest_GPIO_Port->ODR) != 0)

