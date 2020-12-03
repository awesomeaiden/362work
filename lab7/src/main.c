//============================================================================
// ECE 362 lab experiment 7 -- Pulse-Width Modulation
//============================================================================

#include "stm32f0xx.h"
#include <string.h> // for memset() declaration
#include <math.h>   // for M_PI

// Be sure to change this to your login...
const char login[] = "gonza487";

//============================================================================
// setup_tim1()    (Autotest #1)
// Configure Timer 1 and the PWM output pins.
// Parameters: none
//============================================================================
void setup_tim1()
{
    // Activate the RCC clock to GPIO Port A
    RCC -> AHBENR |= RCC_AHBENR_GPIOAEN;
    // Configure the MODER for the four pins to set them for alternate function use
    GPIOA -> MODER &= ~(3<<16);
    GPIOA -> MODER &= ~(3<<18);
    GPIOA -> MODER &= ~(3<<20);
    GPIOA -> MODER &= ~(3<<22);
    GPIOA -> MODER |= (2<<16);
    GPIOA -> MODER |= (2<<18);
    GPIOA -> MODER |= (2<<20);
    GPIOA -> MODER |= (2<<22);
    // Set the alternate function register to route the timer function to the external
    // pins.  The alternate function register is actually an array of two registers
    // TIM1 -> AFR[0] and TIM1 -> AFR[1].  You will have to choose the correct array element to update
    // TIM1_CH1 AF2 for PA8:
    GPIOA -> AFR[1] &= ~0xf;
    GPIOA -> AFR[1] |= 0x2;
    // TIM1_CH2 AF2 for PA9:
    GPIOA -> AFR[1] &= ~0xf0;
    GPIOA -> AFR[1] |= 0x20;
    // TIM1_CH3 AF2 for PA10:
    GPIOA -> AFR[1] &= ~0xf00;
    GPIOA -> AFR[1] |= 0x200;
    // TIM1_CH4 AF2 for PA11:
    GPIOA -> AFR[1] &= ~0xf000;
    GPIOA -> AFR[1] |= 0x2000;
    // Active the RCC clock to Timer 1
    RCC -> APB2ENR |= RCC_APB2ENR_TIM1EN;
    // You will be using Timer 1 for PWM output rather than interrupt generation.
    // In order for output to work AT ALL you must enable the MOE bit of the break and
    // dead-time register (BDTR).  Do that now so you don't forget.
    TIM1 -> BDTR |= 0x8000;
    // Set the prescaler to divide by 1
    TIM1 -> PSC = 0;
    // Set the ARR so that an update event occurs 20000 times per second
    TIM1 -> ARR = 2400 - 1;
    // Configure the output channels of Timer 1:
    // Configure the "capture/compare mode registers" CCMR1 and CCMR2 to set channels
    // 1, 2, 3, and 4 for PWM mode 1 (110).  There are two 3-bit fields in each register that
    // must be adjusted to accomplish this
    TIM1 -> CCMR1 &= ~TIM_CCMR1_OC1M_0; // turn off bit 0
    TIM1 -> CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2; // turn on bits 1 and 2
    TIM1 -> CCMR1 &= ~TIM_CCMR1_OC2M_0; // turn off bit 0
    TIM1 -> CCMR1 |= TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2; // turn on bits 1 and 2
    TIM1 -> CCMR2 &= ~TIM_CCMR2_OC3M_0; // turn off bit 0
    TIM1 -> CCMR2 |= TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2; // turn on bits 1 and 2
    TIM1 -> CCMR2 &= ~TIM_CCMR2_OC4M_0; // turn off bit 0
    TIM1 -> CCMR2 |= TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2; // turn on bits 1 and 2
    // Configure the CCMR2 register to set the "preload enable" bit only for channel 4
    TIM1 -> CCMR2 |= TIM_CCMR2_OC4PE;
    // Enable the (uninverted) channel outputs for all four channels by turning on the CC1E,
    // CC2E, etc. bits in the "capture/compare enable register" CCER.  Until you do this for each channel,
    // the timer will not affect the outputs
    TIM1 -> CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;
    // enable the timer
    TIM1 -> CR1 |= TIM_CR1_CEN;
}

//============================================================================
// Parameters for the wavetable size and expected synthesis rate.
//============================================================================
#define N 1000
#define RATE 20000
short int wavetable[N];

//============================================================================
// init_wavetable()    (Autotest #2)
// Write the pattern for one complete cycle of a sine wave into the
// wavetable[] array.
// Parameters: none
//============================================================================
void init_wavetable(void)
{
    for (int i = 0; i < N; i++) {
        short int val = 32767 * sin(2 * M_PI * i / N);
        wavetable[i] = val;
    }
}

//============================================================================
// Global variables used for four-channel synthesis.
//============================================================================
int volume = 2048;
int stepa = 0;
int stepb = 0;
int stepc = 0;
int stepd = 0; // not used
int offseta = 0;
int offsetb = 0;
int offsetc = 0;
int offsetd = 0; // not used

//============================================================================
// set_freq_n()    (Autotest #2)
// Set the four step and four offset variables based on the frequency.
// Parameters: f: The floating-point frequency desired.
//============================================================================
void set_freq_a(float f)
{
    if (f == 0) {
        stepa = 0;
        offseta = 0;
    } else {
        f = f * (1 << 16);
        stepa = f * N / RATE;
    }
}

void set_freq_b(float f)
{
    if (f == 0) {
        stepb = 0;
        offsetb = 0;
    } else {
        f = f * (1 << 16);
        stepb = f * N / RATE;
    }
}

void set_freq_c(float f)
{
    if (f == 0) {
        stepc = 0;
        offsetc = 0;
    } else {
        f = f * (1 << 16);
        stepc = f * N / RATE;
    }
}


//============================================================================
// Timer 6 ISR    (Autotest #2)
// The ISR for Timer 6 which computes the DAC samples.
// Parameters: none
// (Write the entire subroutine below.)
//============================================================================
void TIM6_DAC_IRQHandler(void) {
    // Acknowledge the timer interrupt by writing a zero to the UIF bit
    TIM6 -> SR &= ~TIM_SR_UIF;
    // Trigger the DAC
    //DAC -> SWTRIGR |= DAC_SWTRIGR_SWTRIG1;
    // Add stepn to offsetn
    offseta += stepa;
    offsetb += stepb;
    offsetc += stepc;
    offsetd += stepd;
    // If any of the offsetn variables are greater than or equal to the maximum
    // fixed-point value, subtract the maximum fixed-point value from it
    if ((offseta >> 16) >= N) {
        offseta -= N << 16;
    }
    if ((offsetb >> 16) >= N) {
        offsetb -= N << 16;
    }
    if ((offsetc >> 16) >= N) {
        offsetc -= N << 16;
    }
    if ((offsetd >> 16) >= N) {
        offsetd -= N << 16;
    }
    // Look up the four samples at each of the four offsets in the
    // wavetable array and add them together into a single combined sample variable
    float combined = wavetable[offseta >> 16] + wavetable[offsetb >> 16] + wavetable[offsetc >> 16] + wavetable[offsetd >> 16];
    // Reduce and shift the combined sample to the range of the DAC
    combined = (((int)combined * volume)>>17) + 1200;
    // If the adjusted sample is greater than 2400, set it to 2400 (clip)
    // If the adjusted sample is less than 0, set it to 0 (clip)
    if (combined > 2400) {
        combined = 2400;
    } else if (combined < 0) {
        combined = 0;
    }
    // Write the final adjusted sample to the DAC's DHR12R1 register
    TIM1 -> CCR4 = combined;
//    start_adc_channel(0);
//    volume = read_adc();
}

//============================================================================
// setup_tim6()    (Autotest #2)
// Set the four step and four offset variables based on the frequency.
// Parameters: f: The floating-point frequency desired.
//============================================================================
void setup_tim6()
{
    // Enable the RCC clock to Timer 6
    RCC -> APB1ENR |= RCC_APB1ENR_TIM6EN;
    // Set the Timer 6 PSC and ARR so that an update event occurs exactly 20000 times per second
    TIM6 -> PSC = 480 - 1;
    TIM6 -> ARR = 5 - 1;
    // Set the UIE bit in the DIER register to generate an interrupt for each update event
    TIM6 -> DIER |= TIM_DIER_UIE;
    // Enable the timer by setting the CEN bit of the CR1 register
    TIM6 -> CR1 |= TIM_CR1_CEN;
    // Write a 1 into the proper position of the NVIC -> ISER[0] register to enable the interrupt for Timer 6.
    NVIC -> ISER[0] |= (1 << 17);
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

char offset;
char history[16];
char display[8];
char queue[2];
int  qin;
int  qout;

//============================================================================
// show_digit()    (Autotest #4)
// Output a single digit on the seven-segment LED array.
// Parameters: none
//============================================================================
void show_digit()
{
    int off = offset & 7;
    GPIOC -> ODR = (off << 8) | display[off];
}

//============================================================================
// set_row()    (Autotest #5)
// Set the row active on the keypad matrix.
// Parameters: none
//============================================================================
void set_row()
{
    int row = offset & 3;
    GPIOB -> BSRR = 0xf0000 | (1 << row);
}

//============================================================================
// get_cols()    (Autotest #6)
// Read the column pins of the keypad matrix.
// Parameters: none
// Return value: The 4-bit value read from PC[7:4].
//============================================================================
int get_cols()
{
    int value = ((GPIOB -> IDR) >> 4) & 0xf;
    return value;
}

//============================================================================
// insert_queue()    (Autotest #7)
// Insert the key index number into the two-entry queue.
// Parameters: n: the key index number
//============================================================================
void insert_queue(int n)
{
    int value = 0x80 | n;
    queue[qin] = value;
    if (qin == 1) {
        qin = 0;
    } else {
        qin = 1;
    }
}

//============================================================================
// update_hist()    (Autotest #8)
// Check the columns for a row of the keypad and update history values.
// If a history entry is updated to 0x01, insert it into the queue.
// Parameters: none
//============================================================================
void update_hist(int cols)
{
    int row = offset & 3;
    int value = 0;
    for(int i=0; i < 4; i++) {
        value = (history[4*row+i]<<1) + ((cols>>i)&1);
        if (value == 0x1) {
            insert_queue(4*row+i);
        }
        history[4*row+i] = value;
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
    show_digit();
    int cols = get_cols();
    update_hist(cols);
    offset = (offset + 1) & 0x7; // count 0 ... 7 and repeat
    set_row();
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
    // exactly once per millisecond
    TIM7 -> PSC = 4800 - 1;
    TIM7 -> ARR = 10 - 1;
    // Enable the UIE bit in the DIER
    TIM7 -> DIER |= TIM_DIER_UIE;
    // Enable the timer
    TIM7 -> CR1 |= TIM_CR1_CEN;
    // Enable the Timer 7 interrupt in the NVIC ISER
    NVIC -> ISER[0] = (1 << TIM7_IRQn);
}

//============================================================================
// getkey()    (Autotest #11)
// Wait for an entry in the queue.  Translate it to ASCII.  Return it.
// Parameters: none
// Return value: The ASCII value of the button pressed.
//============================================================================
int getkey()
{
    for(;;) {
        asm volatile("wfi");
        if (queue[qout] != 0) {
            int value = queue[qout];
            queue[qout] = 0;
            if (qout == 1) {
                qout = 0;
            } else {
                qout = 1;
            }
            value = value & 0x7f;
            if (value == 0) {
                return '1';
            } else if (value == 1) {
                return '2';
            } else if (value == 2) {
                return '3';
            } else if (value == 3) {
                return 'A';
            } else if (value == 4) {
                return '4';
            } else if (value == 5) {
                return '5';
            } else if (value == 6) {
                return '6';
            } else if (value == 7) {
                return 'B';
            } else if (value == 8) {
                return '7';
            } else if (value == 9) {
                return '8';
            } else if (value == 10) {
                return '9';
            } else if (value == 11) {
                return 'C';
            } else if (value == 12) {
                return '*';
            } else if (value == 13) {
                return '0';
            } else if (value == 14) {
                return '#';
            } else if (value == 15) {
                return 'D';
            } else {
                return 0;
            }
        }
    }
}

//============================================================================
// This is a partial ASCII font for 7-segment displays.
// See how it is used below.
//============================================================================
const char font[] = {
        [' '] = 0x00,
        ['0'] = 0x3f,
        ['1'] = 0x06,
        ['2'] = 0x5b,
        ['3'] = 0x4f,
        ['4'] = 0x66,
        ['5'] = 0x6d,
        ['6'] = 0x7d,
        ['7'] = 0x07,
        ['8'] = 0x7f,
        ['9'] = 0x67,
        ['A'] = 0x77,
        ['B'] = 0x7c,
        ['C'] = 0x39,
        ['D'] = 0x5e,
        ['*'] = 0x49,
        ['#'] = 0x76,
        ['.'] = 0x80,
        ['?'] = 0x53,
        ['b'] = 0x7c,
        ['r'] = 0x50,
        ['g'] = 0x6f,
        ['i'] = 0x10,
        ['n'] = 0x54,
        ['u'] = 0x1c,
};

// Shift a new character into the display.
void shift(char c)
{
    memcpy(display, &display[1], 7);
    display[7] = font[c];
}

// Turn on the dot of the rightmost display element.
void dot()
{
    display[7] |= 0x80;
}

// Read an entire floating-point number.
float getfloat()
{
    int num = 0;
    int digits = 0;
    int decimal = 0;
    int enter = 0;
    memset(display,0,8);
    display[7] = font['0'];
    while(!enter) {
        int key = getkey();
        if (digits == 8) {
            if (key != '#')
                continue;
        }
        switch(key) {
        case '0':
            if (digits == 0)
                continue;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            num = num*10 + key-'0';
            decimal <<= 1;
            digits += 1;
            if (digits == 1)
                display[7] = font[key];
            else
                shift(key);
            break;
        case '*':
            if (decimal == 0) {
                decimal = 1;
                dot();
            }
            break;
        case '#':
            enter = 1;
            break;
        default: continue; // ABCD
        }
    }
    float f = num;
    while (decimal) {
        decimal >>= 1;
        if (decimal)
            f = f/10.0;
    }
    return f;
}

// Read a 6-digit BCD number for RGB components.
int getrgb()
{
    memset(display, 0, 8);
    display[0] = font['r'];
    display[1] = font['r'];
    display[2] = font['g'];
    display[3] = font['g'];
    display[4] = font['b'];
    display[5] = font['b'];
    int digits = 0;
    int rgb = 0;
    for(;;) {
        int key = getkey();
        if (key >= '0' || key <= '9') {
            display[digits] = font[key];
            digits += 1;
            rgb = (rgb << 4) + (key - '0');
        }
        if (digits == 6)
            break;
    }
    return rgb;
}

//============================================================================
// setrgb()    (Autotest #12)
// Accept a BCD-encoded value for the 3 color components.
// Update the CCR values appropriately.
// Parameters: rgb: the RGB color component values
//============================================================================
void setrgb(int rgb)
{
    // Get individual values
    char red1 = (rgb >> 16) & 0xf;
    char red2 = (rgb >> 20) & 0xf;
    char green1 = (rgb >> 8) & 0xf;
    char green2 = (rgb >> 12) & 0xf;
    char blue1 = rgb & 0xf;
    char blue2 = (rgb >> 4) & 0xf;
    double red = red1 + (red2 * 10);
    double green = green1 + (green2 * 10);
    double blue = blue1 + (blue2 * 10);
    int redval = (1 - (red / 100)) * 2400;
    int greenval = (1 - (green / 100)) * 2400;
    int blueval = (1 - (blue / 100)) * 2400;
    if (redval % 2 != 0) {
        redval++;
    }
    if (greenval % 2 != 0) {
        greenval++;
    }
    if (blueval % 2 != 0) {
        blueval++;
    }
    if (blue == 1){
        int here = 0;
    }
    TIM1 -> CCR1 = redval;
    TIM1 -> CCR2 = greenval;
    TIM1 -> CCR3 = blueval;
}

void internal_clock();
void demo();
void autotest();

int main(void)
{
    //internal_clock();
    //demo();
    //autotest();
    enable_ports();
    init_wavetable();
    set_freq_a(261.626); // Middle 'C'
    set_freq_b(329.628); // The 'E' above middle 'C'
    set_freq_c(391.996); // The 'G' above middle 'C'
    setup_tim1();
    setup_tim6();
    setup_tim7();

    display[0] = font['r'];
    display[1] = font['u'];
    display[2] = font['n'];
    display[3] = font['n'];
    display[4] = font['i'];
    display[5] = font['n'];
    display[6] = font['g'];
    for(;;) {
        char key = getkey();
        if (key == 'A')
            set_freq_a(getfloat());
        else if (key == 'B')
            set_freq_b(getfloat());
        else if (key == 'C')
            set_freq_c(getfloat());
        else if (key == 'D')
            setrgb(getrgb());
    }
}
