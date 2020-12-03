.syntax unified
.cpu cortex-m0
.fpu softvfp
.thumb

//===================================================================
// ECE 362 Lab Experiment 3
// General Purpose I/O
//===================================================================

.equ  RCC,      0x40021000
.equ  AHBENR,   0x14
.equ  GPIOCEN,  0x00080000
.equ  GPIOBEN,  0x00040000
.equ  GPIOAEN,  0x00020000
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
// micro_wait: Wait for the number of microseconds specified
// in argument 1.  Maximum delay is (1<<31)-1 microseconds,
// or 2147 seconds.
.global micro_wait
micro_wait:
            movs r1, #10    // 1 cycle
loop:       subs r1, #1     // 1 cycle
            bne loop        // 3 cycles
            nop             // 1 cycle
            nop             // 1 cycle
            nop             // 1 cycle
            subs r0, #1     // 1 cycle
            bne  micro_wait // 3 cycles
            bx  lr          // 1 cycle
            // Total delay = r0 * (1 + 10*(1+3) + 1 + 1 + 1 + 1 + 3) + 1

//===========================================================
// enable_ports: Autotest check 1
// Enable Ports B and C in the RCC AHBENR
// No parameters.
// No expected return value.
.global enable_ports
enable_ports:
    push    {lr}
    // Student code goes here
	ldr r0, =RCC
	ldr r1, [r0, #AHBENR]
	ldr r2, =0x000C0000
	orrs r1, r2
	str r1, [r0, #AHBENR]
    // End of student code
    pop     {pc}

//===========================================================
// port_c_output: Autotest check 2
// Set bits 0-3 of Port C to be outputs.
// No parameters.
// No expected return value.
.global port_c_output
port_c_output:
    push    {lr}
    // Student code goes here
	ldr r0, =GPIOC
	ldr r1, [r0, #MODER]
	ldr r2, =0x00000055
	orrs r1, r2
	str r1, [r0, #MODER]
    // End of student code
    pop     {pc}

//===========================================================
// port_b_input: Autotest check 3
// Set bits 3-4 of Port B to be inputs.
// No parameters.
// No expected return value.
.global port_b_input
port_b_input:
    push    {lr}
    // Student code goes here
	ldr r0, =GPIOB
	ldr r1, [r0, #MODER]
	ldr r2, =0x000003C0
	bics r1, r2
	str r1, [r0, #MODER]
    // End of student code
    pop     {pc}

//===========================================================
// setpin: Autotest check 4
// Set the state of a single output pin to be high.
// Do not affect the other bits of the port.
// Parameter 1 is the GPIOx base address.
// Praameter 2 is the bit number of the pin.
// No expected return value.
.global setpin
setpin:
    push    {lr}
    // Student code goes here
	ldr r2, [r0, #BSRR] // value of GPIOx BSRR register
	movs r3, #1 // place a 1 in lowest bit
	lsls r3, r1 // shift 1 over r1 bits
	orrs r2, r3 // set pin r1 of GPIOx BSRR
	str r2, [r0, #BSRR] // write to GPIOx BSRR, which should enable ODR bit
    // End of student code
    pop     {pc}

//===========================================================
// clrpin: Autotest check 5
// Set the state of a single output pin to be low.
// Do not affect the other bits of the port.
// Parameter 1 is the GPIOx base address.
// Parameter 2 is the bit number of the pin.
// No expected return value.
.global clrpin
clrpin:
    push    {lr}
    // Student code goes here
	ldr r2, [r0, #BRR] // value of GPIOx BRR register
	movs r3, #1 // place a 1 in lowest bit
	lsls r3, r1 // shift 1 over r1 bits
	orrs r2, r3 // set pin r1 of GPIOx BRR
	str r2, [r0, #BRR] // write to GPIOx BRR, which should clear ODR bit
    // End of student code
    pop     {pc}

//===========================================================
// getpin: Autotest check 6
// Get the state of the input data register of
// the specified GPIO.
// Parameter 1 is GPIOx base address.
// Parameter 2 is the bit number of pin.
// The subroutine should return 0x1 if the pin is high
// or 0x0 if the pin is low.
.global getpin
getpin:
    push    {lr}
    // Student code goes here
	ldr r2, [r0, #IDR] // load value of GPIOx IDR
	lsrs r2, r1 // shift desired bit to the lowest bit position
	movs r3, #1 // put 1 in r3
	ands r2, r3 // zero every bit except for lowest (desired) bit
	movs r0, r2 // copy value to r0 for return
    // End of student code
    pop     {pc}

//===========================================================
// seq_leds: Autotest check 7
// Update the selected illuminated LED by turning off the currently
// selected LED, incrementing or decrementing 'state' and turning
// on the newly selected LED.
// Parameter 1 is the direction of the sequence
//
// Performs the following logic
// 1) clrpin(GPIOC, state)
// 2) If R0 == 0
//      (a) Increment state by 1
//      (b) Check if state > 3
//      (c) If so set it to 0
// 3) If R1 != 0
//      (a) Decrement state by 1
//      (b) Check if state < 0
//      (c) If so set it to 3
// 4) setpin(GPIOC, state)
// No return value
.data
.align 4
.global state
state: .word 0

.text
.global seq_leds
seq_leds:
    push    {r4,lr}
    // Student code goes here
    movs r4, r0 // copy input to r4
    ldr r0, =GPIOC // argument 1
    ldr r2, =state // address of argument 2
    ldr r1, [r2] // value of state
	bl clrpin
	movs r0, r4 // move input to r0
	cmp r0, #0 // compare to 0
	bne else1
	if1:
		ldr r1, =state // address of state
		ldr r0, [r1] // value of state
		adds r0, #1 // state = state + 1
		str r0, [r1]
		cmp r0, #3
		ble afterelse1
		ldr r1, =state
		ldr r0, [r0]
		movs r0, #0
		str r0, [r1]
		b afterelse1
	else1:
		ldr r1, =state // address of state
		ldr r0, [r1] // value of state
		subs r0, #1 // state = state - 1
		str r0, [r1] // save state value
		cmp r0, #0
		bge afterelse1
		ldr r1, =state // address of state
		ldr r0, [r1] // value of state
		movs r0, #3 // state = 3
		str r0, [r1] // save state value
	afterelse1:
		ldr r2, =state // address of state
		ldr r1, [r2] // value of state
		ldr r0, =GPIOC
		bl setpin
    // End of student code
    pop     {r4,pc}

//===========================================================
// detect_buttons: Autotest check 8
// Invoke seq_leds(0) when a high signal is detected on
// PB3 and wait for it to go low again.
// Invoke seq_leds(1) when a high signal is detected on
// PB4 and wait for it to go low again.
// No parameters.
// No expected return value.
.global detect_buttons
detect_buttons:
    push    {lr}
    // Student code goes here
    if2:
		ldr r0, =GPIOB
		movs r1, #3
		bl getpin
		cmp r0, #1
		bne if3
		movs r0, #0
		bl seq_leds
		while1:
			ldr r0, =GPIOB
			movs r1, #3
			bl getpin
			cmp r0, #1
			beq while1
			bne afterif3
	if3:
		ldr r0, =GPIOB
		movs r1, #4
		bl getpin
		cmp r0, #1
		bne afterif3
		movs r0, #1
		bl seq_leds
		while2:
			ldr r0, =GPIOB
			movs r1, #4
			bl getpin
			cmp r0, #1
			beq while2
			bne afterif3
		movs r0, #1
		bl seq_leds
	afterif3:
		nop
    // End of student code
    pop     {pc}

//===========================================================
// enable_port_a: Autotest check A
// Enable Port A in the RCC AHBENR
// No parameters.
// No expected return value.
.global enable_port_a
enable_port_a:
    push    {lr}
    // Student code goes here
	ldr r0, =RCC
	ldr r1, [r0, #AHBENR]
	ldr r2, =0x00020000
	orrs r1, r2
	str r1, [r0, #AHBENR]
    // End of student code
    pop     {pc}

//===========================================================
// port_a_input: Autotest check B
// Set bit 0 of Port A to be an input and enable its pull-down resistor.
// No parameters.
// No expected return value.
.global port_a_input
port_a_input:
    push    {lr}
    // Student code goes here
	ldr r0, =GPIOA
	ldr r1, [r0, #MODER]
	ldr r2, =0x00000003
	bics r1, r2 // clear bits 0 and 1
	str r1, [r0, #MODER] // save back to MODER
	ldr r1, [r0, #PUPDR]
	ldr r2, =0x00000002
	orrs r1, r2
	str r1, [r0, #PUPDR]
    // End of student code
    pop     {pc}

//===========================================================
// port_b_input2: Autotest check C
// Set bit 2 of Port B to be an input and enable its pull-down resistor.
// No parameters.
// No expected return value.
.global port_b_input2
port_b_input2:
    push    {lr}
    // Student code goes here
	ldr r0, =GPIOB
	ldr r1, [r0, #MODER]
	ldr r2, =0x00000030
	bics r1, r2
	str r1, [r0, #MODER]
	ldr r1, [r0, #PUPDR]
	ldr r2, =0x00000020
	orrs r1, r2
	str r1, [r0, #PUPDR]
    // End of student code
    pop     {pc}

//===========================================================
// port_c_output: Autotest check D
// Set bits 6-9 of Port C to be outputs.
// No parameters.
// No expected return value.
.global port_c_output2
port_c_output2:
    push    {lr}
    // Student code goes here
	ldr r0, =GPIOC
	ldr r1, [r0, #MODER]
	ldr r2, =0x00055000
	orrs r1, r2
	str r1, [r0, #MODER]
    // End of student code
    pop     {pc}

//===========================================================
// seq_leds2: Autotest check E
// Update the selected illuminated LED by turning off the currently
// selected LED, incrementing or decrementing 'state2' and turning
// on the newly selected LED.
// Parameter 1 is the direction of the sequence
//
// Performs the following logic
// 1) clrpin(PORTC, state2)
// 2) If R0 == 0
//      (a) Increment state2 by 1
//      (b) Check if state2 > 9
//      (c) If so set it to 6
// 3) If R1 != 0
//      (a) Decrement state2 by 1
//      (b) Check if state2 < 6
//      (c) If so set it to 9
// 4) setpin(PORTC, state2)
// No return value
.data
.align 4
.global state2
state2: .word 6

.text
.global seq_leds2
seq_leds2:
    push    {r4,lr}
    // Student code goes here
	movs r4, r0 // copy input to r3
    ldr r0, =GPIOC // argument 1
    ldr r2, =state2 // address of argument 2
    ldr r1, [r2] // value of state
	bl clrpin
	movs r0, r4 // move input to r0
	cmp r0, #0 // set flags
	bne else2
	if4:
		ldr r1, =state2 // address of state2
		ldr r0, [r1] // value of state2
		adds r0, #1 // state2 = state2 + 1
		str r0, [r1]
		cmp r0, #9
		ble afterelse2
		ldr r1, =state2 // address of state2
		ldr r0, [r1] // value of state2
		movs r0, #6
		str r0, [r1]
		b afterelse2
	else2:
		ldr r1, =state2 // address of state2
		ldr r0, [r1] // value of state2
		subs r0, #1 // state = state - 1
		str r0, [r1] // save state2 value
		cmp r0, #6
		bge afterelse2
		ldr r1, =state2 // address of state2
		ldr r0, [r1] // value of state2
		movs r0, #9 // state2 = 9
		str r0, [r1] // save state2 value
	afterelse2:
		ldr r2, =state2 // address of state2
		ldr r1, [r2] // value of state2
		ldr r0, =GPIOC
		bl setpin
    // End of student code
    pop     {r4,pc}

//===========================================================
// detect_buttons2: Autotest check F
// Invoke seq_leds2(0) when a high signal is detected on
// PA0 and wait for it to go low again.
// Invoke seq_leds2(1) when a high signal is detected on
// PB2 and wait for it to go low again.
// No parameters.
// No expected return value.
.global detect_buttons2
detect_buttons2:
    push    {lr}
    // Student code goes here
	if5:
		ldr r0, =GPIOA
		movs r1, #0
		bl getpin
		cmp r0, #1
		bne if6
		movs r0, #0
		bl seq_leds2
		while3:
			ldr r0, =GPIOA
			movs r1, #0
			bl getpin
			cmp r0, #1
			beq while3
			bne afterif6
	if6:
		ldr r0, =GPIOB
		movs r1, #2
		bl getpin
		cmp r0, #1
		bne afterif6
		movs r0, #1
		bl seq_leds2
		while4:
			ldr r0, =GPIOB
			movs r1, #2
			bl getpin
			cmp r0, #1
			beq while4
			bne afterif6
		movs r0, #1
		bl seq_leds2
	afterif6:
		nop
    // End of student code
    pop     {pc}

//===========================================================
// The main subroutine calls everything else.
// It never returns.
.global login
login: .string "gonza487" // Change to your login
.align 2
.global main
main:
	ldr r0, =_estack
	ldr r1, =Reset_Handler
	ldr r2, =CEC_CAN_IRQHandler
	ldr r3, =USB_IRQHandler
	//bl   autotest // Uncomment when most things are working
	bl   enable_ports
	bl   port_c_output
	// Turn on LED for PC0
	ldr  r0,=GPIOC
	movs r1,#0
	bl   setpin
	bl   port_b_input
	bl   enable_port_a
	bl   port_a_input
	bl   port_b_input2
	bl   port_c_output2
	// Turn on the LED for PC6
	ldr  r0,=GPIOC
	movs r1,#6
	bl   setpin
endless_loop:
	bl   detect_buttons
	bl   detect_buttons2
	b    endless_loop
