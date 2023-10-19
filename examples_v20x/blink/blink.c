#include "ch32v003fun.h"
#include <stdio.h>

int main()
{
	SystemInit();

	// Enable GPIOs
	//RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC;
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOA;

	// GPIO A6 Push-Pull
	GPIOA->CFGLR &= ~((uint32_t)0x0F<<(4*6));
	GPIOA->CFGLR |= (uint32_t)(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*6);

	while(1)
	{
		GPIOA->BSHR = (1<<6);	 // Turn on GPIO
		Delay_Ms( 1000 );
		GPIOA->BSHR = (1<<(16+6)); // Turn off GPIO
		Delay_Ms( 1000 );
	}
}
