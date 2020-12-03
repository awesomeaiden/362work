.cpu cortex-m0
.thumb
.syntax unified
.fpu softvfp

.data
.align 4
// Your global variables go here
.global result, source, str
result: .word 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
source: .word 1, 2, 2, 4, 5, 9, 12, 8, 9, 10, 11
str: .string "hello, 01234 world! 56789-+"

.text
.global intsub
intsub:
    // Your code for intsub goes here
    movs r0, #0 // for loop variable i
    ldr r1, =source // address of source array
    ldr r2, =result // address of result array
	for1:
		movs r3, #2
		ands r3, r0
		cmp r3, #2
		bne else1
		if1:
			lsls r0, #2 // correct for byte size
			ldr r3, [r1, r0] // load source[i] into r3
			lsls r3, #1 // left shift source[i] by 1
			str r3, [r2, r0] // store r3 into result[i]
			lsrs r0, #2 // undo byte size correction
			b for1end
		else1:
			lsls r0, #2 // correct for byte size
			adds r0, #4 // i + 1
			ldr r3, [r1, r0] // r3 = source[i + 1]
			subs r0, #4 // i
			// temporarily use r2 for arithmetic
			ldr r2, [r1, r0] // r2 = source[i]
			subs r3, r2 // source[i + 1] - source[i]
			ldr r2, =result // restore r2 to point to result array
			str r3, [r2, r0] // store result into result array
			lsrs r0, #2 // undo byte size correction
	for1end:
		adds r0, #1
		cmp r0, #10
		blt for1
    bx lr



.global charsub
charsub:
    // Your code for charsub goes here
	movs r0, #0 // for loop variable x
	ldr r1, =str // address of str string
	ldrb r2, [r1, r0] // r2 = str[x]
	for2:
		cmp r2, #0x30
		blt for2end
		cmp r2, #0x39
		bgt for2end
		adds r2, #0x31 // convert to lowercase character
		strb r2, [r1, r0]
	for2end:
		adds r0, #1
		ldrb r2, [r1, r0]
		cmp r2, #0
		bne for2
    bx lr



.global login
login: .string "gonza487" // Make sure you put your login here.
.align 2
.global main
main:
    bl autotest // uncomment AFTER you debug your subroutines
    bl intsub
    bl charsub
    bkpt
