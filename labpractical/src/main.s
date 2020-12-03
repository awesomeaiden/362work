.cpu cortex-m0
.thumb
.syntax unified
.fpu softvfp

//===================================================================
// ECE 362 Midterm Lab Practical
//===================================================================

// RCC configuration registers
.equ  RCC,      0x40021000
.equ  AHBENR,   0x14
.equ  APB2ENR,  0x18
.equ  APB1ENR,  0x1C
.equ  GPIOFEN,  0x00400000
.equ  GPIOEEN,  0x00200000
.equ  GPIODEN,  0x00100000
.equ  GPIOCEN,  0x00080000
.equ  GPIOBEN,  0x00040000
.equ  GPIOAEN,  0x00020000

// GPIO base addresses
.equ  GPIOA,    0x48000000
.equ  GPIOB,    0x48000400
.equ  GPIOC,    0x48000800
.equ  GPIOD,    0x48000c00
.equ  GPIOE,    0x48001000
.equ  GPIOF,    0x48001400

// GPIO Registers
.equ  MODER,    0x00
.equ  OTYPER,   0x04
.equ  OSPEEDR,  0x08
.equ  PUPDR,    0x0c
.equ  IDR,      0x10
.equ  ODR,      0x14
.equ  BSRR,     0x18
.equ  LCKR,     0x1c
.equ  AFRL,     0x20
.equ  AFRH,     0x24
.equ  BRR,      0x28

// Timer base addresses
.equ  TIM1,  0x40012c00
.equ  TIM2,  0x40000000
.equ  TIM3,  0x40000400
.equ  TIM6,  0x40001000
.equ  TIM7,  0x40001400
.equ  TIM14, 0x40002000
.equ  TIM15, 0x40014000
.equ  TIM16, 0x40014400
.equ  TIM17, 0x40014800

.equ  TIM_CR1,   0x00
.equ  TIM_CR2,   0x04
.equ  TIM_DIER,  0x0c
.equ  TIM_SR,    0x10
.equ  TIM_EGR,   0x14
.equ  TIM_CNT,   0x24
.equ  TIM_PSC,   0x28
.equ  TIM_ARR,   0x2c

// Timer configuration register bits
.equ TIM_CR1_CEN,  1<<0
.equ TIM_DIER_UDE, 1<<8
.equ TIM_DIER_UIE, 1<<0
.equ TIM_SR_UIF,   1<<0

// NVIC configuration registers
.equ NVIC, 0xe000e000
.equ ISER, 0x100
.equ ICER, 0x180

// You will need to add your own configuration symbols as needed.
.global setup_pins
setup_pins:
	push {lr}
	// Student code goes below
	// Enable the RCC clock to GPIO ports B, C, D, and F
	ldr r0, =RCC
	ldr r1, [r0, #AHBENR]
	ldr r2, =GPIOBEN
	orrs r1, r2
	ldr r2, =GPIOCEN
	orrs r1, r2
	ldr r2, =GPIODEN
	orrs r1, r2
	ldr r2, =GPIOFEN
	orrs r1, r2
	str r1, [r0, #AHBENR]
	// Configure as inputs: PB4, PB5, PB6, PB7, PB12, PB13, PB14, PC11, PC12, PC9
	ldr r0, =GPIOB
	ldr r1, [r0, #MODER]
	ldr r2, =0x3f00ff00
	bics r1, r2
	str r1, [r0, #MODER]
	ldr r0, =GPIOC
	ldr r1, [r0, #MODER]
	ldr r2, =0x03cc0000
	bics r1, r2
	str r1, [r0, #MODER]
	// Configure as outputs: PB0, PB1, PB2, PB3, PB10, PB15, PB8, PB9, PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC10
	ldr r0, =GPIOB
	ldr r1, [r0, #MODER]
	ldr r2, =0x802a00aa
	bics r1, r2
	ldr r2, =0x40150055
	orrs r1, r2
	str r1, [r0, #MODER]
	ldr r0, =GPIOC
	ldr r1, [r0, #MODER]
	ldr r2, =0x0020aaaa
	bics r1, r2
	ldr r2, =0x00105555
	orrs r1, r2
	str r1, [r0, #MODER]
	// Configure to be pulled up: PB4, PB5, PB14
	ldr r0, =GPIOB
	ldr r1, [r0, #PUPDR]
	ldr r2, =0x20000a00
	bics r1, r2
	ldr r2, =0x10000500
	orrs r1, r2
	str r1, [r0, #PUPDR]
	// Configure to be pulled down: PB6, PB7, PB13, PC9
	ldr r0, =GPIOB
	ldr r1, [r0, #PUPDR]
	ldr r2, =0x04005000
	bics r1, r2
	ldr r2, =0x0800a000
	orrs r1, r2
	str r1, [r0, #PUPDR]
	ldr r0, =GPIOC
	ldr r1, [r0, #PUPDR]
	ldr r2, =0x00040000
	bics r1, r2
	ldr r2, =0x00080000
	orrs r1, r2
	str r1, [r0, #PUPDR]
	// Student code goes above
	pop  {pc}

.global setup_timer
setup_timer:
	push {lr}
	// Student code goes below
	// Enable the RCC clock for Timer 14
	ldr r0, =RCC
	ldr r1, [r0, #APB1ENR]
	ldr r2, =1<<8
	orrs r1, r2
	str r1, [r0, #APB1ENR]
	// Set the timer to have an update event as close as possible to every x seconds
	ldr r0, =TIM14
	ldr r1, =365 - 1
	str r1, [r0, #TIM_PSC]
	ldr r1, =65492 - 1
	str r1, [r0, #TIM_ARR]
	// Do all that is necessary to enable an update interrupt for this timer
	// Configure TIM14_DIER to enable the UIE flag
	ldr r1, [r0, #TIM_DIER]
	ldr r2, =TIM_DIER_UIE
	orrs r1, r2
	str r1, [r0, #TIM_DIER]
	// Set TIM_CR1_CEN in TIM14_CR1
	ldr r1, [r0, #TIM_CR1]
	ldr r2, =TIM_CR1_CEN
	orrs r1, r2
	str r1, [r0, #TIM_CR1]
	// Enable the interrupt for TIM14 in the NVIC ISER
	ldr r0, =NVIC
	ldr r1, =ISER
	ldr r2, =(1<<19)
	str r2, [r0, r1]
	// Student code goes above
	pop  {pc}

.global TIM14_IRQHandler
.type TIM14_IRQHandler, %function
TIM14_IRQHandler:
	push {lr}
	// Student code goes below
	// Acknowledge interrupt
	ldr r0, =TIM14
	ldr r1, =~TIM_SR_UIF
	str r1, [r0, #TIM_SR]
	// Set a '0' on the output of PB0
	ldr r0, =GPIOB
	movs r1, #1
	str r1, [r0, #BRR]
	// Set a '1' on the output of PB1
	movs r1, #2
	str r1, [r0, #BSRR]
	// Read the value of PB4 from the Port B IDR.  If it is a '0', toggle all
	// eight bits of PC[7:0]. i.e., read the port C ODR, XOR it with 0xff, and
	// store the new 8-bit result back to the port C ODR
	ldr r1, [r0, #IDR]
	lsrs r1, #4
	movs r2, #1
	ands r1, r2
	cmp r1, #0
	bne tim14handler_skip
	ldr r0, =GPIOC
	ldr r1, [r0, #ODR]
	ldr r2, =0xff
	eors r1, r2
	str r1, [r0, #ODR]
	tim14handler_skip:
	// Student code goes above
	pop  {pc}

.global main
main:
    bl setup_pins
    bl setup_timer
endless: // Do nothing else
    wfi
    b endless
