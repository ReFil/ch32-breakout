#include "ch32fun.h"
#include "ch32v003_GPIO_branchless.h"

#include <stdio.h>
#include <stdbool.h>

// use defines to make more meaningful names for our GPIO pins
#define BUTTON GPIOv_from_PORT_PIN(GPIO_port_D, 2)


#define PSU_EN GPIOv_from_PORT_PIN(GPIO_port_C, 3)
#define EN_LED GPIOv_from_PORT_PIN(GPIO_port_C, 7)
#define FAULT_LED GPIOv_from_PORT_PIN(GPIO_port_C, 6)

#define PS_ALARM GPIOv_from_PORT_PIN(GPIO_port_C, 4)



bool btn;
bool btn_prv;

bool supply_on = false;

void set_supply(bool state) {
	// Toggle the state of the enable pin on the power supply
	supply_on = state;
	if(supply_on) {
		GPIO_digitalWrite(PSU_EN, high);
		GPIO_digitalWrite(EN_LED, high);
	} else {
		GPIO_digitalWrite(PSU_EN, low);
		GPIO_digitalWrite(EN_LED, low);
	}
	
}

int main()
{
	SystemInit();

	GPIO_port_enable(GPIO_port_D);
	GPIO_port_enable(GPIO_port_C);
	GPIO_ADCinit();



	GPIO_pinMode(PSU_EN, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
	GPIO_pinMode(EN_LED, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
	GPIO_pinMode(FAULT_LED, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);


	GPIO_pinMode(BUTTON, GPIO_pinMode_I_pullUp, GPIO_Speed_In); 
	GPIO_pinMode(PS_ALARM, GPIO_pinMode_I_pullUp, GPIO_Speed_In); 
	

	while(1)
	{
		// read button and look for risig edge
		btn = !GPIO_digitalRead(BUTTON);
		if(btn && !btn_prv){
			set_supply(!supply_on);
		}

		// Read PSOK pin and light FAULT_LED if it goes low and supply is on
		if((GPIO_analogRead(GPIO_Ain4_D3) < 500) && supply_on) {
			GPIO_digitalWrite(FAULT_LED, high);
		} else {
			GPIO_digitalWrite(FAULT_LED, low);
		} 
		printf("Current value: %u\n", GPIO_analogRead(GPIO_Ain7_D4));
		Delay_Ms(50);
		btn_prv = btn;



	}
}