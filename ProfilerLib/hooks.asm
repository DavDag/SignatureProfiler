.code

extrn PEnter:proc
extrn PExit:proc

PUSHREGS macro
	push rax
	lahf 
	push rax
	push rcx
	push rdx
	push r8
endm

POPREGS	macro
	pop	r8
	pop	rdx
	pop	rcx
	pop	rax
	sahf
	pop	rax
endm

;--------------------------------------------------------------------
; _penter procedure
;--------------------------------------------------------------------

PUBLIC _penter
_penter proc
	PUSHREGS

	;rdtsc
	mov	rcx, qword ptr [rsp + 28h]
	shl	rdx, 20h
	or  rdx, rax
	call PEnter

	POPREGS
	ret
_penter endp

;--------------------------------------------------------------------
; _pexit procedure
;--------------------------------------------------------------------

PUBLIC _pexit
_pexit proc
	PUSHREGS

	;rdtsc
	xor	rcx, rcx
	shl	rdx, 20h
	or  rdx, rax
	call PExit

	POPREGS
	ret
_pexit endp

end