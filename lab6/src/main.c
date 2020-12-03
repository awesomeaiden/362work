//=============================================================================
// ECE 362 lab experiment 6 -- Analog Input/Output
//=============================================================================

#include "stm32f0xx.h"
#include <math.h>

// Be sure to change this to your login...
const char login[] = "gonza487";

void internal_clock(void);
void display_float(float);
void control(void);

//============================================================================
// setup_adc()    (Autotest #1)
// Configure the ADC peripheral and analog input pins.
// Parameters: none
//============================================================================
void setup_adc(void)
{
    // Enable the clock to GPIO Port A
    RCC -> AHBENR |= RCC_AHBENR_GPIOAEN;
    // Change the configuration only for pins 0 and 1 for analog operation
    GPIOA -> MODER |= 15;
    // Enable the clock to the ADC peripheral
    RCC -> APB2ENR |= RCC_APB2ENR_ADC1EN;
    // Turn on the "high-speed internal" 14 MHz clock (HSI14)
    RCC -> CR2 |= RCC_CR2_HSI14ON;
    // Wait for the 14 MHz clock to be ready
    while (!(RCC -> CR2 & RCC_CR2_HSI14RDY));
    // Enable the ADC by setting the ADEN bit in the CR register
    ADC1 -> CR |= ADC_CR_ADEN;
    // Wait for the ADC to be ready
    while (!(ADC1 -> ISR & ADC_ISR_ADRDY));
}

//============================================================================
// start_adc_channel()    (Autotest #2)
// Select an ADC channel, and initiate an A-to-D conversion.
// Parameters: n: channel number
//============================================================================
void start_adc_channel(int n)
{
    // Select ADC channel specified
    ADC1 -> CHSELR &= 0;
    ADC1 -> CHSELR |= (1 << n);
    // Wait for the ADRDY bit to be set in the ADC's ISR register
    while (!(ADC1 -> ISR & ADC_ISR_ADRDY));
    // Set the ADSTART bit in the CR register
    ADC1 -> CR |= ADC_CR_ADSTART;
}

//============================================================================
// read_adc()    (Autotest #3)
// Wait for A-to-D conversion to complete, and return the result.
// Parameters: none
// Return value: converted result
//============================================================================
int read_adc(void)
{
    // Wait for EOC bit to be set in the ADC's ISR register
    while (!(ADC1 -> ISR & ADC_ISR_EOC));
    // Return the value read from the ADC's DR register
    int val = ADC1 -> DR;
    return val;
}

//============================================================================
// setup_dac()    (Autotest #4)
// Configure the DAC peripheral and analog output pin.
// Parameters: none
//============================================================================
void setup_dac(void)
{
    // Enable the clock to GPIO Port A
    RCC -> AHBENR |= RCC_AHBENR_GPIOAEN;
    // Change the configuration only for pin 4 for analog operation
    GPIOA -> MODER |= 768;
    // Enable the RCC clock for the DAC
    RCC -> APB1ENR |= RCC_APB1ENR_DACEN;
    // Select a software trigger for the DAC
    DAC -> CR |= DAC_CR_TSEL1;
    // Enable the trigger for the DAC
    DAC -> CR |= DAC_CR_TEN1;
    // Enable the DAC
    DAC -> CR |= DAC_CR_EN1;
}

//============================================================================
// write_dac()    (Autotest #5)
// Write a sample to the right-aligned 12-bit DHR, and trigger conversion.
// Parameters: sample: value to write to the DHR
//============================================================================
void write_dac(int sample)
{
    // Write sample to the 12-bit right-aligned data holding register
    DAC -> DHR12R1 = sample;
    // Set the software trigger bit to initiate the conversion
    DAC -> SWTRIGR |= DAC_SWTRIGR_SWTRIG1;
}


//============================================================================
// Parameters for the wavetable size and expected DAC rate.
//============================================================================
#define N 1000
#define RATE 20000
short int wavetable[N];

//============================================================================
// init_wavetable()    (Autotest #6)
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
int stepd = 0;
int offseta = 0;
int offsetb = 0;
int offsetc = 0;
int offsetd = 0;

//============================================================================
// set_freq_n()    (Autotest #7)
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

void set_freq_d(float f)
{
    if (f == 0) {
        stepd = 0;
        offsetd = 0;
    } else {
        f = f * (1 << 16);
        stepd = f * N / RATE;
    }
}

//============================================================================
// Timer 6 ISR    (Autotest #8)
// The ISR for Timer 6 which computes the DAC samples.
// Parameters: none
// (Write the entire subroutine below.)
//============================================================================
void TIM6_DAC_IRQHandler(void) {
    // Acknowledge the timer interrupt by writing a zero to the UIF bit
    TIM6 -> SR &= ~TIM_SR_UIF;
    // Trigger the DAC
    DAC -> SWTRIGR |= DAC_SWTRIGR_SWTRIG1;
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
    combined = (((int)combined * volume)>>16) + 2048;
    // If the adjusted sample is greater than 4095, set it to 4095 (clip)
    // If the adjusted sample is less than 0, set it to 0 (clip)
    if (combined > 4095) {
        combined = 4095;
    } else if (combined < 0) {
        combined = 0;
    }
    // Write the final adjusted sample to the DAC's DHR12R1 register
    DAC -> DHR12R1 = combined;
    start_adc_channel(0);
    volume = read_adc();
}


//============================================================================
// setup_tim6()    (Autotest #9)
// Configure Timer 6 to raise an interrupt RATE times per second.
// Parameters: none
//============================================================================
void setup_tim6(void)
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

int main(void)
{
    //internal_clock(); // Use the internal oscillator if you need it
    //autotest(); // test all of the subroutines you wrote
    init_wavetable();
    setup_dac();
    setup_adc();
    setup_tim6();
    set_freq_a(261.626); // Middle 'C'
    set_freq_b(329.628); // The 'E' above middle 'C'
    //control();
    while(1) {
        for(int out=0; out<4096; out++) {
            if ((TIM6->CR1 & TIM_CR1_CEN) == 0)
                write_dac(out);
            start_adc_channel(0);
            int sample = read_adc();
            float level = 2.95 * sample / 4095;
            display_float(level);
        }
    }
}
