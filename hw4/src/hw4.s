.cpu cortex-m0
.thumb
.syntax unified
.fpu softvfp

.global login
login: .string "gonza487"
hello_str: .string "Hello, %s!\n"
.align 2
.global hello
hello:
	push {lr}
	ldr r0, =hello_str
	ldr r1, =login
	bl printf
	pop  {pc}

showmult2_str: .string "%d * %d = %d\n"
.align 2
.global showmult2
showmult2:
	push {lr}
	movs r2, r1 // r2 = b
	movs r1, r0 // r1 = a
	muls r0, r2 // r0 = a * b
	movs r3, r0 // r3 = a * b
	ldr r0, =showmult2_str
	bl printf
	pop  {pc}

// Add the rest of the subroutines here
showmult3_str: .string "%d * %d * %d = %d\n"
.align 2
.global showmult3
showmult3:
	push {r4, lr}
	movs r3, r2 // r3 = c
	movs r2, r1 // r2 = b
	movs r1, r0 // r1 = a
	muls r0, r2 // r0 = a * b
	muls r0, r3 // r0 = a * b * c
	movs r4, r0 // r4 = a * b * c
	ldr r0, =showmult3_str
	// push r4 to the stack to pass to printf
	sub sp, #4 // allocate 4 bytes on stack
	str r4, [sp, #0] // store r4 on the stack
	bl printf
	add sp, #4 // deallocate 4 bytes from the stack
	pop  {r4, pc}

listing_str: .string "%s %05d %s %d students in %s, %d\n"
.align 2
.global listing
listing:
	push {r4, lr}
	ldr r4, [sp, #8] // get season from current stack position
	sub sp, #12 // allocate three spaces on stack
	str r4, [sp, #4] // store season on the stack in 2nd to last spot
	ldr r4, [sp, #24] // get year from stack
	str r4, [sp, #8] // store year in 3rd to last spot
	str r3, [sp, #0] // store enrollment in last spot
	movs r3, r2 // move verb to proper spot
	movs r2, r1 // move course to proper spot
	movs r1, r0 // move school to proper spot
	ldr r0, =listing_str // load format string into first spot
	bl printf
	add sp, #12 // deallocate 3 spaces from the stack
	pop  {r4, pc}

.align 2
.global trivial
trivial:
	push {lr}
	// r0 = unsigned int n
	sub sp, #400 // allocate 100 words on stack
	movs r1, #0 // r1 = x
	trivialfor:
		cmp r1, #100
		bge trivialif
		movs r2, r1 // copy x to r2
		adds r2, #1 // r2 = x + 1
		lsls r1, #2
		mov r3, sp
		adds r3, r1
		str r2, [r3]
		lsrs r1, #2
		adds r1, #1
		b trivialfor
	trivialif:
		cmp r0, #99
		bls trivialend
		movs r0, #99
	trivialend:
		lsls r0, #2
		mov r3, sp
		adds r3, r0
		ldr r0, [r3]
		add sp, #400 // deallocate 100 words from stack
	pop  {pc}

.align 2
.global reverse_puts
reverse_puts:
	push {r4, r5, lr}
	// r0 = const char *s
	movs r4, r0 // r4 = input s
	bl strlen // r0 = unsigned int len
	movs r1, r4 // r1 = input s
	movs r2, r0 // len
	adds r2, #4 // len + 4
	ldr r3, =0xfffffffc
	ands r2, r3 // r2 = (len + 4) & ~3 = newlen
	movs r5, r2 // copy newlen to r5
	mov r3, sp
	subs r3, r2
	mov sp, r3 // allocated char buffer[newlen]
	movs r3, #0
	mov r2, sp
	strb r3, [r2, r0] // buffer[len] = 0
	movs r2, #0 // r2 = x = 0
	subs r0, #1
	reverse_putsfor:
		cmp r2, r0
		bhi reverse_putsend
		mov r3, sp // copy sp to r3
		adds r3, r0
		subs r3, r2 // r3 = sp + (len - 1 - x)
		ldrb r4, [r1, r2] // r4 = s[x]
		strb r4, [r3]
		adds r2, #1
		b reverse_putsfor
	reverse_putsend:
		adds r0, #1
		mov r0, sp
		bl puts
		add sp, r5 // deallocate buffer
		pop  {r4, r5, pc}

.align 2
.global sumsq
sumsq:
	push {r4, r5, r6, lr}
	// r0 = a, r1 = b
	sub sp, #400 // int tmp[100]
	cmp r0, #99
	bls sumsqafterif1
	sumsqif1:
		movs r0, #99
	sumsqafterif1:
		cmp r1, #99
		bls sumsqafterif2
	sumsqif2:
		movs r1, #99
	sumsqafterif2:
		movs r2, #1 // r2 = step
		cmp r0, r1
		beq sumsqif3
		cmp r0, r1
		bhi sumsqelseif
		b sumsqafterelseif
	sumsqif3:
		movs r2, #0 // step = 0
		b sumsqafterelseif
	sumsqelseif:
		ldr r2, =-1 // step = -1
	sumsqafterelseif:
		movs r3, #0 // r3 = x = 0
	sumsqfor1:
		cmp r3, #100
		bge sumsqafterfor1
		mov r4, sp // copy sp to r4
		movs r5, r3
		muls r5, r5 // r5 = x * x
		lsls r3, #2 // adjust for size of word
		adds r4, r3 // r4 = sp + 4x
		str r5, [r4] // tmp[x] = x * x
		lsrs r3, #2 // undo adjust for size of word
		adds r3, #1 // x += 1
		b sumsqfor1
	sumsqafterfor1:
		movs r3, #0 // r3 = sum
		movs r4, r0 // r4 = x = a
	sumsqfor2:
		mov r5, sp // copy sp to r5
		lsls r4, #2 // adjust x for size of word
		ldr r6, [r5, r4]
		lsrs r4, #2 // undo adjust x for size of word
		adds r3, r6
		cmp r4, r1
		beq sumsqend
		adds r4, r2 // x += step
		b sumsqfor2
	sumsqend:
		movs r0, r3 // return sum
		add sp, #400 // deallocate int tmp[100]
		pop  {r4, r5, r6, pc}
