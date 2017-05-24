	.file	"vreg.c"
__SREG__ = 0x3f
__SP_H__ = 0x3e
__SP_L__ = 0x3d
__CCP__  = 0x34
__tmp_reg__ = 0
__zero_reg__ = 1
	.global __do_copy_data
	.global __do_clear_bss
	.text
.global	__vector_14
	.type	__vector_14, @function
__vector_14:
	sei
	push __zero_reg__
	push r0
	in r0,__SREG__
	push r0
	clr __zero_reg__
	push r18
	push r19
	push r24
	push r25
/* prologue: Interrupt */
/* frame size = 0 */
	in r18,36-32
	in r19,(36)+1-32
	in r24,39-32
	andi r24,lo8(15)
	brne .L2
	ldi r24,lo8(824)
	ldi r25,hi8(824)
	sub r24,r18
	sbc r25,r19
	ldi r18,5
1:	lsl r24
	rol r25
	dec r18
	brne 1b
	subi r24,lo8(-(298))
	sbci r25,hi8(-(298))
	cp __zero_reg__,r24
	cpc __zero_reg__,r25
	brlt .L3
	ldi r24,lo8(1)
	ldi r25,hi8(1)
	rjmp .L4
.L3:
	ldi r18,hi8(411)
	cpi r24,lo8(411)
	cpc r25,r18
	brlt .L4
	ldi r24,lo8(410)
	ldi r25,hi8(410)
.L4:
	out (74)+1-32,r25
	out 74-32,r24
	ldi r24,lo8(-63)
	rjmp .L8
.L2:
	subi r18,lo8(-(8))
	sbci r19,hi8(-(8))
	ldi r24,4
1:	lsr r19
	ror r18
	dec r24
	brne 1b
	sts stkParam+20,r18
	ldi r24,lo8(-64)
.L8:
	out 39-32,r24
	sbi 38-32,6
/* epilogue start */
	pop r25
	pop r24
	pop r19
	pop r18
	pop r0
	out __SREG__,r0
	pop r0
	pop __zero_reg__
	reti
	.size	__vector_14, .-__vector_14
.global	vregInit
	.type	vregInit, @function
vregInit:
/* prologue: function */
/* frame size = 0 */
	ldi r24,lo8(-64)
	out 39-32,r24
	ldi r24,lo8(-115)
	out 38-32,r24
	sbi 38-32,6
/* epilogue start */
	ret
	.size	vregInit, .-vregInit
