
;External function used to Find out the name of th function given its address
extrn PEnter:Proc
extrn PExit:Proc

.code

PUSHREGS macro
	push rax
	push rcx
	push rdx
	push r8
    push r9
    push r10
    push r11
endm

POPREGS	macro
    pop r11
    pop r10
    pop r9
	pop	r8
	pop	rdx
	pop	rcx
	pop	rax
endm

;--------------------------------------------------------------------
; _penter procedure
;--------------------------------------------------------------------

PUBLIC _penter
_penter proc
	PUSHREGS
	
	sub  rsp,20h 

	mov  rcx,rsp
	mov  rcx,qword ptr[rcx+58h]
	sub  rcx,5

	call PEnter

	add  rsp,20h

	POPREGS
	ret
_penter endp

;--------------------------------------------------------------------
; _pexit procedure
;--------------------------------------------------------------------

PUBLIC _pexit
_pexit proc
	PUSHREGS
	
	sub  rsp,20h 
	
	mov  rcx,rsp
	mov  rcx,qword ptr[rcx+58h]

	call PExit

	add rsp,20h

	POPREGS
	ret
_pexit endp

end