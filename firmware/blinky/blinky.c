#include "ch32fun.h"
#include "stdio.h"
#include "stdbool.h"

#define LED_1 0
#define LED_2 1
#define LED_3 3 
#define LED_4 2

typedef struct mode
{
	uint8_t start_brightness[4];
	uint8_t brightness_change[4];
	uint8_t max_brightness[4];
	uint8_t min_brightness[4];
	uint8_t phase_offset[4];
	bool sawtooth;
	uint32_t refresh;
};

#define NUM_MODES 7
static struct mode modes[NUM_MODES] = {
	{.start_brightness = {0}, .brightness_change = {0, 0, 0, 0}, .refresh = 20000},
	{.start_brightness = {10, 10, 10, 10}, .max_brightness = {10, 10, 10, 10}, .brightness_change = {0, 0, 0, 0}, .refresh = 20000},
	{.brightness_change = {2, 3, 5, 7}, .max_brightness = {51, 49, 50, 47}, .phase_offset = {1, 2, 3, 5}, .sawtooth = false, .refresh = 1000},
	{.brightness_change = {2, 3, 5, 7}, .max_brightness = {25, 24, 26, 27}, .phase_offset = {1, 2, 3, 5}, .sawtooth = false, .refresh = 1000},
	{.brightness_change = {1, 1, 1, 1}, .max_brightness = {25, 25, 25, 25}, .phase_offset = {0, 0, 0, 0}, .sawtooth = false, .refresh = 1000},
	{.brightness_change = {1, 1, 1, 1}, .max_brightness = {28, 28, 28, 28}, .phase_offset = {0, 14, 28, 42}, .sawtooth = false, .refresh = 1000},
	{.brightness_change = {1, 1, 1, 1}, .max_brightness = {25, 25, 25, 25},.min_brightness = {5, 5, 5, 5}, .phase_offset = {0, 0, 0, 0}, .sawtooth = false, .refresh = 1000}

};

volatile uint8_t current_mode = 0;
volatile bool updown[4] = {true};


volatile uint8_t duty_cycle[4] = {0};

void update_brightness(){
	if(modes[current_mode].sawtooth)
	// Simple mod adddition in sawtooth mode
		for(uint8_t i=0; i<4; i++){
			duty_cycle[i] = (duty_cycle[i]+ modes[current_mode].brightness_change[i])%(modes[current_mode].max_brightness[i] - modes[current_mode].min_brightness[i]) + modes[current_mode].min_brightness[i];
			updown[i] = true;
		}
	else
	{
		for(uint8_t i=0; i<4; i++){
			if(updown[i]){
				//going up
				uint16_t sum = duty_cycle[i] + modes[current_mode].brightness_change[i];

				if(sum > modes[current_mode].max_brightness[i]){
					duty_cycle[i] = modes[current_mode].max_brightness[i] - (sum - modes[current_mode].max_brightness[i]);
					updown[i] = false;
				}
				else {
					duty_cycle[i] = sum;
					updown[i] = true;
				}
			}
			else {
				// Going down
				int16_t sum = duty_cycle[i] - modes[current_mode].brightness_change[i];

				if(sum < modes[current_mode].min_brightness[i]){
					duty_cycle[i] = modes[current_mode].min_brightness[i] + (modes[current_mode].min_brightness[i] - sum);
					updown[i] = true;
				}
				else {
					duty_cycle[i] = sum;
					updown[i] = false;
				}
			}
		}
	}
}

// Tim1 is the brightness updating timer

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

		update_brightness();

		// LEDs 3 & 4 are swapped
		TIM2->CH1CVR = duty_cycle[0];
		TIM2->CH2CVR = duty_cycle[1];
		TIM2->CH4CVR = duty_cycle[2];
		TIM2->CH3CVR = duty_cycle[3];
	
		TIM2->SWEVGR |= TIM_UG; // load new value in compare capture register

		TIM1->INTFR &= ~(u16)TIM_IT_Update;
	}
}

void EXTI7_0_IRQHandler( void ) __attribute__((interrupt));
void EXTI7_0_IRQHandler( void ) 
{

	
	//Switch to new mode
	current_mode = (current_mode+1)%(NUM_MODES);
	for(uint8_t i=0; i<4; i++){
		duty_cycle[i] = modes[current_mode].start_brightness[i];
		// Always start going up
		updown[i] = true;
		if(modes[current_mode].phase_offset[i] > 0){
			//Start the phase offset by running the brightness update function phase offset times
			if(modes[current_mode].sawtooth)
			// Simple mod adddition in sawtooth mode
				for(uint8_t i=0; i<modes[current_mode].phase_offset[i]; i++){
					duty_cycle[i] = (duty_cycle[i]+ modes[current_mode].brightness_change[i])%(modes[current_mode].max_brightness[i] - modes[current_mode].min_brightness[i]) + modes[current_mode].min_brightness[i];
					updown[i] = true;
				}
			else
			{
				for(uint8_t i=0; i<modes[current_mode].phase_offset[i]; i++){
					if(updown[i]){
						//going up
						uint16_t sum = duty_cycle[i] + modes[current_mode].brightness_change[i];

						if(sum > modes[current_mode].max_brightness[i]){
							duty_cycle[i] = modes[current_mode].max_brightness[i] - (sum - modes[current_mode].max_brightness[i]);
							updown[i] = false;
						}
						else {
							duty_cycle[i] = sum;
							updown[i] = true;
						}
					}
					else {
						// Going down
						int16_t sum = duty_cycle[i] - modes[current_mode].brightness_change[i];

						if(sum < modes[current_mode].min_brightness[i]){
							duty_cycle[i] = modes[current_mode].min_brightness[i] + (modes[current_mode].min_brightness[i] - sum);
							updown[i] = true;
						}
						else {
							duty_cycle[i] = sum;
							updown[i] = false;
						}
					}
				}
			}
		}
	}

	// Set new update timer
	TIM1->ATRLR = modes[current_mode].refresh;


	//update PWM freqs
	TIM2->CH1CVR = duty_cycle[0];
	TIM2->CH2CVR = duty_cycle[1];
	TIM2->CH4CVR = duty_cycle[2];
	TIM2->CH3CVR = duty_cycle[3];
	
	Delay_Us(500);
	// Acknowledge the interrupt
	EXTI->INTFR = EXTI_Line5;
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
		
	TIMER1_INIT(4800, 1000);
	NVIC_EnableIRQ(TIM1_UP_IRQn);

	Delay_Ms(100);

	GPIOD->CFGLR &= ~(0xf<<(4*5)); //clear values
	GPIOD->CFGLR |= (GPIO_CNF_IN_PUPD)<<(4*5); //set new ones
	//1 = pull-up, 0 = pull-down
	GPIOD->OUTDR |= 1<<5;

	AFIO->EXTICR = AFIO_EXTICR_EXTI5_PD;
	EXTI->INTENR = EXTI_INTENR_MR5; // Enable EXT5
	EXTI->FTENR = EXTI_FTENR_TR5;  // Falling edge trigger

	// enable interrupt
	NVIC_EnableIRQ( EXTI7_0_IRQn );


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
		}
}