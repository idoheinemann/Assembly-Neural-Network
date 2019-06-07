include \masm32\include\masm32rt.inc
.const

INT_MIN_VALUE equ 10000000000000000000000000000000b

NOT_INT_MIN_VALUE equ -(INT_MIN_VALUE)-1

.data

str_length PROTO string:DWORD

parse_int PROTO string:DWORD

int_to_string PROTO number:DWORD

concat PROTO st1:DWORD,st2:DWORD

compare PROTO st1:DWORD,st2:DWORD

index_of PROTO st1:DWORD,patt:DWORD
.code

str_length PROC,string:DWORD ; returns the length of the string
	push ebx
	push ecx
	
	mov ebx,string
	xor ecx,ecx

	get_length_loop:
	mov al,[ebx+ecx]
	test al,al ; search for the null char
	jz finish ; finish if found
	inc ecx ; add 1 to counter if not found
	jmp get_length_loop

	finish:
	mov eax,ecx
	
	pop ecx
	pop ebx
	ret
str_length ENDP

parse_int PROC, string:DWORD ; returns (int)string
	push ebx
	push ecx
	push edx
	push edi
	push esi
	
	invoke str_length,string

	xor esi,esi
	mov ecx,eax ; ecx = length
	xor eax,eax
	mov edx,string
	xor edi,edi

	; check if negative
	cmp byte ptr [edx],"-"
	jnz AfterNeg ; if negative
		inc esi ; boolean indicating weather the number is negative
		inc edx ; skip the "-" sign
		dec ecx ; length is lenght-1 without the "-" sign
	AfterNeg:
	eval_loop:
		xor ebx,ebx
		mov bl,[edx+edi] ; the char
		sub bl,"0" ; from ascii to digit
		
		push edi
		push ecx
		push eax
		push edx
		
		mov eax,1 ; eax = exponent
		mov edi,10

		jmp check_pow_loop
		pow_loop:
			mul edi ; get 10^(ecx-1) to eax
			dec ecx
		check_pow_loop:
			cmp ecx,1
			jnz pow_loop
		after_pow_loop:
		
		mul ebx 
		mov ebx,eax ; ebx = digit*10^(ecx-1)
		
		pop edx
		pop eax
		pop ecx
		pop edi
		
		add eax,ebx ; add the number to the total sum
		
		inc edi
	loop eval_loop

	test esi,esi
	jz finish ; positive
		neg eax


	finish:
	pop esi
	pop edi
	pop edx
	pop ecx
	pop ebx
	ret
parse_int ENDP

int_to_string PROC, number:DWORD ; returns (string)number, pointer to the string through eax
	push ebx
	push ecx
	push edx
	push edi
	; protect values
	
	mov edi,10 ; for division

	xor ecx,ecx ; counter
	mov eax,number
	test eax,10000000000000000000000000000000b ; if negative
	jz get_length_loop
	neg eax

	get_length_loop:
		; length is amount of times needed to divide the number by 10 (the base) until it becomes zero
		inc ecx 
		xor edx,edx
		div edi
	test eax,eax
	jnz get_length_loop

	afterCount:

	inc ecx ; one after must be 0
	push ecx
	mov eax,number
	test eax,10000000000000000000000000000000b ; if negative
	jnz negativeAlloc
		invoke Alloc,ecx
		jmp afterAlloc
		negativeAlloc: ; allocate one more byte for the - sign
			inc ecx
			invoke Alloc,ecx
			mov byte ptr [eax],"-"
			inc eax ; skip the "-" sign
	afterAlloc:
	pop ecx
	dec ecx

	mov ebx,eax ; ebx = string pointer
	mov eax,number
	test eax,10000000000000000000000000000000b ; if negative
	jz afterNegation
		neg eax ; get absolute value
	afterNegation:

	push edx
	mov edi,10
	write_string_loop:
		xor edx,edx
		div edi ; remainder is the current units digit
		add dl,"0" ; to ascii
		mov [ebx+ecx-1],dl
	loop write_string_loop
	pop edx

	test number,10000000000000000000000000000000b ; if negative
	jz finish_1
		dec ebx ; if the number is negative the pointer should be one below for the "-" sign
	finish_1:
	mov eax,ebx
	
	pop edi
	pop edx
	pop ecx
	pop ebx
	; restore values
	ret
int_to_string ENDP

concat PROC,st1:DWORD,st2:DWORD ; st1+st2, pointer to the new string through eax
	push ebx
	push ecx
	push edx
	push edi
	push esi
	push es

	invoke str_length,st1
	mov ebx,eax
	invoke str_length,st2
	mov ecx,eax
	
	add ecx,ebx
	push ecx
	push eax
	push ebx
	invoke Alloc,ecx ; new string
	
	mov edi,eax ; new string
	pop ecx ; st1 len
	mov esi,st1 ; first string
	
	push ds
	pop es
	
	rep movsb ; copy from first string
	mov esi,st2
	pop ecx ; st2 len
	rep movsb ; copy from second string

	mov eax,edi
	pop ebx ; total length
	sub eax,ebx
	
	pop es
	pop esi
	pop edi
	pop edx
	pop ecx
	pop ebx
	ret
concat endp

compare PROC,st1:DWORD,st2:DWORD ; returns st1 == st2
	push ecx
	push edi
	push esi
	push es
	
	invoke str_length,st1
	push eax
	invoke str_length,st2
	cmp eax,[esp] ; if sizes don't match return false
	je afterFalse
	
		pop eax ; clear stack
		xor eax,eax ; zero eax
		; restore values
		pop es
		pop esi
		pop edi
		pop ecx
		ret
	afterFalse:
	
	pop ecx ; length
	push ds
	pop es ; extra to data
	
	mov esi,st1
	mov edi,st2
	repe cmpsb ; while edi[i] == esi[i] && ecx != 0
	je rettrue ; if loop was terminated because ecx reached 0 then ZF is still signed
	; if loop was terminated because edi[i] != esi[i] then ZF is not signed
	
	retfalse:
	xor eax,eax
	jmp finish
	rettrue:
	mov eax,1
	
	finish:
	pop es
	pop esi
	pop edi
	pop ecx
	ret
compare endp

index_of PROC, st1:DWORD,patt:DWORD ; first appearance of the pattern in the string, not a regex pattern btw
	push ecx
	
	invoke str_length,st1
	push eax
	invoke str_length,patt
	cmp eax,[esp] ; if length of patt is larger the st1 then the input is iligal
	jle ligalInput
		pop eax
		mov eax, -1 ; return -1 for iligal input
		pop ecx
		ret
	ligalInput:
	
	pop ecx ; st1 length
	
	push ebx
	push edx
	push esi
	push edi

	mov edi,st1
	mov esi,patt
	xor edx,edx
	xor ebx,ebx

	scan_loop:
		xor ebx,ebx ; index in patt
		push edi
		tempmatch_loop:
			cmp ebx,eax ; if reached the end of patt; eax = length of patt
			jz found
			mov dl,[edi] ; next char
			cmp dl,[esi+ebx] ; compare chars
			jnz nextloop ; if chars don't match the index wasn't a match
			inc edi
			inc ebx
		jmp tempmatch_loop
		nextloop:
		pop edi 
		inc edi ; previous index + 1
	loop scan_loop
	mov eax,-1 ; if loop reached it's end without finding a match
	jmp final
	found:
		invoke str_length,st1
		sub eax,ecx ; ecx is a backwords counter
		pop edi ; jumped while something was in the stack
		
	final:
	pop edi
	pop esi
	pop edx
	pop ebx
	pop ecx
	ret
index_of ENDP