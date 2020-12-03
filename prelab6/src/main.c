/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f0xx.h"
			

int main(void)
{
    setup_portc();
    copy_pa0_pc6();
	for(;;);
}

void setup_portc() {
  // Enable the RCC clock to GPIOC
  RCC -> AHBENR |= RCC_AHBENR_GPIOCEN;
  // Set PC0-PC10 as outputs
  GPIOC -> MODER &= 4290772992; // Clear out PC0-PC10
  GPIOC -> MODER |= 1398101; // Set least significant bit of PC0-PC10
  // Write the value of 0x03f to the Port C ODR
  GPIOC -> ODR = 63;
}

void copy_pa0_pc6() {
    // Enable the RCC clock to GPIOA
    RCC -> AHBENR |= RCC_AHBENR_GPIOAEN;
    // Set PA0 as an input
    GPIOA -> MODER &= ~GPIO_MODER_MODER0;
    // Set the PUPDR to pull down PA0
    GPIOA -> PUPDR &= ~GPIO_PUPDR_PUPDR0;
    GPIOA -> PUPDR |= GPIO_PUPDR_PUPDR0_1;
    // Enable the RCC clock to GPIOC
    RCC -> AHBENR |= RCC_AHBENR_GPIOCEN;
    // Set PC6 as an output
    GPIOA -> MODER &= ~GPIO_MODER_MODER6;
    GPIOA -> MODER |= GPIO_MODER_MODER6_1;
    // Continually copy the value read from PA0 to PC6
    for (;;) {
        GPIOC -> BSRR = ((1<<6)<<16) | (((GPIOA -> IDR) & 1) << 6);
    }
}
