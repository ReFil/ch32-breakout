#include "ch32fun.h"
#include "ch32v003_GPIO_branchless.h"

#include <stdio.h>
#include <stdbool.h>

// use defines to make more meaningful names for our GPIO pins
#define CH_1 GPIOv_from_PORT_PIN(GPIO_port_D, 2)

#define CH_2 GPIOv_from_PORT_PIN(GPIO_port_D, 4)
#define CH_3 GPIOv_from_PORT_PIN(GPIO_port_D, 6)
#define CH_4 GPIOv_from_PORT_PIN(GPIO_port_D, 0)


#define BUTTON GPIOv_from_PORT_PIN(GPIO_port_D, 5)


bool btn;
bool btn_prv;

uint8_t brightness = 0;

uint16_t period1 = 250;
uint16_t period2 = 300;
uint16_t period3 = 350;
uint16_t period4 = 400;

uint16_t duty_cycle1 = 0;
uint16_t duty_cycle2 = 0;
uint16_t duty_cycle3 = 0;
uint16_t duty_cycle4 = 0;

uint8_t count = 1;
bool updown = false;

uint8_t brightness_mod(uint8_t bright) {
	if((count == 100) || (count == 0))
		updown = !updown;
	if(updown)
		count--;
	else
		count++;
	return (brightness + (brightness/5)*(count/8));
}

void softPWM_feed() {

	
}

int main()
{
	SystemInit();

	GPIO_port_enable(GPIO_port_D);
	
	GPIO_pinMode(CH_1, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz); // Brown
	GPIO_pinMode(CH_2, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
	GPIO_pinMode(CH_3, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
	GPIO_pinMode(CH_4, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);


	GPIO_pinMode(BUTTON, GPIO_pinMode_I_pullUp, GPIO_Speed_In); // Yellow


	while(1)
	{
		btn = !GPIO_digitalRead(BUTTON);
		if(btn && !btn_prv)
			brightness += 10;

		btn_prv = btn;


		GPIO_digitalWrite( CH_1,     high ); // Turn on PIN_1
		GPIO_digitalWrite( CH_2,     high );
		GPIO_digitalWrite( CH_3,     high );
		GPIO_digitalWrite( CH_4,     high );
		Delay_Us( 5 + brightness_mod(brightness) );
		GPIO_digitalWrite( CH_1,     low );  // Turn off PIN_1
		GPIO_digitalWrite( CH_2,     low );
		GPIO_digitalWrite( CH_3,     low );
		GPIO_digitalWrite( CH_4,     low );
		Delay_Us( 12000);

	}
}