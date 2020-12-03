
//============================================================================
// ECE 362 lab experiment 10 -- Asynchronous Serial Communication
//============================================================================

#include "stm32f0xx.h"
#include "ff.h"
#include "diskio.h"
#include "fifo.h"
#include "tty.h"
#include <string.h> // for memset()
#include <stdio.h> // for printf()

void advance_fattime(void);
void command_shell(void);

// Write your subroutines below.
void setup_usart5() {
    // Enable the RCC clocks to GPIOC and GPIOD
    RCC -> AHBENR |= RCC_AHBENR_GPIOCEN;
    RCC -> AHBENR |= RCC_AHBENR_GPIODEN;
    // Do all the steps necessary to configure PC12 to be routed to USART5_TX
    GPIOC -> MODER &= ~(GPIO_MODER_MODER12);
    GPIOC -> MODER |= GPIO_MODER_MODER12_1;
    GPIOC -> AFR[1] &= ~(GPIO_AFRH_AFR12);
    GPIOC -> AFR[1] |= (2 << 16); // Select AF2
    // Do all the steps necessary to configure PD2 to be routed to USART5_RX
    GPIOD -> MODER &= ~(GPIO_MODER_MODER2);
    GPIOD -> MODER |= GPIO_MODER_MODER2_1;
    GPIOD -> AFR[0] &= ~(GPIO_AFRL_AFR2);
    GPIOD -> AFR[0] |= (2 << 8); // Select AF2
    // Enable the RCC clock to the USART5 peripheral
    RCC -> APB1ENR |= RCC_APB1ENR_USART5EN;
    // Configure USART5 as follows:
    // First, disable it by turning off its UE bit
    USART5 -> CR1 &= ~USART_CR1_UE;
    // Set a word size of 8 bits
    USART5 -> CR1 &= ~USART_CR1_M;
    // Set it for one stop bit
    USART5 -> CR2 &= ~USART_CR2_STOP;
    // Set it for no parity
    USART5 -> CR1 &= ~USART_CR1_PCE;
    // Use 16x oversampling
    USART5 -> CR1 &= ~USART_CR1_OVER8;
    // Use a baud rate of 115200 (115.2 kbaud)
    USART5 -> BRR = 0x1A1;
    // Enable the transmitter and the receiver by setting the TE and RE bits
    USART5 -> CR1 |= (USART_CR1_TE | USART_CR1_RE);
    // Enable the USART
    USART5 -> CR1 |= USART_CR1_UE;
    // Finally, you should wait for the TE and RE bits to be acknowledged.  This indicates
    // that the USART is ready to transmit and receive
    while ((USART5 -> ISR & USART_ISR_TEACK) == 0);
    while ((USART5 -> ISR & USART_ISR_REACK) == 0);
}

int simple_putchar(int character) {
    // Wait for the USART5 ISR TXE to be set
    while ((USART5 -> ISR & USART_ISR_TXE) == 0);
    // Write the argument to the USART5 TDR (transmit data register)
    USART5 -> TDR = character;
    // Return the argument that was passed in (that's how putchar() is defined to work)
    return character;
}

int simple_getchar() {
    // Wait for the USART5 ISR RXNE bit to be set
    while ((USART5 -> ISR & USART_ISR_RXNE) == 0);
    // Return the value of the USART5 RDR (receive data register)
    return (USART5 -> RDR);
}

int __io_putchar(int ch) {
    return better_putchar(ch);
}

int __io_getchar(void) {
    return interrupt_getchar();
}

int better_putchar(int character) {
    // Newline handling
    if (character == 10) { // if character is \n
        simple_putchar(13); // write \r
        return simple_putchar(character); // write \n
    } else {
        return simple_putchar(character);
    }
}

int better_getchar() {
    // Wait for the USART5 ISR RXNE bit to be set
    while ((USART5 -> ISR & USART_ISR_RXNE) == 0);
    // Return the value of the USART5 RDR (receive data register)
    int character = (USART5 -> RDR);
    if (character == 13) { // if character is \r
        return 10; // return \n instead
    }
    return character;
}

int interrupt_getchar() {
    while(fifo_newline(&input_fifo) == 0) {
        asm volatile ("wfi"); // wait for interrupt
    }
    // Return a character from the line buffer.
    char ch = fifo_remove(&input_fifo);
    return ch;
}

void USART3_4_5_6_7_8_IRQHandler(void) {
    // Check and clear the ORE flag
    if ((USART5 -> ISR) & USART_ISR_ORE)
        (USART5 -> ICR) |= USART_ICR_ORECF;
    // Read the new character from the USART5 RDR
    int character = (USART5 -> RDR);
    // Check if the input_fifo is full.  If it is, return from the ISR (throw away the character)
    if (fifo_full(&input_fifo)) {
        return;
    }
    // Call insert_echo_char() with the character
    insert_echo_char(character);
}

void enable_tty_interrupt() {
    // Enable RXNEIE bit in CR1
    USART5 -> CR1 |= USART_CR1_RXNEIE;
    // Enable interrupt for USART3_4_5_6_7_8 in NVIC ISER (bit 29)
    NVIC -> ISER[0] |= (1 << 29);
}

void setup_spi1() {
    // Enable the RCC clock to GPIOA
    RCC -> AHBENR |= RCC_AHBENR_GPIOAEN;
    // Configure PA1 to be a general-purpose output
    GPIOA -> MODER &= ~(GPIO_MODER_MODER1);
    GPIOA -> MODER |= GPIO_MODER_MODER1_0;
    // Configure GPIOA so that pins 5, 6, and 7 are routed to SPI1
    GPIOA -> MODER &= ~(GPIO_MODER_MODER5 | GPIO_MODER_MODER6 | GPIO_MODER_MODER7);
    GPIOA -> MODER |= (GPIO_MODER_MODER5_1 | GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1);
    GPIOA -> AFR[0] &= ~(GPIO_AFRL_AFR5 | GPIO_AFRL_AFR6 | GPIO_AFRL_AFR7);
    // PA5 is SCK (already set to AF0 as desired)
    // PA6 is MISO (already set to AF0 as desired)
    // PA7 is MOSI (already set to AF0 as desired)
    // Enable the internal pull-up resistor for pin 6 (MISO)
    GPIOA -> PUPDR &= ~(GPIO_PUPDR_PUPDR6);
    GPIOA -> PUPDR |= GPIO_PUPDR_PUPDR6_0;
    // Enable the RCC clock to the SPI1 peripheral
    RCC -> APB2ENR |= RCC_APB2ENR_SPI1EN;
    // Disable the SPI1 peripheral by turning off the SPE bit
    SPI1 -> CR1 &= ~(SPI_CR1_SPE);
    // Set it for as low a baud rate as possible
    SPI1 -> CR1 |= SPI_CR1_BR;
    // Ensure that BIDIMODE and BIDIOE are cleared
    SPI1 -> CR1 &= ~(SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE);
    // Enable master mode
    SPI1 -> CR1 |= SPI_CR1_MSTR;
    // Set NSSP and configure the peripheral for an 8-bit word (which is the default)
    // and set the bit that sets the FIFO-reception threshold to 8-bits
    SPI1 -> CR2 = (SPI_CR2_NSSP | SPI_CR2_FRXTH);
    // Enable the SPI1 peripheral
    SPI1 -> CR1 |= SPI_CR1_SPE;
}

void spi_high_speed() {
    // Disable the SPI1 SPE bit
    SPI1 -> CR1 &= ~(SPI_CR1_SPE);
    // Configure SPI1 for a 6 MHz SCK rate
    SPI1 -> CR1 &= ~(SPI_CR1_BR);
    SPI1 -> CR1 |= (SPI_CR1_BR_2 | SPI_CR1_BR_0);
    // Re-enable the SPI1 SPE bit
    SPI1 -> CR1 |= SPI_CR1_SPE;
}

void TIM14_IRQHandler(void) {
    // Acknowledge the interrupt
    TIM14 -> SR &= ~(TIM_SR_UIF);
    advance_fattime();
}

// Configure Timer 14 to raise an interrupt every two seconds (0.5 Hz)
void setup_tim14() {
    // Enable the RCC clock to Timer 14
    RCC -> APB1ENR |= RCC_APB1ENR_TIM14EN;
    // Configure TIM14_PSC to prescale the system clock by 48,000
    TIM14 -> PSC = (48000 - 1);
    // Configure the TIM14_ARR to have a counting period of 2000
    TIM14 -> ARR = (2000 - 1);
    // Configure TIM14_DIER to enable the UIE flag
    TIM14 -> DIER |= TIM_DIER_UIE;
    // Set TIM_CR1_CEN in TIM14_CR1
    TIM14 -> CR1 |= TIM_CR1_CEN;
    // Enable the interrupt for TIM14 in the NVIC ISER (bit 19)
    NVIC -> ISER[0] |= (1 << 19);
}

// Write your subroutines above.

const char testline[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\r\n";

int main()
{
    setup_usart5();

    // Uncomment these when you're asked to...
    setbuf(stdin, 0);
    setbuf(stdout, 0);
    setbuf(stderr, 0);

    // Test 2.2 simple_putchar()

//    for(;;)
//        for(const char *t=testline; *t; t++)
//            simple_putchar(*t);

    // Test for 2.3 simple_getchar()

//    for(;;)
//        simple_putchar( simple_getchar() );

    // Test for 2.4 and 2.5 __io_putchar() and __io_getchar()

//    printf("Hello!\n");
//    for(;;)
//        putchar( getchar() );

    // Test for 2.6

//    for(;;) {
//        printf("Enter string: ");
//        char line[100];
//        fgets(line, 99, stdin);
//        line[99] = '\0'; // just in case
//        printf("You entered: %s", line);
//    }

    // Test for 2.7
    //
//    enable_tty_interrupt();
//    for(;;) {
//        printf("Enter string: ");
//        char line[100];
//        fgets(line, 99, stdin);
//        line[99] = '\0'; // just in case
//        printf("You entered: %s", line);
//    }

    // Test for 2.8 Test the command shell and clock.
    //
    enable_tty_interrupt();
    setup_tim14();
    FATFS fs_storage;
    FATFS *fs = &fs_storage;
    f_mount(fs, "", 1);
    command_shell();

    return 0;
}
