.syntax unified
.cpu cortex-m0
.fpu softvfp
.thumb

//===================================================================
// ECE 362 Lab Experiment 4
// Interrupts
//===================================================================

// RCC configuration registers
.equ  RCC,      0x40021000
.equ  AHBENR,   0x14
.equ  GPIOCEN,  0x00080000
.equ  GPIOBEN,  0x00040000
.equ  GPIOAEN,  0x00020000
.equ  APB2ENR,  0x18
.equ  SYSCFGCOMPEN, 1

// GPIO configuration registers
.equ  GPIOC,    0x48000800
.equ  GPIOB,    0x48000400
.equ  GPIOA,    0x48000000
.equ  MODER,    0x00
.equ  PUPDR,    0x0c
.equ  IDR,      0x10
.equ  ODR,      0x14
.equ  BSRR,     0x18
.equ  BRR,      0x28

// SYSCFG constrol registers
.equ SYSCFG, 0x40010000
.equ EXTICR1, 0x8
.equ EXTICR2, 0xc
.equ EXTICR3, 0x10
.equ EXTICR4, 0x14

// External interrupt control registers
.equ EXTI, 0x40010400
.equ IMR, 0
.equ EMR, 0x4
.equ RTSR, 0x8
.equ FTSR, 0xc
.equ SWIER, 0x10
.equ PR, 0x14

// Variables to register things for EXTI on pin 0
.equ EXTI_RTSR_TR0, 1<<0
.equ EXTI_IMR_MR0,  1<<0
.equ EXTI_PR_PR0,   1<<0
// Variables to register things for EXTI on pin 1
.equ EXTI_RTSR_TR1, 1<<1
.equ EXTI_IMR_MR1,  1<<1
.equ EXTI_PR_PR1,   1<<1
// Variables to register things for EXTI on pin 2
.equ EXTI_RTSR_TR2, 1<<2
.equ EXTI_IMR_MR2,  1<<2
.equ EXTI_PR_PR2,   1<<2
// Variables to register things for EXTI on pin 3
.equ EXTI_RTSR_TR3, 1<<3
.equ EXTI_IMR_MR3,  1<<3
.equ EXTI_PR_PR3,   1<<3
// Variables to register things for EXTI on pin 4
.equ EXTI_RTSR_TR4, 1<<4
.equ EXTI_IMR_MR4,  1<<4
.equ EXTI_PR_PR4,   1<<4

// SysTick counter variables...
.equ STK, 0xe000e010
.equ CSR, 0x0
.equ RVR, 0x4
.equ CVR, 0x8

// NVIC configuration registers
.equ NVIC, 0xe000e000
.equ ISER, 0x100
.equ ICER, 0x180
.equ ISPR, 0x200
.equ ICPR, 0x280
.equ IPR,  0x400
.equ EXTI0_1_IRQn,5  // External interrupt number for pins 0 and 1 is IRQ 5.
.equ EXTI2_3_IRQn,6  // External interrupt number for pins 2 and 3 is IRQ 6.
.equ EXTI4_15_IRQn,7 // External interrupt number for pins 4 - 15 is IRQ 7.

// GPIO configuration registers
.equ  GPIOC,    0x48000800
.equ  GPIOB,    0x48000400
.equ  GPIOA,    0x48000000
.equ  MODER,    0x00
.equ  PUPDR,    0x0c
.equ  IDR,      0x10
.equ  ODR,      0x14
.equ  BSRR,     0x18
.equ  BRR,      0x28

//===========================================================
// gcd
// Euclid's algorithm for Greatest Common Denominator
// Find the GCD of the first two parameters.
// Parameter2 1 and 2 are unsigned integers
// Write the entire subroutine below.
.global gcd
gcd:
	cmp r0, r1 // compare a and b
	beq gcd_end // if equal, skip while loop entirely
	gcd_while:
		bls gcd_else
		subs r0, r1 // a = a - b
		cmp r0, r1
		bne gcd_while
		b gcd_end
		gcd_else:
			subs r1, r0 // b = b - a
			cmp r0, r1
			bne gcd_while
	gcd_end:
		bx lr

//===========================================================
// enable_ports
// Enable the RCC clock for GPIO ports B and C.
// Parameters: none
// Write the entire subroutine below.
.global enable_ports
enable_ports:
	push    {lr}
	ldr r0, =RCC
	ldr r1, [r0, #AHBENR]
	ldr r2, =0x000C0000
	orrs r1, r2
	str r1, [r0, #AHBENR]
    pop     {pc}


//===========================================================
// port_c_output
// Configure PC6, PC7, PC8, and PC9 to be outputs.
// Do not modify any other pin's configuration.
// Parameters: none
// Write the entire subroutine below.
.global port_c_output
port_c_output:
	push    {lr}
	ldr r0, =GPIOC
	ldr r1, [r0, #MODER]
	ldr r2, =0x00055000
	orrs r1, r2
	str r1, [r0, #MODER]
    pop     {pc}


//===========================================================
// port_b_input
// Configure PB2, PB3, and PB4 to be inputs.
// Enable the pull-down resistor for PB2.
// Do not modify any other pin's configuration.
// Parameters: none
// Write the entire subroutine below.
.global port_b_input
port_b_input:
	push    {lr}
	ldr r0, =GPIOB
	ldr r1, [r0, #MODER]
	ldr r2, =0x000003F0
	bics r1, r2
	str r1, [r0, #MODER]
	ldr r1, [r0, #PUPDR]
	ldr r2, =0x00000020
	orrs r1, r2
	ldr r2, =0x00000010
	bics r1, r2
	str r1, [r0, #PUPDR]
    pop     {pc}


//===========================================================
// toggle_portc_pin
// Change the ODR value from 0 to 1 or 1 to 0 for a specified
// pin of Port C.
// Parameters: r0 holds the pin number to toggle
// Write the entire subroutine below.
.global toggle_portc_pin
toggle_portc_pin:
	push    {lr}
    ldr r1, =GPIOC // r1 = address of GPIOC
    ldr r2, [r1, #ODR] // load value of output data register
    lsrs r2, r0
    movs r3, #1
    ands r2, r3 // isolate pin value
    lsls r3, r0
    cmp r2, #0
    beq toggle_portc_pin_set
    ldr r2, [r1, #ODR]
    bics r2, r3
    str r2, [r1, #ODR]
    pop     {pc}
    toggle_portc_pin_set:
        ldr r2, [r1, #ODR]
        orrs r2, r3
        str r2, [r1, #ODR]
        pop     {pc}


//===========================================================
// SysTick_Handler
// The ISR for the SysTick interrupt.
// Call toggle_portc_pin(7).
// Parameters: none
.global SysTick_Handler
.type SysTick_Handler, %function
SysTick_Handler:
	push {lr}
	// Student code goes below
	movs r0, #7 // pin 7
	bl toggle_portc_pin
	// Student code goes above
	pop  {pc}


//===========================================================
// enable_systick
// Enable the SysTick interrupt to occur every 0.5 seconds.
// Parameters: none
.global enable_systick
enable_systick:
	push {lr}
	// Student code goes below
	ldr r0, =STK // address of SysTick registers
	movs r1, #0 // CVR value
	ldr r2, =2999999 // RVR value
	str r1, [r0, #CVR] // set CVR
	str r2, [r0, #RVR] // set RVR
	movs r1, #3 // STK_CSR value
	movs r2, #4 // CLKSOURCE bit to clear
	ldr r3, [r0, #CSR] // load value of CSR
	orrs r3, r1 // set TICKINT and ENABLE bits
	bics r3, r2 // clear CLKSOURCE bit (for 6 MHz clock)
	str r3, [r0, #CSR] // store value of CSR
	// Student code goes above
	pop  {pc}


//===========================================================
// Write the EXTI interrupt handler for pins 2 and 3 below.
// Copy the name from startup/startup_stm32.s, create a label
// of that name below, declare it to be global, and declare
// it to be a function.
// It acknowledge the pending bit for pin 3, and it should
// call toggle_portc_pin(8).
.global EXTI2_3_IRQHandler
.type EXTI2_3_IRQHandler, %function
EXTI2_3_IRQHandler:
	push {lr}
	ldr r0, =EXTI // r0 = EXTI base address
	ldr r1, =EXTI_PR_PR3 // value to load into EXTI_PR
	str r1, [r0, #PR] // r1 = EXTI_PR
	movs r0, #8 // pin 8
	bl toggle_portc_pin
	pop {pc}


//===========================================================
// Write the EXTI interrupt handler for pins 4-15 below.
// It should acknowledge the pending bit for pin4, and it
// should call toggle_portc_pin(9).
.global EXTI4_15_IRQHandler
.type EXTI4_15_IRQHandler, %function
EXTI4_15_IRQHandler:
	push {lr}
	ldr r0, =EXTI // r0 = EXTI base address
	ldr r1, =EXTI_PR_PR4 // value to load into EXTI_PR
	str r1, [r0, #PR] // r1 = EXTI_PR
	movs r0, #9 // pin 9
	bl toggle_portc_pin
	pop {pc}


//===========================================================
// enable_exti
// Enable the SYSCFG subsystem, and select Port B for
// pins 2, 3, and 4.
// Parameters: none
.global enable_exti
enable_exti:
	push {lr}
	// Student code goes below
	ldr r0, =RCC // base address of RCC registers
	ldr r1, [r0, #APB2ENR] // value of RCC_APB2ENR
	movs r2, #1 // SYSCFGCOMPEN bit
	orrs r1, r2 // set bit
	str r1, [r0, #APB2ENR] // store value back to RCC_APB2ENR
	ldr r0, =SYSCFG // base address of SYSCFG registers
	ldr r1, [r0, #EXTICR1] // load value of SYSCFG_EXTICR1
	ldr r2, =0x00001100 // associate pins 2 and 3 with port B
	orrs r1, r2
	str r1, [r0, #EXTICR1] // store value of SYSCFG_EXTICR1
	ldr r1, [r0, #EXTICR2] // load value of SYSCFG_EXTICR2
	ldr r2, =0x00000001 // associate pin 4 with port B
	orrs r1, r2
	str r1, [r0, #EXTICR2] // store value of SYSCFG_EXTICR2
	// Student code goes above
	pop  {pc}


//===========================================================
// init_rtsr
// Configure the EXTI_RTSR register so that an EXTI
// interrupt is generated on the rising edge of
// pins 2, 3, and 4.
// Parameters: none
.global init_rtsr
init_rtsr:
	push {lr}
	// Student code goes below
	ldr r0, =EXTI // base address of EXTI registers
	ldr r1, [r0, #RTSR] // load value of RTSR register
	ldr r2, =EXTI_RTSR_TR2
	orrs r1, r2
	ldr r2, =EXTI_RTSR_TR3
	orrs r1, r2
	ldr r2, =EXTI_RTSR_TR4
	orrs r1, r2
	str r1, [r0, #RTSR] // store value of RTSR register
	// Student code goes above
	pop  {pc}


///==========================================================
// init_imr
// Configure the EXTI_IMR register so that the EXTI
// interrupts are unmasked for pins 2, 3, and 4.
// Parameters: none
.global init_imr
init_imr:
	push {lr}
	// Student code goes below
	ldr r0, =EXTI // base address of EXTI registers
	ldr r1, [r0, #IMR] // load value of IMR register
	ldr r2, =EXTI_IMR_MR2
	orrs r1, r2
	ldr r2, =EXTI_IMR_MR3
	orrs r1, r2
	ldr r2, =EXTI_IMR_MR4
	orrs r1, r2
	str r1, [r0, #IMR] // store value of IMR register
	// Student code goes above
	pop  {pc}


//===========================================================
// init_iser
// Enable the two interrupts for EXTI pins 2-3 and EXTI pins 4-15.
// Do not enable any other interrupts.
// Parameters: none
.global init_iser
init_iser:
	push {lr}
	// Student code goes below
	// enable interrupts for EXTI pins 2-3
	ldr r2, =1<<EXTI2_3_IRQn
	ldr r0, =NVIC
	ldr r1, =ISER
	str r2, [r0, r1]
	// enable interrupts for EXTI pins 4-15
	ldr r2, =1<<EXTI4_15_IRQn
	ldr r0, =NVIC
	ldr r1, =ISER
	str r2, [r0, r1]
	// Student code goes above
	pop  {pc}


//===========================================================
// adjust_priorities
// Set the priority for the EXTI pins 2-3 interrupt to 192
// Set the priority for the EXTI pins 4-15 interrupt to 128
// Do not adjust the priority for any other interrupts.
.global adjust_priorities
adjust_priorities:
	push {lr}
	// Student code goes below
	ldr r0, =NVIC
	ldr r1, =IPR
	adds r1, #4 // increment to second 32-bit value
	ldr r2, [r0, r1] // load value of second word of IPR table
	ldr r3, =0xffff0000
	bics r2, r3 // clear out interrupt 6 and 7 priorities
	ldr r3, =0x80C00000
	orrs r2, r3 // set interrupt 6 and 7 priorities
	str r2, [r0, r1] // store new value of second word of IPR table
	// Student code goes above
	pop  {pc}

//===========================================================
// The main subroutine calls everything else.
// It never returns.
.global login
login: .string "gonza487" // Change to your login
.align 2
.global main
main:
	bl autotest // Uncomment when most things are working
	ldr  r0,=3000000000 // 3 billion
	ldr  r1,=750000000  // 750 million
	bl   gcd            // find the GCD
	// CHECK: Result in R0 now should be 750 million
	ldr  r0,=1125000000
	ldr  r1,=3000000000
	bl   gcd
	// CHECK: Result in R0 now should be 375 million

	bl enable_ports
	bl port_c_output
	bl port_b_input
	bl enable_systick

	bl enable_exti
	bl init_rtsr
	bl init_imr
	bl adjust_priorities
	bl init_iser

endless_loop:
	movs r0,#6
	bl   toggle_portc_pin
	ldr  r0,=4000000000
	ldr  r1,=1000
	bl   gcd
	b    endless_loop
