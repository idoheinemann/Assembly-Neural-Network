include \masm32\include\masm32rt.inc

.686

.data
;;;;;;;;;; NOTE!! ALL MATH FUNCTIONS RETURN REAL4 VALUES THROUGH EAX ;;;;;;;;;

;;;; SERIES ;;;;

factor PROTO x:REAL4

;;;; LOGARITHMS AND EXPONENTIALS

log2 PROTO x:REAL4
ln PROTO x:REAL4
exp PROTO x:REAL4
pow PROTO x:REAL4,y:REAL4 ; x to the power of y
log PROTO x:REAL4,y:REAL4 ; logarithm base x of y

;;;; TRIGONOMETRY ;;;;

cos PROTO x:REAL4
sin PROTO x:REAL4
tan PROTO x:REAL4
tanh PROTO x:REAL4
cosh PROTO x:REAL4
sinh PROTO x:REAL4
acos PROTO x:REAL4
asin PROTO x:REAL4
atan PROTO x:REAL4
atanh PROTO x:REAL4
acosh PROTO x:REAL4
asinh PROTO x:REAL4

;;;; NOT SURE HOW TO DESCRIBE THIS ;;;;

random PROTO

.code
;;;; MEMORY TO MEMORY MACROS ;;;;

st0_to_eax MACRO ; name is self-explaining
	sub esp, 4
	fstp dword ptr [esp]
	pop eax
ENDM

fld_eax MACRO ; name also self-explaining
	push eax
	fld DWORD PTR [esp]
	pop eax
ENDM

f_add MACRO x,y ;x += y for float
	fld REAL4 PTR x
	fadd REAL4 PTR y
	fstp REAL4 PTR x
endm

f_sub MACRO x,y ; x -= y for float
	fld REAL4 PTR x
	fsub REAL4 PTR y
	fstp REAL4 PTR x
endm

f_mul MACRO x,y ; x *= y for float
	fld REAL4 PTR x
	fmul REAL4 PTR y
	fstp REAL4 PTR x
endm

f_div MACRO x,y ; x /= y for float
	fld REAL4 PTR x
	fdiv REAL4 PTR y
	fstp REAL4 PTR x
endm

f_mod MACRO x,y ; x %= y for float
	fld REAL4 PTR x
	fld REAL4 PTR y
	fprem st(1),st
	fstp st(0) ; pop
	fstp REAL4 PTR x
endm

f_to_int MACRO x,y ; x = (int)y ;  for x is int, y is float
	fld REAL4 PTR y
	fistp DWORD PTR x
endm

f_to_float MACRO x,y ; x = (float)y ;  for x is float, y is int
	fild DWORD PTR y
	fstp REAL4 PTR x
endm

ln PROC, x:REAL4 ; ln(x) = ln(2)*log2(x)
	fldln2
	fld REAL4 ptr x
	FYL2X
	st0_to_eax
	ret
ln ENDP

factor PROC, x:REAL4 ; x!
	fld x ; counter
	fld1 ; result
	the_loop:
		fldz
		fcomip st,st(2) ; if counter == 0 return result
		je the_finish
		fmul st,st(1) ; result *= counter
		fld1
		fsubp st(2),st ; counter--
	jmp the_loop
	the_finish:
	st0_to_eax
	fstp st(0) ; pop counter
	ret
factor endp

exp PROC, x:REAL4 ; returns e^x
	; fpu instruction didn't work so this is calculated by the taylor series
	; exp(x) = sum from n = 0 to inf [x^n/n!]
	push ecx
	sub esp,8
	fld1 ; first element of the series, x^0/0! == 1
	fstp DWORD PTR [esp] ; [esp] = result
	mov [esp+4],dword ptr 1 ; [esp+4] counter
	
	the_loop:

		fild DWORD PTR [esp+4]
		st0_to_eax ; eax -> counter as float
		invoke factor,eax
		
		; normalpow x,counter
		
		mov ecx,[esp+4] ; ecx -> counter as int
		fld x
		fld1 ; result of normalpow, temp y = 1
		pow_loop:
			fmul st,st(1) ; y*=x
		loop pow_loop ; repeat counter times, y = x*x*x*x... counter times = x^n
		
		fstp st(1) ; pop, y to st
		fld_eax ; st = n!
		fdivp st(1),st ; st(0) = x^n/n!
		fld DWORD PTR [esp] ; sum 
		fadd st,st(1) ; st = sum + (x^n/n!)
		fstp dword ptr [esp] ; store new sum in sum local variable
		fstp st(0) ; pop 
		inc DWORD PTR [esp+4] ; counter++ 
	cmp [esp+4],DWORD PTR 35 ; maximum factorial that isn't infinity
	jl the_loop

	mov eax,[esp] ; result
	add esp,8 ; delete locals
	
	pop ecx
	ret
exp ENDP

pow PROC, x:REAL4, y:REAL4 ; x^y
	invoke ln,x
	fld y
	fmul x ; y*ln(x)
	st0_to_eax
	invoke exp,eax ; x^y = exp(ln(x)*y)
	ret
pow endp

log PROC, x:REAL4, y:REAL4 ; logx(y) = log2(y)/log2(x)
	fld1
	fld y
	fyl2x ; log2(y)
	fld1
	fld x
	fyl2x ; log2(x)
	fdivp st(1),st ; log2(y)/log2(x)
	st0_to_eax
	ret
log endp

log2 PROC,x:REAL4 ; using fpu instruction
	fld1
	fld x
	fyl2x
	st0_to_eax
	ret
log2 endp

cos PROC,x:REAL4 ; fpu instruction
	fld x
	fcos
	st0_to_eax
	ret
cos endp

sin PROC,x:REAL4 ; fpu instruction
	fld x
	fsin
	st0_to_eax
	ret
sin endp

tan PROC,x:REAL4 ; fpu instruction
	fld x
	fptan
	fstp st(0) ; fptan pushes 1 to the stack
	st0_to_eax
	ret
tan endp

tanh PROC,x:REAL4 ; hyperbolic tangent = (exp(2x)-1)/(exp(2x)+1)
	fld x
	fadd x ; 2x
	st0_to_eax
	invoke exp,eax ; exp(2x)
	fld_eax
	fld_eax
	fld1
	fsub st(2),st ; exp(2x)-1
	fadd st(1),st ; exp(2x)+1
	fstp st(0) ; pop
	fdivp st(1),st ; st = (exp(2x)-1)/(exp(2x)+1)
	st0_to_eax
	ret
tanh endp

sinh PROC,x:REAL4 ; hyperbolic sine = (exp(x)-exp(-x))/2
	invoke exp,x
	fld_eax
	fld1
	fld_eax
	fdivp st(1),st ; exp(-x) = 1/exp(x)
	fsubp st(1),st ; st = exp(x)-exp(-x)
	fld1
	fld1
	faddp st(1),st ; 2.0
	fdivp st(1),st
	st0_to_eax
	ret
sinh ENDP

cosh PROC,x:REAL4 ; hyperbolic cosine = (exp(x)+exp(-x))/2
	invoke exp,x
	fld_eax
	fld1
	fld_eax
	fdivp st(1),st ; exp(-x) = 1/exp(x)
	faddp st(1),st ; st = exp(x)+exp(-x)
	fld1
	fld1
	faddp st(1),st ; 2.0
	fdivp st(1),st
	st0_to_eax
	ret
cosh ENDP

atan PROC,x:REAL4 ; fpu instruction
	fld x
	fld1 ; arctan(x/1) = arctan(x)
	fpatan
	st0_to_eax
	ret
atan ENDP

asin PROC,x:REAL4
	fld x
	fld1
	fld1
	fld x
	fmul x ; x^2
	fsubp st(1),st ; 1-x^2
	fsqrt
	faddp st(1),st ; 1+sqrt(1-x^2)
	fpatan ; arcsin(x) = 2*arctan(x/(1+sqrt(1-x^2)))
	fadd st,st(0) ; mul by 2
	st0_to_eax
	ret
asin ENDP

acos PROC,x:REAL4
	fld1
	fld x
	fmul x ; x^2
	fsubp st(1),st ; 1-x^2
	fsqrt ; st(0) = sqrt(1-x^2)
	fld1
	fld x
	faddp st(1),st ; st(0) = x+1
	fpatan ; arccos(x) = 2*arctan(sqrt(1-x^2)/x+1)
	fadd st,st(0) ; mul by 2
	st0_to_eax
	ret
acos ENDP

atanh PROC,x:REAL4 ; atanh(x) = ln((1+x)/(1-x))/2
	fld x
	fld x
	fld1
	fadd st(2),st ; st(2) = 1+x
	fsub st(1),st ; st(1) = 1-x
	fstp st(0) ; pop
	fdivp st(1),st ; (1+x)/(1-x)
	st0_to_eax
	invoke ln,eax
	fld_eax
	fld1
	fld1
	faddp st(1),st ; 2.0
	fdivp st(1),st ; divide by 2
	st0_to_eax
	ret
atanh ENDP

asinh PROC,x:REAL4
	fld x
	fmul x
	fld1
	faddp st(1),st
	fsqrt ; sqrt(x^2+1)
	fadd x
	st0_to_eax
	invoke ln,eax ; asinh(x) = ln(x+sqrt(x^2+1))
	ret
asinh ENDP

acosh PROC,x:REAL4
	fld x
	fmul x
	fld1
	fsubp st(1),st
	fsqrt ; sqrt(x^2+1)
	fadd x
	st0_to_eax
	invoke ln,eax ; acosh(x) = ln(x+sqrt(x^2-1))
	ret
acosh ENDP

random PROC ; returns a random number between 0 and 1
	push edx
	
	rdtsc ; edx:eax = systime
	ror eax,8 ; most changing number as the highest parts of the integer
	and eax,01111111111111111111111111111111b ; remove sign
	push eax
	fild DWORD PTR [esp] ; load random integer
	push DWORD PTR 01111111111111111111111111111111b
	fild DWORD PTR [esp] ; load int max value
	pop edx ; clear from stack
	fdivp st(1),st ; random number / max
	fstp dword ptr [esp] ; store result in stack
	pop eax ; result to eax
	
	pop edx
	ret
random ENDP