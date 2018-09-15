#include "cmptmr.h"
#include "inttmr.h"

#define PIN_DISPLAY 0
#define ENCODER_TEST 0
#define USEC_MIN (60L * 1000000L)

#define DBGPORT USART1
#define REMPORT USART3

#define remoteISR(x) USART3_IRQHandler(x)
#define REMOTE_IRQn USART3_IRQn

#define INT_WIDTH 2		/* internal timer pulse width */

#define indexISR(x) EXTI0_IRQHandler(x)
#define indexIRQn EXTI0_IRQn

#define extISR(x) EXTI1_IRQHandler(x)
#define extIRQn EXTI1_IRQn

#define encoderISR(x) EXTI15_10_IRQHandler(x)
#define aIRQn EXTI14_IRQn
#define bIRQn EXTI15_IRQn

#define ENC_TMR_TIM2		/* encoder test timer */

#define CMP_TMR_TIM4		/* compare timer 4 */

#define INT_TMR_TIM3		/* internal timer 3 */
#define INT_TMR_SCL CMP_TMR_SCL	/* use same pre scaler as compare timer*/
#define INT_TMR_PWM 0		/* internal timer pwm ena / dis */
//#define INT_TIM_PWM1		/* internal timer output pmw 1 */

#include "enctmr.h"
#include "inttmr.h"
#include "cmptmr.h"
#include "pins.h"

//#define readySet() xFlagSet()
//#define readyClr() xFlagSet()
//#define ready() xFlag()

/* start encoder */
#define start() ((ZFlag_Pin & ZFlag_GPIO_Port->IDR) != 0)
#define startNE0() ((ZFlag_Pin & ZFlag_GPIO_Port->IDR) != 0)
#define startEQ0() ((ZFlag_Pin & ZFlag_GPIO_Port->IDR) == 0)

/* encoder ready */
#define readySet() XFlag_GPIO_Port->BSRR = XFlag_Pin
#define readyClr() XFlag_GPIO_Port->BSRR = (XFlag_Pin << 16)
#define ready() ((XFlag_Pin & XFlag_GPIO_Port->ODR) != 0)

//#define start() zFlag()
