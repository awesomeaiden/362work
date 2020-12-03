
//============================================================================
// ECE 362 lab experiment 8 -- SPI and DMA
//============================================================================

#include "stm32f0xx.h"
#include "lcd.h"
#include <stdio.h> // for sprintf()

// Be sure to change this to your login...
const char login[] = "gonza487";

// Prototypes for misc things in lcd.c
void nano_wait(unsigned int);

// Write your subroutines below.

// Setup bit-banging with GPIO port B
// PB12 is NSS, PB13 is SCK, and PB15 is MOSI
void setup_bb() {
    // Enable RCC clock to GPIO port B
    RCC -> AHBENR |= RCC_AHBENR_GPIOBEN;
    // Configure PB12, PB13, and PB15 for output
    GPIOB -> MODER &= ~(GPIO_MODER_MODER12 | GPIO_MODER_MODER13 | GPIO_MODER_MODER15);
    GPIOB -> MODER |= (GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 | GPIO_MODER_MODER15_0);
    // Initialize ODR so that NSS is high and SCK is low
    GPIOB -> ODR |= GPIO_ODR_12;
    GPIOB -> ODR &= ~(GPIO_ODR_13);
    // Doesn't matter what MOSI is initialized to - don't worry about it
}

void small_delay() {
    nano_wait(1000000);
}

void bb_write_bit(int bit) {
    // Set MOSI to value of bit
    GPIOB -> ODR &= ~(1 << 15);
    GPIOB -> ODR |= (bit << 15);
    small_delay();
    // Set the SCK pin to high
    GPIOB -> ODR |= GPIO_ODR_13;
    small_delay();
    // Set the SCK pin to low
    GPIOB -> ODR &= ~(GPIO_ODR_13);
}

void bb_write_byte(int byte) {
    bb_write_bit((byte >> 7) & 1);
    bb_write_bit((byte >> 6) & 1);
    bb_write_bit((byte >> 5) & 1);
    bb_write_bit((byte >> 4) & 1);
    bb_write_bit((byte >> 3) & 1);
    bb_write_bit((byte >> 2) & 1);
    bb_write_bit((byte >> 1) & 1);
    bb_write_bit((byte) & 1);
}

void bb_cmd(int cmd) {
    // Set the NSS pin low to start an SPI transfer
    GPIOB -> ODR &= ~(GPIO_ODR_12);
    small_delay();
    // RS is 0 to start a command byte
    bb_write_bit(0);
    // R/W is 0 for a write
    bb_write_bit(0);
    // Call bb_write_byte() with parameter
    bb_write_byte(cmd);
    small_delay();
    // Set the NSS pin high to signal the end of the SPI transfer
    GPIOB -> ODR |= GPIO_ODR_12;
    small_delay();
}

void bb_data(int data) {
    // Set the NSS pin low to start an SPI transfer
    GPIOB -> ODR &= ~(GPIO_ODR_12);
    small_delay();
    // RS is 1 to start a data byte
    bb_write_bit(1);
    // R/W is 0 for a write
    bb_write_bit(0);
    // Call bb_write_byte() with parameter
    bb_write_byte(data);
    small_delay();
    // Set the NSS pin high to signal the end of the SPI transfer
    GPIOB -> ODR |= GPIO_ODR_12;
    small_delay();
}

void bb_init_oled() {
    // Wait 1 ms for the display to power up and stabilize
    nano_wait(1000000);
    // Set for 8-bit operation
    bb_cmd(0x38);
    // Turn display off
    bb_cmd(0x08);
    // Clear display
    bb_cmd(0x01);
    // Wait for display to clear
    nano_wait(2000000);
    // Set the display to scroll
    bb_cmd(0x06);
    // Move the cursor to the home position
    bb_cmd(0x02);
    // Turn the display on
    bb_cmd(0x0c);
}

void bb_display1(const char * string) {
    // Move the cursor to the home position
    bb_cmd(0x02);
    int i = 0;
    while (string[i] != NULL) {
        bb_data(string[i]);
        i++;
    }
}

void bb_display2(const char * string) {
    // Move the cursor to the lower row (offset 0x40)
    bb_cmd(0xc0);
    int i = 0;
    while (string[i] != NULL) {
        bb_data(string[i]);
        i++;
    }
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
    nano_wait(1000000);
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

void spi_enable_dma(const short * addr) {
    // Enable the RCC clock to the DMA controller
    RCC -> AHBENR |= RCC_AHBENR_DMA1EN;
    // Set CPAR to the address of the SPI2_DR register
    DMA1_Channel5 -> CPAR = (uint32_t) (&(SPI2 -> DR));
    // Set CMAR to the parameter passed in
    DMA1_Channel5 -> CMAR = addr;
    // Set CNDTR to 34
    DMA1_Channel5 -> CNDTR = 34;
    // Set the DIRection for copying to memory-to-peripheral
    DMA1_Channel5 -> CCR |= DMA_CCR_DIR;
    // Set the MINC to increment the CMAR for every transfer
    DMA1_Channel5 -> CCR |= DMA_CCR_MINC;
    // Set the memory datum size to 16-bit
    DMA1_Channel5 -> CCR &= ~(DMA_CCR_MSIZE);
    DMA1_Channel5 -> CCR |= DMA_CCR_MSIZE_0;
    // Set the peripheral datum size to 16-bit
    DMA1_Channel5 -> CCR &= ~(DMA_CCR_PSIZE);
    DMA1_Channel5 -> CCR |= DMA_CCR_PSIZE_0;
    // Set the channel for CIRCular operation
    DMA1_Channel5 -> CCR |= DMA_CCR_CIRC;
    // Enable the channel
    DMA1_Channel5 -> CCR |= DMA_CCR_EN;
    // Turn on the configuration bit in SPI2_CR2 that enables a DMA trigger when TX is empty
    SPI2 -> CR2 |= SPI_CR2_TXDMAEN;
}

void setup_spi1() {
    // Enable the RCC clock to GPIO Port A
    RCC -> AHBENR |= RCC_AHBENR_GPIOAEN;
    // Configure pins PA4, PA5, and PA7 for alternate function 0
    GPIOA -> MODER &= ~(GPIO_MODER_MODER4 | GPIO_MODER_MODER5 | GPIO_MODER_MODER7);
    GPIOA -> MODER |= (GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1 | GPIO_MODER_MODER7_1);
    GPIOA -> AFR[0] &= ~(GPIO_AFRL_AFR4 | GPIO_AFRL_AFR5 | GPIO_AFRL_AFR7);
    // Configure pins PA2 and PA3 to be outputs
    GPIOA -> MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    GPIOA -> MODER |= (GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0);
    // Enable the RCC clock to SPI1
    RCC -> APB2ENR |= RCC_APB2ENR_SPI1EN;
    // Configure SPI1 for Master/Bidimode/BidiOE, but set the baud rate as high as possible
    SPI1 -> CR1 |= (SPI_CR1_MSTR | SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE);
    SPI1 -> CR1 &= ~(SPI_CR1_BR);
    // Set SSOE/NSSP as you did for SPI2, but leave the word size set to 8-bit (the default)
    SPI1 -> CR2 = SPI_CR2_SSOE | SPI_CR2_NSSP;
    // Enable the SPI channel
    SPI1 -> CR1 |= SPI_CR1_SPE;
}

// Write your subroutines above.

void show_counter(short buffer[])
{
    for(int i=0; i<10000; i++) {
        char line[17];
        sprintf(line,"% 16d", i);
        for(int b=0; b<16; b++)
            buffer[1+b] = line[b] | 0x200;
    }
}

void internal_clock();
void demo();
void autotest();

extern const Picture *image;

int main(void)
{
    //internal_clock();
    //demo();
    //autotest();

    setup_bb();
    bb_init_oled();
    bb_display1("Hello,");
    bb_display2(login);

    setup_spi2();
    spi_init_oled();
    spi_display1("Hello again,");
    spi_display2(login);

    short buffer[34] = {
            0x02, // This word sets the cursor to the beginning of line 1.
            // Line 1 consists of spaces (0x20)
            0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
            0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
            0xc0, // This word sets the cursor to the beginning of line 2.
            // Line 2 consists of spaces (0x20)
            0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
            0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220, 0x220,
    };

    spi_enable_dma(buffer);
    show_counter(buffer);

    setup_spi1();
    LCD_Init();
    LCD_Clear(BLACK);
    LCD_DrawLine(10,20,100,200, WHITE);
    LCD_DrawRectangle(10,20,100,200, GREEN);
    LCD_DrawFillRectangle(120,20,220,200, RED);
    LCD_Circle(50, 260, 50, 1, BLUE);
    LCD_DrawFillTriangle(130,130, 130,200, 190,160, YELLOW);
    LCD_DrawChar(150,155, BLACK, WHITE, 'X', 16, 1);
    LCD_DrawString(140,60,  WHITE, BLACK, "ECE 362", 16, 0);
    LCD_DrawString(140,80,  WHITE, BLACK, "has the", 16, 1);
    LCD_DrawString(130,100, BLACK, GREEN, "best toys", 16, 0);
    LCD_DrawPicture(110,220,(const Picture *)&image);
}
