.cpu cortex-m0
.thumb
.syntax unified
.fpu softvfp

//===================================================================
// ECE 362 Lab Experiment 5
// Timers
//===================================================================

// RCC configuration registers
.equ  RCC,      0x40021000
.equ  AHBENR,   0x14
.equ  GPIOCEN,  0x00080000
.equ  GPIOBEN,  0x00040000
.equ  GPIOAEN,  0x00020000
.equ  APB1ENR,  0x1c
.equ  TIM6EN,   1<<4
.equ  TIM7EN,   1<<5

// NVIC configuration registers
.equ NVIC, 0xe000e000
.equ ISER, 0x100
.equ ICER, 0x180
.equ ISPR, 0x200
.equ ICPR, 0x280
.equ IPR,  0x400
.equ TIM6_DAC_IRQn, 17
.equ TIM7_IRQn, 18

// Timer configuration registers
.equ TIM6, 0x40001000
.equ TIM7, 0x40001400
.equ TIM_CR1,  0x0
.equ TIM_CR2,  0x4
.equ TIM_DIER, 0xc
.equ TIM_SR,   0x10
.equ TIM_EGR,  0x14
.equ TIM_CNT,  0x24
.equ TIM_PSC,  0x28
.equ TIM_ARR,  0x2c

// Timer configuration register bits
.equ TIM_CR1_CEN,  1<<0
.equ TIM_DIER_UDE, 1<<8
.equ TIM_DIER_UIE, 1<<0
.equ TIM_SR_UIF,   1<<0

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

//===========================================================================
// enable_ports  (Autotest 1)
// Enable the RCC clock for GPIO ports B and C.
// Parameters: none
// Return value: none
.global enable_ports
enable_ports:
	push {lr}
	// Student code goes below
	// Enable RCC clock for GPIOB and GPIOC
	ldr r0, =RCC
	ldr r1, [r0, #AHBENR]
	ldr r2, =GPIOBEN
	ldr r3, =GPIOCEN
	orrs r1, r2
	orrs r1, r3
	str r1, [r0, #AHBENR]
	// Configure pins PB0-PB3 to be outputs
	ldr r0, =GPIOB
	ldr r1, [r0, #MODER]
	ldr r2, =0x000000FF
	bics r1, r2
	ldr r2, =0x00000055
	orrs r1, r2
	str r1, [r0, #MODER]
	// Configure pins PB4-PB7 to be inputs
	ldr r0, =GPIOB
	ldr r1, [r0, #MODER]
	ldr r2, =0x0000FF00
	bics r1, r2
	str r1, [r0, #MODER]
	// Configure pins PB4 – PB7 to be internally pulled low
	ldr r0, =GPIOB
	ldr r1, [r0, #PUPDR]
	ldr r2, =0x0000AA00
	orrs r1, r2
	ldr r2, =0x00005500
	bics r1, r2
	str r1, [r0, #PUPDR]
	// Configure pins PC0 – PC10 to be outputs
	ldr r0, =GPIOC
	ldr r1, [r0, #MODER]
	ldr r2, =0xFFFFFFFF
	bics r1, r2
	ldr r2, =0x00155555
	orrs r1, r2
	str r1, [r0, #MODER]
	// Student code goes above
	pop  {pc}

//===========================================================================
// Timer 6 Interrupt Service Routine  (Autotest 2)
// Parameters: none
// Return value: none
// Write the entire subroutine below
.global TIM6_DAC_IRQHandler
.type TIM6_DAC_IRQHandler, %function
TIM6_DAC_IRQHandler:
	push {lr}
	// Student code goes below
	// Acknowledge interrupt
	ldr r0, =TIM6
	ldr r1, =~TIM_SR_UIF
	str r1, [r0, #TIM_SR]
	// Toggle the PC6 bit
	ldr r0, =GPIOC
	ldr r1, [r0, #ODR]
	movs r2, #1
	lsrs r1, #6
	ands r1, r2
	cmp r1, #1
	bne pc6_toggle_on
	movs r2, #1
	lsls r2, #6
	ldr r1, [r0, #ODR]
	bics r1, r2 // clear PC6
	str r1, [r0, #ODR]
	pop  {pc}
	pc6_toggle_on:
	movs r2, #1
	lsls r2, #6
	ldr r1, [r0, #ODR]
	orrs r1, r2 // set PC6
	str r1, [r0, #ODR]
	// Student code goes above
	pop  {pc}

//===========================================================================
// setup_tim6  (Autotest 3)
// Configure timer 6
// Parameters: none
// Return value: none
.global setup_tim6
setup_tim6:
	push {lr}
	// Student code goes below
	// Enable the RCC clock to Timer 6
	ldr r0, =RCC
	ldr r1, [r0, #APB1ENR]
	ldr r2, =TIM6EN
	orrs r1, r2
	str r1, [r0, #APB1ENR]
	// Configure TIM6_PSC to prescale the system clock by 48000
	ldr r0, =TIM6
	ldr r1, =48000 - 1
	str r1, [r0, #TIM_PSC]
	// Configure TIM6_ARR to have a counting period of 500
	ldr r1, =500 - 1
	str r1, [r0, #TIM_ARR]
	// Configure TIM6_DIER to enable the UIE flag
	ldr r1, [r0, #TIM_DIER]
	ldr r2, =TIM_DIER_UIE
	orrs r1, r2
	str r1, [r0, #TIM_DIER]
	// Set TIM_CR1_CEN in TIM6_CR1
	ldr r1, [r0, #TIM_CR1]
	ldr r2, =TIM_CR1_CEN
	orrs r1, r2
	str r1, [r0, #TIM_CR1]
	// Enable the interrupt for TIM6 in the NVIC ISER (like lab 4)
	ldr r0, =NVIC
	ldr r1, =ISER
	ldr r2, =(1<<TIM6_DAC_IRQn)
	str r2, [r0, r1]
	// Student code goes above
	pop  {pc}

.data
.global display
display: .byte 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07
.global history
history: .space 16
.global offset
offset: .byte 0
.text

//===========================================================================
// show_digit  (Autotest 4)
// Set up the Port C outputs to show the digit for the current
// value of the offset variable.
// Parameters: none
// Return value: none
// Write the entire subroutine below.
.global show_digit
show_digit:
	push {lr}
	// Student code goes below
	ldr r1, =offset
	ldrb r0, [r1] // r0 = offset
	movs r1, #7
	ands r0, r1 // r0 = int off = offset & 7
	movs r1, r0
	lsls r1, #8 // r1 = (off << 8)
	ldr r2, =display
	ldrb r3, [r2, r0]
	orrs r1, r3 // r1 = (off << 8) | display[off]
	ldr r2, =GPIOC
	str r1, [r2, #ODR]
	// Student code goes above
	pop  {pc}

//===========================================================================
// get_cols  (Autotest 5)
// Return the current value of the PC8 - PC4.
// Parameters: none
// Return value: 4-bit result of columns active for the selected row
// Write the entire subroutine below.
.global get_cols
get_cols:
	push {lr}
	// Student code goes below
	ldr r0, =GPIOB
	ldr r1, [r0, #IDR]
	lsrs r1, #4 // r1 = (GPIOB->IDR >> 4)
	ldr r2, =0xf
	ands r1, r2 // r1 = (GPIOB->IDR >> 4) & 0xf
	movs r0, r1 // return value
	// Student code goes above
	pop  {pc}

//===========================================================================
// update_hist  (Autotest 6)
// Update the history byte entries for the current row.
// Parameters: r0: cols: 4-bit value read from matrix columns
// Return value: none
// Write the entire subroutine below.
.global update_hist
update_hist:
	push {r4, r5, r6, r7, lr}
	// Student code goes below
	// r0 = int cols
	ldr r2, =offset
	ldrb r1, [r2] // r1 = offset
	movs r2, #3
	ands r1, r2 // r1 = int row = offset & 3
	movs r2, #0 // r2 = int i = 0
	ldr r3, =history
	update_hist_for:
		movs r4, r1
		movs r5, #4
		muls r4, r5
		adds r4, r2 // r4 = 4*row+i
		ldrb r5, [r3, r4]
		lsls r5, #1 // r5 = (history[4*row+i]<<1)
		movs r6, r0
		lsrs r6, r2
		movs r7, #1
		ands r6, r7 // r6 = ((cols>>i)&1)
		adds r5, r6 // r5 = (history[4*row+i]<<1) + ((cols>>i)&1)
		strb r5, [r3, r4]
		adds r2, #1
		cmp r2, #4
		blt update_hist_for
	// Student code goes above
	pop  {r4, r5, r6, r7, pc}

//===========================================================================
// set_row  (Autotest 7)
// Set PB3 - PB0 to represent the row being scanned.
// Parameters: none
// Return value: none
// Write the entire subroutine below.
.global set_row
set_row:
	push {lr}
	// Student code goes below
	ldr r1, =offset
	ldr r0, [r1] // r0 = offset
	movs r1, #3
	ands r0, r1 // r0 = int row = offset & 3
	ldr r1, =0x000f0000
	movs r2, #1
	lsls r2, r0
	orrs r1, r2 // r1 = 0xf0000 | (1<<row)
	ldr r2, =GPIOB
	str r1, [r2, #BSRR]
	// Student code goes above
	pop  {pc}

//===========================================================================
// Timer 7 Interrupt Service Routine  (Autotest 8)
// Parameters: none
// Return value: none
// Write the entire subroutine below
.global TIM7_IRQHandler
.type TIM7_IRQHandler, %function
TIM7_IRQHandler:
	push {lr}
	// Student code goes below
	// Acknowledge interrupt
	ldr r0, =TIM7
	ldr r1, =~TIM_SR_UIF
	str r1, [r0, #TIM_SR]
	// Do stuff
	bl show_digit
	bl get_cols
	bl update_hist
	ldr r1, =offset
	ldr r0, [r1] // r0 = offset
	adds r0, #1 // r0 = (offset + 1)
	movs r1, #7
	ands r0, r1 // r0 = (offset + 1) & 0x7
	ldr r1, =offset
	str r0, [r1] // offset = (offset + 1) & 0x7
	bl set_row
	// Student code goes above
	pop  {pc}


//===========================================================================
// setup_tim7  (Autotest 9)
// Configure Timer 7.
// Parameters: none
// Return value: none
.global setup_tim7
setup_tim7:
	push {lr}
	// Student code goes below
	// Enable the RCC clock to Timer 7
	ldr r0, =RCC
	ldr r1, [r0, #APB1ENR]
	ldr r2, =TIM7EN
	orrs r1, r2
	str r1, [r0, #APB1ENR]
	// Disable the RCC clock for TIM6
	ldr r0, =RCC
	ldr r1, [r0, #APB1ENR]
	ldr r2, =TIM6EN
	bics r1, r2
	str r1, [r0, #APB1ENR]
	// Configure TIM7_PSC to prescale the system clock by 4800
	ldr r0, =TIM7
	ldr r1, =4800 - 1
	str r1, [r0, #TIM_PSC]
	// Configure TIM7_ARR to have a counting period of 10
	ldr r1, =10 - 1
	str r1, [r0, #TIM_ARR]
	// Configure TIM7_DIER to enable the UIE flag
	ldr r1, [r0, #TIM_DIER]
	ldr r2, =TIM_DIER_UIE
	orrs r1, r2
	str r1, [r0, #TIM_DIER]
	// Set TIM_CR1_CEN in TIM7_CR1
	ldr r1, [r0, #TIM_CR1]
	ldr r2, =TIM_CR1_CEN
	orrs r1, r2
	str r1, [r0, #TIM_CR1]
	// Enable the interrupt for TIM7 in the NVIC ISER (like lab 4)
	ldr r0, =NVIC
	ldr r1, =ISER
	ldr r2, =(1<<TIM7_IRQn)
	str r2, [r0, r1]
	// Student code goes above
	pop  {pc}


//===========================================================================
// get_keypress  (Autotest 10)
// Wait for and return the number (0-15) of the ID of a button pressed.
// Parameters: none
// Return value: button ID
.global get_keypress
get_keypress:
	push {lr}
	// Student code goes below
	get_keypress_for:
		wfi
		ldr r1, =offset
		ldrb r0, [r1] // r0 = offset
		movs r1, #3
		ands r0, r1 // r0 = (offset & 3)
		cmp r0, #0
		bne get_keypress_for // keep waiting
	movs r0, #0 // r0 = int i = 0
	ldr r1, =history
	get_keypress_inner:
		ldrb r2, [r1, r0]
		cmp r2, #1
		bne get_keypress_inner_more
		pop {pc} // return r0 = i
	get_keypress_inner_more:
		adds r0, #1
		cmp r0, #16
		blt get_keypress_inner
		b get_keypress_for
	// Student code goes above
	pop  {pc}


//===========================================================================
// handle_key  (Autotest 11)
// Shift the symbols in the display to the left and add a new digit
// in the rightmost digit.
// ALSO: Create your "font" array just above.
// Parameters: ID of new button to display
// Return value: none
.global font
font: .byte 0x06, 0x5b, 0x4f, 0x77, 0x66, 0x6d, 0x7d, 0x7c, 0x07, 0x7f, 0x67, 0x39, 0x49, 0x3f, 0x76, 0x5e
.global handle_key
handle_key:
	push {r4, lr}
	// Student code goes below
	ldr r1, =0xf
	ands r0, r1 // r0 = inputted int key & 0xf
	movs r1, #0 // r1 = int i = 0
	ldr r2, =display
	handle_key_for:
		adds r1, #1
		ldrb r3, [r2, r1] // r3 = display[i + 1]
		subs r1, #1
		strb r3, [r2, r1] // display[i] = display[i + 1]
		adds r1, #1
		cmp r1, #7
		blt handle_key_for
	ldr r4, =font
	ldrb r3, [r4, r0] // r3 = font[key]
	movs r1, #7 // sanity check
	strb r3, [r2, r1] // display[7] = font[key]
	// Student code goes above
	pop  {r4, pc}

.global login
login: .string "gonza487"
.align 2

//===========================================================================
// main
// Already set up for you.
// It never returns.
.global main
main:
	bl  check_wiring
	//bl  autotest
	//bl  enable_ports
	//bl  setup_tim6
	//bl  setup_tim7

endless_loop:
	//bl   get_keypress
	//bl   handle_key
	//b    endless_loop
