#include "ch32fun.h"
#include "stdio.h"
#include "stdbool.h"

#define LED_PORT GPIOD

#define LED_1 2
#define LED_2 4
#define LED_3 6 
#define LED_4 0

static uint8_t leds[4] = {LED_1, LED_2, LED_3, LED_4};

uint16_t duty_cycle[4] = {2, 10, 256, 32};

volatile bool test;
volatile uint16_t count = 0;

// Tim1 is main PWM timer

void TIM1_UP_IRQHandler() __attribute__( ( interrupt() ) );

void TIMER1_INIT( u16 psclr, u16 atlr ) {
	RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
    TIM1->CTLR1 |= TIM_CounterMode_Up | TIM_CKD_DIV1;
    TIM1->CTLR2 = TIM_MMS_1;
    TIM1->ATRLR = atlr;
    TIM1->PSC = psclr;
    TIM1->RPTCR = 0;
    TIM1->SWEVGR = TIM_PSCReloadMode_Immediate;
    TIM1->INTFR = ~TIM_FLAG_Update;
    TIM1->DMAINTENR |= TIM_IT_Update;
    TIM1->CTLR1 |= TIM_CEN;
}

void TIM1_UP_IRQHandler() {
	if ( TIM1->INTFR & ( TIM_IT_Update ) ) {
		printf("interrupted\n");

		TIM1->INTFR &= ~(u16)TIM_IT_Update;
	}
}

/*
 * Example for using AFIO to remap peripheral outputs to alternate configuration
 * 06-01-2023 B. Roy, based on previous work by:
 * 03-28-2023 E. Brombaugh
 * 05-29-2023 recallmenot adapted from Timer1 to Timer2
 *
 * Usage: 
 * Connect LEDs between PD3 and GND, PD4 and GND, PC1 and GND, and PC7 and GND
 * Observe activity on PD3 and PD4, then activity on PC1 and PC7, and back
 *
 * Nutshell:
 * 1. Ensure you're providing a clock to the AFIO peripheral! Save yourself an 
 * 	hour of troubleshooting!
 *	RCC->APB2PCENR |= RCC_APB2Periph_AFIO
 * 2. Apply the remapping configuration bits to the AFIO register:
 * 	AFIO->PCFR1 |= AFIO_PCFR1_TIM2_REMAP_FULLREMAP
 * 3. Go on about your business.
 *
 * /


Timer 2 pin mappings by AFIO->PCFR1
	00	AFIO_PCFR1_TIM2_REMAP_NOREMAP
		D4		T2CH1ETR
		D3		T2CH2
		C0		T2CH3
		D7		T2CH4  --note: requires disabling nRST in opt
	01	AFIO_PCFR1_TIM2_REMAP_PARTIALREMAP1
		C5		T2CH1ETR_
		C2		T2CH2_
		D2		T2CH3_
		C1		T2CH4_
	10	AFIO_PCFR1_TIM2_REMAP_PARTIALREMAP2
		C1		T2CH1ETR_
		D3		T2CH2
		C0		T2CH3
		D7		T2CH4  --note: requires disabling nRST in opt
	11	AFIO_PCFR1_TIM2_REMAP_FULLREMAP
		C1		T2CH1ETR_
		C7		T2CH2_
		D6		T2CH3_
		D5		T2CH4_
*/

/******************************************************************************************
 * initialize TIM2 for PWM
 ******************************************************************************************/
void t2pwm_init( void )
{
	// Enable GPIOC, GPIOD, TIM2, and AFIO *very important!*
	RCC->APB2PCENR |= RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC;
	RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

	GPIOC->CFGLR &= ~(0xf<<(4*5));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF)<<(4*5);

	GPIOC->CFGLR &= ~(0xf<<(4*2));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF)<<(4*2);

	GPIOD->CFGLR &= ~(0xf<<(4*2));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF)<<(4*2);

	GPIOC->CFGLR &= ~(0xf<<(4*1));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF)<<(4*1);
	
	// Reset TIM2 to init all regs
	RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
	RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;
	
	// SMCFGR: default clk input is CK_INT
	// set TIM2 clock prescaler divider 
	TIM2->PSC = 0x000;
	// set PWM total cycle width
	TIM2->ATRLR = 8192;
	
	// for channel 1 and 2, let CCxS stay 00 (output), set OCxM to 110 (PWM I)
	// enabling preload causes the new pulse width in compare capture register only to come into effect when UG bit in SWEVGR is set (= initiate update) (auto-clears)
	TIM2->CHCTLR1 |= TIM_OC1M_2 | TIM_OC1M_1 | TIM_OC1PE;
	TIM2->CHCTLR1 |= TIM_OC2M_2 | TIM_OC2M_1 | TIM_OC2PE;
	TIM2->CHCTLR2 |= TIM_OC3M_2 | TIM_OC3M_1 | TIM_OC3PE;
	TIM2->CHCTLR2 |= TIM_OC4M_2 | TIM_OC4M_1 | TIM_OC4PE;

	// CTLR1: default is up, events generated, edge align
	// enable auto-reload of preload
	TIM2->CTLR1 |= TIM_ARPE;

	// Enable CH1 output, positive pol
	TIM2->CCER |= TIM_CC1E;
	// Enable CH2 output, positive pol
	TIM2->CCER |= TIM_CC2E;
	// Enable CH3 output, positive pol
	TIM2->CCER |= TIM_CC3E;
	// Enable CH4 output, positive pol
	TIM2->CCER |= TIM_CC4E;

	// initialize counter
	TIM2->SWEVGR |= TIM_UG;

	// Enable TIM2
	TIM2->CTLR1 |= TIM_CEN;
}

/*****************************************************************************************
 * set timer channel PW
 *****************************************************************************************/
void t2pwm_setpw(uint8_t chl, uint16_t width)
{
	switch(chl&3)
	{
		case 0: TIM2->CH1CVR = width; break;
		case 1: TIM2->CH2CVR = width; break;
		case 2: TIM2->CH3CVR = width; break;
		case 3: TIM2->CH4CVR = width; break;
	}
	TIM2->SWEVGR |= TIM_UG; // load new value in compare capture register
}


/*****************************************************************************************
 * entry
 *****************************************************************************************/
int main()
{
	
	SystemInit();
	Delay_Ms( 100 );
	printf("\r\r\n\ntim2_pwm example, with remap\n\r");

	// init TIM2 for PWM
	printf("initializing tim2...");
	t2pwm_init();
	AFIO->PCFR1 |= AFIO_PCFR1_TIM2_REMAP_PARTIALREMAP1;
	printf("done.\n\r");
		
	TIMER1_INIT(48, 1000);
	NVIC_EnableIRQ(TIM1_UP_IRQn);

	Delay_Ms(100);

	/*RCC->APB2PCENR |= RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC;


	GPIOC->CFGLR &= ~( 0xf << ( 4 * 1 ) );
	GPIOC->CFGLR &= ~( 0xf << ( 4 * 2 ) );
	GPIOD->CFGLR &= ~( 0xf << ( 4 * 2 ) );
	GPIOC->CFGLR &= ~( 0xf << ( 4 * 5 ) );

	GPIOC->CFGLR |= ( GPIO_CNF_OUT_PP | GPIO_Speed_50MHz ) << ( 4 * 1 );
	GPIOC->CFGLR |= ( GPIO_CNF_OUT_PP | GPIO_Speed_50MHz ) << ( 4 * 2 );
	GPIOD->CFGLR |= ( GPIO_CNF_OUT_PP | GPIO_Speed_50MHz ) << ( 4 * 2 );
	GPIOC->CFGLR |= ( GPIO_CNF_OUT_PP | GPIO_Speed_50MHz ) << ( 4 * 5 );

	GPIOD->OUTDR |= (1<<2);
	Delay_Ms(100);
	GPIOD->OUTDR &= ~(1<<2);
	GPIOC->OUTDR |= (1<<1);
	Delay_Ms(100);
	GPIOC->OUTDR &= ~(1<<1);
	GPIOC->OUTDR |= (1<<2);
	Delay_Ms(100);
	GPIOC->OUTDR &= ~(1<<2);
	GPIOC->OUTDR |= (1<<5);
	Delay_Ms(100);
	GPIOC->OUTDR &= ~(1<<5);*/


	while(1) {
		t2pwm_setpw(0, 1);	
		t2pwm_setpw(1, 1);	
		t2pwm_setpw(2, 12);		
		t2pwm_setpw(3, 12);	

		Delay_Ms(1000);

		t2pwm_setpw(0, 12);	
		t2pwm_setpw(1, 12);	
		t2pwm_setpw(2, 1);		
		t2pwm_setpw(3, 1);	

		Delay_Ms(1000);
		}
}