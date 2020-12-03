//============================================================================
// ECE 362 lab practical 2
//============================================================================

#include "stm32f0xx.h"
#include <string.h> // for memset() declaration
#include <math.h>   // for M_PI

int counter = 0;
int increment = 0;
char buffer[20];
int button;

void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

void small_delay() {
    nano_wait(100000);
}

//============================================================================
// enable_ports()    (Autotest #3)
// Configure GPIO Ports B and C.
// Parameters: none
//============================================================================
void enable_ports()
{
    // Enable RCC clocks for GPIOB and GPIOC
    RCC -> AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC -> AHBENR |= RCC_AHBENR_GPIOCEN;
    // Configure PC0 - PC10 to be outputs
    GPIOC -> MODER &= ~0x3fffff;
    GPIOC -> MODER |= 0x155555;
    // Configure PB0 - PB3 to be outputs
    GPIOB -> MODER &= ~0xff;
    GPIOB -> MODER |= 0x55;
    // Configure PB4 - PB7 to be inputs
    GPIOB -> MODER &= ~0xff00;
    // Configure internal pull-down resistors for PB4-PB7
    GPIOB -> PUPDR &= ~0x5500;
    GPIOB -> PUPDR |= 0xaa00;
}

//============================================================================
// set_row()    (Autotest #5)
// Set the row active on the keypad matrix.
// Parameters: none
//============================================================================
void set_row(int row)
{
    row = row - 1;
    GPIOB -> BSRR = 0xf0000 | (1 << row);
}

//============================================================================
// get_cols()    (Autotest #6)
// Read the column pins of the keypad matrix.
// Parameters: none
// Return value: The 4-bit value read from PC[7:4].
//============================================================================
int get_col(int col)
{
    int value = ((GPIOB -> IDR) >> (3 + col)) & 1;
    return value;
}

void setup_spi2() {
    // Enable RCC clock to GPIO port B
    RCC -> AHBENR |= RCC_AHBENR_GPIOBEN;
    // Configure GPIOB ports for AF SPI2 operation
    GPIOB -> MODER &= ~(GPIO_MODER_MODER12 | GPIO_MODER_MODER13 | GPIO_MODER_MODER15);
    GPIOB -> MODER |= (GPIO_MODER_MODER12_1 | GPIO_MODER_MODER13_1 | GPIO_MODER_MODER15_1);
    GPIOB -> AFR[1] &= ~(GPIO_AFRH_AFR12 | GPIO_AFRH_AFR13 | GPIO_AFRH_AFR15);
    // PB12 is NSS, already set to AF0 as desired
    // PB13 is SCK, already set to AF0 as desired
    // PB15 is MOSI, already set to AF0 as desired
    // First enable the RCC clock to the SPI2 system
    RCC -> APB1ENR |= RCC_APB1ENR_SPI2EN;
    // Set the baud rate as low as possible
    SPI2 -> CR1 |= SPI_CR1_BR;
    // Configure the interface for a 10-bit word size,
    // and set the SS output enable bit and enable NSSP
    SPI2 -> CR2 = SPI_CR2_SSOE | SPI_CR2_NSSP | SPI_CR2_DS_3 | SPI_CR2_DS_0;
    // Configure the SPI channel to be in "master mode", but set BIDIMODE and BIDIOE as well
    SPI2 -> CR1 |= (SPI_CR1_MSTR | SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE);
    // Enable the SPI channel
    SPI2 -> CR1 |= SPI_CR1_SPE;
}

void spi_cmd(int cmd) {
    while (((SPI2 -> SR) & SPI_SR_TXE) != SPI_SR_TXE) {
        small_delay();
    }
    SPI2 -> DR = cmd;
}

void spi_data(int data) {
    while (((SPI2 -> SR) & SPI_SR_TXE) != SPI_SR_TXE) {
        small_delay();
    }
    SPI2 -> DR = (data | 0x200);
}

void spi_init_oled() {
    // Wait 1 ms for the display to power up and stabilize
    nano_wait(100000);
    // Set for 8-bit operation
    spi_cmd(0x38);
    // Turn display off
    spi_cmd(0x08);
    // Clear display
    spi_cmd(0x01);
    // Wait for display to clear
    nano_wait(2000000);
    // Set the display to scroll
    spi_cmd(0x06);
    // Move the cursor to the home position
    spi_cmd(0x02);
    // Turn the display on
    spi_cmd(0x0c);
}

void spi_display1(const char * string) {
    // Move the cursor to the home position
    spi_cmd(0x02);
    int i = 0;
    while (string[i] != NULL) {
        spi_data(string[i]);
        i++;
    }
}

void spi_display2(const char * string) {
    // Move the cursor to the lower row (offset 0x40)
    spi_cmd(0xc0);
    int i = 0;
    while (string[i] != NULL) {
        spi_data(string[i]);
        i++;
    }
}

//============================================================================
// Timer 7 ISR()    (Autotest #9)
// The Timer 7 ISR
// Parameters: none
// (Write the entire subroutine below.)
//============================================================================
void TIM7_IRQHandler(void) {
    // Acknowledge the timer interrupt by writing a zero to the UIF bit
    TIM7 -> SR &= ~TIM_SR_UIF;
    // Check if the keypad button 'D' is pressed
    button = get_col(4);
    if (button > 0) { // if it is, enable the counter to be incremented
        increment = 1;
        counter = 0;
    } else {
        counter += increment;
    }
    // Check if the keypad button '#' is pressed
    button = get_col(3);
    if (button > 0) { // if it is, turn off the variable that enables the counter to be incremented
        increment = 0;
    }
    // Display the counter every time it is incremented by sending the number through
    // SPI2 to the OLED LCD.  Use setup code from Lab 8 to initialize SPI2 channel and
    // configure the display
    spi_cmd(0x02);
    sprintf(buffer, "%08d", counter);
    spi_display1(buffer);
}

//============================================================================
// setup_tim7()    (Autotest #10)
// Configure timer 7.
// Parameters: none
//============================================================================
void setup_tim7()
{
    // Enable the RCC clock for TIM7
    RCC -> APB1ENR |= RCC_APB1ENR_TIM7EN;
    // Disable the RCC clock for TIM6
    RCC -> APB1ENR &= ~RCC_APB1ENR_TIM6EN;
    // Set the prescaler and auto-reload register to result in a timer update event
    // exactly 10 times per second
    TIM7 -> PSC = 48000 - 1;
    TIM7 -> ARR = 100 - 1;
    // Enable the UIE bit in the DIER
    TIM7 -> DIER |= TIM_DIER_UIE;
    // Enable the timer
    TIM7 -> CR1 |= TIM_CR1_CEN;
    // Enable the Timer 7 interrupt in the NVIC ISER
    NVIC -> ISER[0] = (1 << TIM7_IRQn);
}

int main(void)
{
    enable_ports();
    setup_spi2();
    spi_init_oled();
    set_row(4);
    setup_tim7();
    for(;;) {
        asm volatile ("wfi");
    }
}
