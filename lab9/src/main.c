
//============================================================================
// ECE 362 lab experiment 9 -- I2C
//============================================================================

#include "stm32f0xx.h"
#include <stdint.h> // for uint8_t
#include <string.h> // for strlen() and strcmp()

// Be sure to change this to your login...
const char login[] = "gonza487";

//============================================================================
// Wait for n nanoseconds. (Maximum: 4.294 seconds)
//============================================================================
void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

// Write your subroutines below...

// Configure PB8 and PB9 to be SCL and SDA respectively (I2C1)
void setup_i2c() {
    // Enable GPIO Port B and the I2C1 channel
    RCC -> AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC -> APB1ENR |= RCC_APB1ENR_I2C1EN;
    // Set the MODER fields for PB8 and PB9
    GPIOB -> MODER &= ~(GPIO_MODER_MODER8 | GPIO_MODER_MODER9);
    GPIOB -> MODER |= (GPIO_MODER_MODER8_1 | GPIO_MODER_MODER9_1);
    // Set alternate function register entries
    GPIOB -> AFR[1] &= ~(GPIO_AFRH_AFR8 | GPIO_AFRH_AFR9);
    GPIOB -> AFR[1] |= (1 | (1 << 4)); // Select AF1
    // Configure the I2C1 channel:
    // Disable the PE bit in CR1 first
    I2C1 -> CR1 &= ~(I2C_CR1_PE);
    // Turn off the ANFOFF bit (turn on the analog noise filter)
    I2C1 -> CR1 &= ~(I2C_CR1_ANFOFF);
    // Disable the error interrupt
    I2C1 -> CR1 &= ~(I2C_CR1_ERRIE);
    // Turn off the NOSTRETCH bit in CR1 to enable clock stretching
    I2C1 -> CR1 &= ~(I2C_CR1_NOSTRETCH);
    // Set the TIMINGR register as follows:
    // Set the prescaler to 0
    I2C1 -> TIMINGR &= ~(I2C_TIMINGR_PRESC);
    // Set the SCLDEL field to 3
    I2C1 -> TIMINGR &= ~(I2C_TIMINGR_SCLDEL);
    I2C1 -> TIMINGR |= (3 << 20);
    // Set the SDADEL field to 1
    I2C1 -> TIMINGR &= ~(I2C_TIMINGR_SDADEL);
    I2C1 -> TIMINGR |= (1 << 16);
    // Set the SCLH field to 3
    I2C1 -> TIMINGR &= ~(I2C_TIMINGR_SCLH);
    I2C1 -> TIMINGR |= (3 << 8);
    // Set the SCLL field to 9
    I2C1 -> TIMINGR &= ~(I2C_TIMINGR_SCLL);
    I2C1 -> TIMINGR |= 9;
    // Disable both of the "own addresses", OAR1 and OAR2
    I2C1 -> OAR1 &= ~(I2C_OAR1_OA1EN);
    I2C1 -> OAR2 &= ~(I2C_OAR2_OA2EN);
    // Configure the ADD10 field of CR2 for 7-bit mode
    I2C1 -> CR2 &= ~(I2C_CR2_ADD10);
    // Turn on the AUTOEND setting to enable automatic end
    I2C1 -> CR2 |= I2C_CR2_AUTOEND;
    // Enable the channel by setting the PE bit in CR1
    I2C1 -> CR1 |= I2C_CR1_PE;
}

void i2c_waitidle(void) {
    while ( (I2C1->ISR & I2C_ISR_BUSY) == I2C_ISR_BUSY); // while busy, wait.
}

int i2c_checknack() {
    int nack = ((I2C1 -> ISR) >> 4) & 1;
    return nack;
}

void i2c_clearnack() {
    I2C1 -> ICR |= I2C_ICR_NACKCF;
}

void i2c_start(uint32_t devaddr, uint8_t size, uint8_t dir) {
    // dir: 0 = master requests a write transfer
    // dir: 1 = master requests a read transfer
    uint32_t tmpreg = I2C1->CR2;
    tmpreg &= ~(I2C_CR2_SADD | I2C_CR2_NBYTES |
            I2C_CR2_RELOAD | I2C_CR2_AUTOEND |
            I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_STOP);
    if (dir == 1)
        tmpreg |= I2C_CR2_RD_WRN; // Read from slave
    else
        tmpreg &= ~I2C_CR2_RD_WRN; // Write to slave
    tmpreg |= ((devaddr<<1) & I2C_CR2_SADD) | ((size << 16) & I2C_CR2_NBYTES);
    tmpreg |= I2C_CR2_START;
    I2C1->CR2 = tmpreg;
}


void i2c_stop(void) {
    if (I2C1->ISR & I2C_ISR_STOPF)
        return;
    // Master: Generate STOP bit after current byte has been transferred.
    I2C1->CR2 |= I2C_CR2_STOP;
    // Wait until STOPF flag is reset
    while( (I2C1->ISR & I2C_ISR_STOPF) == 0);
    I2C1->ICR |= I2C_ICR_STOPCF; // Write to clear STOPF flag
}

int8_t i2c_senddata(uint8_t devaddr, void *pdata, uint8_t size) {
    int i;
    if (size <= 0 || pdata == 0) {
        return -1;
    }
    uint8_t *udata = (uint8_t*)pdata;
    i2c_waitidle();
    // Last argument is dir: 0 = sending data to the slave device
    i2c_start(devaddr, size, 0);

    for (i = 0; i < size; i++) {
        // TXIS bit is set by hardware when the TXDR register is empty and the
        // data to be transmitted must be written in the TXDR register.  It is
        // cleared when the next data to be sent is written in the TXDR register.
        // The TXIS flag is not set when a NACK is received.
        int count = 0;
        while ((I2C1 -> ISR & I2C_ISR_TXIS) == 0) {
            count += 1;
            if (count > 1000000) {
                return -1;
            }
            if (i2c_checknack()) {
                i2c_clearnack();
                i2c_stop();
                return -1;
            }
        }
        // TXIS is cleared by writing to the TXDR register
        I2C1 -> TXDR = udata[i] & I2C_TXDR_TXDATA;
    }
    // Wait until TC flag is set or the NACK flag is set
    while ((I2C1 -> ISR & I2C_ISR_TC) == 0 && (I2C1 -> ISR & I2C_ISR_NACKF) == 0);

    if ((I2C1 -> ISR & I2C_ISR_NACKF) != 0) {
        return -1;
    }
    i2c_stop();
    return 0;
}

int8_t i2c_recvdata(uint8_t devaddr, void *pdata, uint8_t size) {
    int i;
    if (size <= 0 || pdata == 0) {
        return -1;
    }
    uint8_t *udata = (uint8_t*)pdata;
    i2c_waitidle();
    // Last argument is dir: 1 = receiving data from the slave device
    i2c_start(devaddr, size, 1);

    for (i = 0; i < size; i++) {
        // wait until RXNE flag is set
        while ((I2C1 -> ISR & I2C_ISR_RXNE) == 0);
        udata[i] = I2C1 -> RXDR & I2C_RXDR_RXDATA;
    }

    // wait until TCR flag is set
    while ((I2C1 -> ISR & I2C_ISR_TC) == 0);

    i2c_stop();
    return 0;
}

void i2c_set_iodir(int input) {
    uint8_t data[2];
    // Address of IODIR register
    data[0] = 0;
    data[1] = input;
    // 0x27 is address of MCP23008 in current configuration
    i2c_senddata(0x27, data, sizeof data);
}

void i2c_set_gpio(int n) {
    char data[2];
    // Address of GPIO register
    data[0] = 9;
    data[1] = (char)n;
    // 0x27 is address of MCP23008 in current configuration
    i2c_senddata(0x27, data, 2);
}

int i2c_get_gpio() {
    char buffer[1];
    // Address of GPIO register
    buffer[0] = 9;
    // 0x27 is address of MCP23008 in current configuration
    // empty write to set cursor
    i2c_senddata(0x27, buffer, 1);
    // now read from gpio buffer
    i2c_recvdata(0x27, buffer, 1);
    return buffer[0];
}

void i2c_write_flash(uint16_t loc, const char * data, uint8_t len) {
    char buffer[34];
    // Set first two bytes to storage location
    buffer[0] = (loc >> 8) & 0xFF;
    buffer[1] = loc & 0xFF;
    int i;
    for (i = 0; i < len; i++) {
        buffer[i + 2] = data[i];
    }
    // 0x57 is address of 24AA32AF in current configuration
    i2c_senddata(0x57, buffer, len + 2);
}

int i2c_write_flash_complete() {
    // Wait for the I2C channel to be idle
    i2c_waitidle();
    // Initiate an i2c_start() with the correct I2C EEPROM device ID, zero length,
    // and write-intent
    i2c_start(0x57, 0, 0);
    // Wait until TC flag is set or the NACK flag is set
    while ((I2C1 -> ISR & I2C_ISR_TC) == 0 && (I2C1 -> ISR & I2C_ISR_NACKF) == 0);
    // If the NACKF flag is set, clear it, invoke i2c_stop(), and return 0
    if (((I2C1 -> ISR) & I2C_ISR_NACKF) > 0) {
        i2c_clearnack();
        i2c_stop();
        return 0;
    } else { // If the NACKF flag is not set, invoke i2c_stop() and return 1
        i2c_stop();
        return 1;
    }
}

void i2c_read_flash(uint16_t loc, char data[], uint8_t len) {
    char address[2];
    // Set first two bytes to storage location
    address[0] = (loc >> 8) & 0xFF;
    address[1] = loc & 0xFF;
    // 0x57 is address of MCP23008 in current configuration
    // empty write to set cursor
    i2c_senddata(0x57, address, 2);
    // now read from MCP23008
    i2c_recvdata(0x57, data, len);
}

void i2c_test() {
    char buffer[2];
    while (1 == 1) {
        i2c_start(0x27, 2, 0);
        i2c_stop();
        //i2c_senddata(0x27, buffer, 2);
        i2c_waitidle();
    }

}


void internal_clock();
void demo();
void autotest();

int main(void)
{
    //internal_clock();
    //demo();
    //autotest();

    setup_i2c();
    //i2c_test();

    i2c_set_iodir(0xf0); //  upper 4 bits input / lower 4 bits output

    // Show the happy LEDs for 4 seconds.
    for(int i=0; i<10; i++) {
        for(int n=1; n <= 8; n <<= 1) {
            i2c_set_gpio(n);
            int value = i2c_get_gpio();
            if ((value & 0xf) != n)
                break;
            nano_wait(100000000); // 0.1 s
        }
    }

    const char string[] = "This is a test.";
    int len = strlen(string) + 1;
    i2c_write_flash(0x200, string, len);

    int count = 0;
    while(1) {
        if (i2c_write_flash_complete())
            break;
        count++;
    }

    if (count == 0) {
        // It could not have completed immediately.
        // i2c_write_flash_complete() does not work.  Show slow angry LEDs.
        int all = 0xf;
        for(;;) {
            i2c_set_gpio(all);
            all ^= 0xf;
            nano_wait(500000000);
        }
    }

    char readback[100];
    i2c_read_flash(0x200, readback, len);
    if (strcmp(string,readback) == 0) {
        // String comparison matched.  Show the happy LEDs.
        for(;;) {
            for(int n=1; n <= 8; n <<= 1) {
                i2c_set_gpio(n);
                int value = i2c_get_gpio();
                if ((value & 0xf) != n)
                    break;
                nano_wait(100000000); // 0.1 s
            }
        }
    } else {
        // String comparison failed.  Show the angry LEDs.
        int all = 0xf;
        for(;;) {
            i2c_set_gpio(all);
            all ^= 0xf;
            nano_wait(100000000);
        }
    }
}
