.cpu cortex-m0
.thumb
.syntax unified
.fpu softvfp

.global main
main:
	bl autotest
	bkpt

.global login
login:
	.asciz "gonza487"
.align 2
