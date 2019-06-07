include math.inc

.data

Matrix STRUCT
	elements DWORD ? ; pointers to the first element in each row
	rows DWORD ?
	columns DWORD ?
Matrix ends

zero_matrix PROTO rows:DWORD,columns:DWORD

matrix_get_row PROTO mat:dword,row:DWORD

matrix_get_element PROTO mat:DWORD,row:DWORD,col:DWORD

matrix_set_element PROTO mat:DWORD,row:DWORD,col:DWORD,value:DWORD

matrix_set_row PROTO mat:DWORD,row:DWORD,reprow:DWORD

matrix_add PROTO dst:DWORD,scr:DWORD ; += 

.code

matrix_delete PROC,mat:DWORD ; name is self-exlaining
	pusha
	mov ebx,mat
	mov ebx,[ebx]
	
	invoke Free,[ebx] ; the byte data

	mov ebx,mat ; the pointers to the first element of every row
	invoke Free,[ebx]

	invoke Free,mat ; the matrix data (rows,columns,pointer to pointers
	popa
	RET
matrix_delete ENDP

zero_matrix PROC,rows:DWORD,columns:DWORD ; returns a pointer to a new zero matrix
	push ebx
	push ecx
	push edx

	invoke Alloc,SIZEOF Matrix ; create a new matrix
	push eax
	
	mov ebx,eax ; ebx is a pointer to the new matrix
	mov ecx,rows
	mov [ebx+4],ecx
	mov edx,columns
	mov [ebx+8],edx ; initialize rows and columns counters
	
	invoke Alloc,rows ; allocate room for pointers
	mov ebx,[esp]
	mov [ebx],eax ; place pointer to pointer in matrix.elements
	
	mov ecx,rows
	shl columns,2
	mov eax,columns
	mul ecx ; eax = rows*columns*4, byte size of the matrix
	invoke Alloc,eax
	mov edx,eax
	
	mov ebx,[esp]
	mov ebx,[ebx] ; ebx = pointers
	mov ecx,rows ; repeat rows times
	row_loop:
		mov [ebx],edx ; initialize the pointer
		add edx,columns ; jump up the matrix by columns*4
		add ebx,4 ; jump up the pointers by 4
	loop row_loop ; repeat rows times

	pop eax
	pop edx
	pop ecx
	pop ebx
	ret
zero_matrix ENDP

matrix_get_row PROC,mat:DWORD,row:DWORD ; returns a pointer to the first element in the row
	push ebx
	mov eax,mat
	mov eax,[eax] ; eax = pointer to pointers
	mov ebx,row
	shl ebx,2 ; ebx = rows*4
	mov eax,[eax+ebx] ; linear address of the pointer
	pop ebx
	ret
matrix_get_row endp

matrix_get_element PROC,mat:DWORD,row:DWORD,col:DWORD ; returns mat[row,col]
	push ebx
	
	invoke matrix_get_row,mat,row ; retrive the specific row to eax
	mov ebx,col
	shl ebx,2 ; linear location (row[col*4])
	mov eax,[eax+ebx]
	
	pop ebx
	ret
matrix_get_element ENDP

matrix_set_element PROC,mat:DWORD,row:DWORD,col:DWORD,value:DWORD ; mat[row,col] = value
	push ebx
	push eax
	
	invoke matrix_get_row,mat,row ; retrive to specific row to eax
	mov ebx,col
	shl ebx,2 ; linear location (row[col*4])
	push value
	pop DWORD PTR [eax+ebx] ; move the value
	
	pop eax
	pop ebx
	ret
matrix_set_element ENDP

matrix_set_row PROC,mat:DWORD,row:DWORD,reprow:DWORD ; copies the row specified by the row pointer reprow to the row of the matrix
	pusha
	
	invoke matrix_get_row,mat,row ; get the pointer to the row to eax
	mov ecx,mat
	mov ecx,[ecx+8] ; columns
	push ds
	pop es ; extra to data (thanks to CodeGuru Extreme for the idea)
	mov edi,eax ; destination = mat[row]
	mov esi,reprow ; source = reprow
	rep movsd ; copy dword from reprow to row columns times
	
	popa
	ret
matrix_set_row ENDP

matrix_load PROC,dst:DWORD,src:DWORD ; copies the destination matrix to the source matrix
	push eax
	push ecx
	
	mov ecx,dst
	mov ecx,[ecx+4] ; ecx = rows
	the_loop:
		dec ecx ; matrix is 0 based indexed, loop instruction is 1 based
		invoke matrix_get_row,src,ecx ; get the row pointer at src[ecx] to eax
		invoke matrix_set_row,dst,ecx,eax ; copy the row to dst[ecx]
		inc ecx
	loop the_loop ; repeat rows times
	
	pop ecx
	pop eax
	ret
matrix_load endp

matrix_add PROC,dst:DWORD,src:DWORD ; += instruction for matrices, dst += src
	pusha
	
	mov ebx,src
	mov ecx,[ebx+4] ; rows 
	push DWORD PTR [ebx+8] ; columns
	mov eax,dst
	mov ebx,[ebx] ; pointer to row pointers of source matrix
	mov eax,[eax] ; pointer to row pointers of destination matrix
	
	outer_loop: ; loop through every row
		push ecx
		dec ecx ; matrix is 0 bases, loop instruction is 1 based
		shl ecx,2
		mov edi,[eax+ecx] ; row from dst
		mov esi,[ebx+ecx] ; row from src 
		mov edx,[esp+4] ; temporary columns variable pushed to the stack earlier
		
		inner_loop: ; loop through every column
			push edx ; for every element in the row
			dec edx
			shl edx,2 ; linear location, row[col*4]
			f_add [edi+edx],[esi+edx] ; float addition macro, add element in source to element in destination
			pop edx
		dec edx ; loops backwords (from columns-1 to 0)
		jnz inner_loop
		
		pop ecx
	loop outer_loop
	pop edx ; delete temporary columns variable from the stack
	
	popa
	ret
matrix_add ENDP

matrix_plus PROC,dst:DWORD,src:DWORD ; +, returns a pointer to the new matrix dst+src
	mov eax,dst
	invoke zero_matrix,[eax+4],[eax+8] ; new zero matrix to size of the matrices
	invoke matrix_load,eax,dst ; copy the destination matrix
	invoke matrix_add,eax,src ; add the source matrix
	ret
matrix_plus ENDP

matrix_sub PROC,dst:DWORD,src:DWORD ;  -= instruction for matrices, dst-= src
	pusha
	
	mov ebx,src
	mov ecx,[ebx+4] ; ecx = rows
	push DWORD PTR [ebx+8] ; temporary columns variable
	mov eax,dst
	mov ebx,[ebx] ; pointer to the row pointers of source matrix
	mov eax,[eax] ; pointer to the row pointers of destination matrix
	
	outer_loop:
		push ecx
		dec ecx ; matrix is 0 based index, loop instruction is 1 based
		shl ecx,2 ; linear location
		mov edi,[eax+ecx] ; edi = dst[ecx]
		mov esi,[ebx+ecx] ; esi = dst[ecx]
		mov edx,[esp+4]
		
		inner_loop:
			push edx
			dec edx
			shl edx,2 ; linear location
			f_sub [edi+edx],[esi+edx] ; using the float sub macro
			pop edx
		dec edx
		jnz inner_loop
		
		pop ecx
	loop outer_loop
	
	pop edx ; delete temporary column variable
	
	popa
	ret
matrix_sub ENDP

matrix_minus PROC,dst:DWORD,src:DWORD ; -, returns a pointer to the new matrix dst-src
	mov eax,dst
	invoke zero_matrix,[eax+4],[eax+8] ; create a new matrix to size of the matrices
	invoke matrix_load,eax,dst ; copy the destination matrix to the new matrix
	invoke matrix_sub,eax,src ; substruct the source matrix
	ret
matrix_minus ENDP

matrix_elementwize_mul PROC,dst:DWORD,src:DWORD ; *=, multiply dst by src elementwize
	pusha
	
	mov ebx,src
	mov ecx,[ebx+4] ; rows
	push DWORD PTR [ebx+8] ; temp columns constant
	mov eax,dst
	mov ebx,[ebx]
	mov eax,[eax]
	
	outer_loop: ; loop for every row
		push ecx
		dec ecx ; matrix zero bases, loop 1 based
		shl ecx,2 ; linear location
		mov edi,[eax+ecx] ; row of destination
		mov esi,[ebx+ecx] ; row of source
		mov edx,[esp+4] ; columns
		
		inner_loop: ; loop for every column
			push edx
			dec edx
			shl edx,2 ; linear location
			f_mul [edi+edx],[esi+edx] ; float multiply macro
			pop edx
		dec edx
		jnz inner_loop
		
		pop ecx
	loop outer_loop
	
	pop edx ; remove temp columns constant
	popa
	ret
matrix_elementwize_mul ENDP

matrix_elementwize_times PROC,dst:DWORD,src:DWORD ; *, returns a pointer to the new matrix src*dst
	mov eax,dst
	invoke zero_matrix,[eax+4],[eax+8] ; new matrix to size of the destination matrix
	invoke matrix_load,eax,dst ; copy of the destination matrix
	invoke matrix_elementwize_mul,eax,src ; multiply elementwize by the source matrix
	ret
matrix_elementwize_times ENDP

matrix_mul PROC,mat1:DWORD,mat2:DWORD ; matrix multiplication, returns pointer to a new matrix
	push ebx
	push ecx
	push edx
	push esi
	push edi

	; mat1 is R(n*m), mat2 is R(m*k)
	; the returned matrix is R(n*k)
	mov ebx,mat1
	mov ecx,mat2
	invoke zero_matrix,[ebx+4],[ecx+8] ; new matrix with the amount of rows in mat1 and columns of mat2
	push eax ; pointer to the new matrix
	push REAL4 ptr 0 ; float sum = 0
	push DWORD ptr 0; int i = 0, row counter for mat1
	push DWORD ptr 0 ; int j = 0, column counter for mat2
	push DWORD ptr 0 ; int k = 0, row/column counter for mat2/mat1, respectively

	outer_loop_1: ; row loop for mat1
		mov [esp+4],DWORD PTR 0 ; j = 0
		
		outer_loop_2: ; column loop for mat2
			mov REAL4 PTR [esp+12],0 ; sum = 0
			mov [esp],DWORD PTR 0 ; k = 0
			
			inner_loop: ; sum loop
				mov ebx,[esp+8] ; ebx = i
				mov ecx,[esp] ; ecx = k
				invoke matrix_get_element,mat1,ebx,ecx ; mat1[i,k]
				fld_eax ; load to fpu stack
				
				mov ebx,[esp+4] ; ebx = j
				invoke matrix_get_element,mat2,ecx,ebx ; mat2[k,j]
				fld_eax ; load to fpu stack
				
				fmulp st(1),st ; mat1[i,k]*mat2[k,j]
				fld REAL4 PTR [esp+12] ; load sum
				faddp st(1),st ; add mat1[i,k]*mat2[k,j] to sum
				fstp REAL4 PTR [esp+12] ; store result in sum

			inc DWORD PTR [esp] ; k++
			mov eax,mat1
			mov eax,[eax+8] ; mat1.columns
			cmp eax,[esp] ; repeat until k = mat1.columns
			jnz inner_loop

			mov eax,[esp+16] ; the return matrix
			mov ebx,[esp+8] ; i
			mov ecx,[esp+4] ; j
			mov edx,[esp+12] ; sum
			invoke matrix_set_element,eax,ebx,ecx,edx ; return matrix at [i,j] = sum

		inc DWORD PTR [esp+4] ; j++
		mov eax,[esp+16]
		mov eax,[eax+8] ; new matrix columns
		cmp eax,[esp+4] ; repeat for j = 0 to columns
		jnz outer_loop_2
	
	inc DWORD PTR [esp+8] ; i++
	mov eax,[esp+16]
	mov eax,[eax+4] ; new matrix rows
	cmp eax,[esp+8] ; repeat for i=0 to rows
	jnz outer_loop_1
	
	add esp,16 ; delete temporary variables
	pop eax ; restore result matrix to eax

	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx
	ret
matrix_mul endp

matrix_scalar_mul PROC,mat:DWORD,scl:REAL4 ; matrix multiplication by a scalar, mat *= scl
	pusha
	
	mov ebx,mat
	mov ecx,[ebx+4] ; ecx = rows
	push DWORD PTR [ebx+8] ; temp column constant
	mov ebx,[ebx] ; ebx = pointer to rows pointers
	
	outer_loop: ; for every row
		push ecx
		dec ecx
		shl ecx,2 ; linear address of the row
		mov edi,[ebx+ecx] ; edi = row
		
		mov edx,[esp+4] ; edx = columns
		
		inner_loop: ; for every column
			push edx
			dec edx
			shl edx,2 ; linear address
			f_mul [edi+edx],scl ; float multiplication macro
			pop edx
		dec edx
		jnz inner_loop
		
		pop ecx
	loop outer_loop
	
	pop edx ; delete temp column constant
	
	popa
	ret	
matrix_scalar_mul ENDP

matrix_scalar_times PROC,mat:DWORD,scl:REAL4 ; *, returns a new matrix
	mov eax,mat
	invoke zero_matrix,[eax+4],[eax+8]
	invoke matrix_load,eax,mat
	invoke matrix_scalar_mul,eax,scl
	ret
matrix_scalar_times endp

matrix_elementwize PROC,mat:DWORD,func:DWORD ; function must be stdcall, and take one REAL4 argument; equivalent to element = f(element) for every element
	pusha
	
	mov ebx,mat
	mov ecx,[ebx+4] ; ecx = rows
	push DWORD PTR [ebx+8] ; temp columns
	mov ebx,[ebx] ; row pointers
	
	outer_loop: ; rows
		push ecx
		dec ecx
		shl ecx,2 ; linear address
		mov edi,[ebx+ecx] ; edi = row pointer
		
		mov edx,[esp+4] ; columns
		inner_loop: ; columns
			push edx
			dec edx
			shl edx,2 ; linear location
			
			push edi ; protect values
			push edx
			
			push REAL4 PTR [edi+edx] ; call the function with mat[row,col] as an argument
			call func
			
			pop edx
			pop edi ; restore values
			mov [edi+edx],eax ; move the result to the element location, mat[row,col] = f(mat[row,col])
			pop edx
		dec edx
		jnz inner_loop
		
		pop ecx
	loop outer_loop
	
	pop edx ; delete temp
	popa
	ret
matrix_elementwize ENDP

matrix_element_function PROC,mat:DWORD,func:DWORD ; function must be stdcall, and take one REAL4 argument; returns f(element) for each element
	mov eax,mat
	invoke zero_matrix,[eax+4],[eax+8] ; new matrix by dimensions of the source
	invoke matrix_load,eax,mat ; copy matrix
	invoke matrix_elementwize,eax,func ; perform the function on every element and get the result
	ret
matrix_element_function endp

random_matrix PROC,rows:DWORD,columns:DWORD ; returns a new matrix R(rows,columns) of random values between 0 and 1
	push ebx
	push ecx
	push edx
	
	invoke Alloc,SIZEOF Matrix ; create new matrix
	push eax
	
	mov ebx,eax
	mov ecx,rows
	mov [ebx+4],ecx
	mov edx,columns
	mov [ebx+8],edx ; initialize rows and column fields of the matrix
	
	invoke Alloc,rows ; allocate rows pointer
	mov ebx,[esp]
	mov [ebx],eax ; initialize rows pointer
	
	mov ecx,rows
	shl columns,2
	mov eax,columns
	mul ecx ; byte size of the matrix = rows*columns*4
	push eax
	invoke Alloc,eax ; allocate raw data
	
	pop ecx ; size of matrix
	mov edx,eax
	push edx ; edx = raw data pointer
	rand_loop: ; for every element set to random number between 0 and 1
		invoke random ; random number to eax
		mov [edx],eax ; store in element
		add edx,4
	loop rand_loop
	
	pop edx ; restore raw data pointer
	mov ebx,[esp] ; return matrix
	mov ebx,[ebx] ; row pointers
	mov ecx,rows
	row_loop: ; initialize row pointers
		mov [ebx],edx
		add edx,columns ; beginning of next row
		add ebx,4 ; next row pointer
	loop row_loop
	
	pop eax ; new matrix pointer to eax
	pop edx
	pop ecx
	pop ebx
	ret
random_matrix ENDP

ones_matrix PROC,rows:DWORD,columns:DWORD ; returns a matrix R(rows,columns) of ones
	push ebx
	push ecx
	push edx
	
	invoke Alloc,SIZEOF Matrix
	push eax ; new matrix
	
	mov ebx,eax
	mov ecx,rows
	mov [ebx+4],ecx
	mov edx,columns
	mov [ebx+8],edx ; initialize rows and columns fields
	
	invoke Alloc,rows ; allocate rows pointers
	mov ebx,[esp]
	mov [ebx],eax ; initialize rows pointers
	
	mov ecx,rows
	shl columns,2
	mov eax,columns
	mul ecx ; byte size = rows*columns*4
	push eax
	invoke Alloc,eax ; allocate raw data
	
	pop ecx ; matrix size
	mov edx,eax
	push edx
	ones_loop:
		fld1
		fstp REAL4 PTR [edx] ; store 1 in the element
		add edx,4
	loop ones_loop ; loop for every element
	
	pop edx ; first element pointer
	mov ebx,[esp] ; return matrix
	mov ebx,[ebx] ; rows pointers
	mov ecx,rows
	row_loop: ; set pointer for every row
		mov [ebx],edx
		add edx,columns ; next row
		add ebx,4 ; next pointer
	loop row_loop
	
	pop eax ; return matrix
	pop edx
	pop ecx
	pop ebx
	ret
ones_matrix ENDP

identity_matrix PROC,rows:DWORD,columns:DWORD ; returns the identity_matrix by the dimensions R(rows,columns)
	push ecx
	push edx
	
	invoke zero_matrix,rows,columns ; new matrix to dimensions R(rows,columns)

	mov ecx,rows
	cmp ecx,columns
	jl afterFoundMin ; get min(rows,columns) to ecx, number of times to loop
		mov ecx,columns
	afterFoundMin:

	fld1
	push DWORD PTR 0
	fstp DWORD PTR [esp]
	pop edx ; edx = 1.0

	ones_loop:
		dec ecx
		invoke matrix_set_element,eax,ecx,ecx,edx ; set mat[ecx,ecx] to 1
		inc ecx
	loop ones_loop ; loop min(rows,columns) times


	pop edx
	pop ecx
	ret
identity_matrix ENDP 

matrix_transpose PROC, mat:DWORD ; returns the transpose of the matrix through eax
	push ebx
	push ecx
	push edx

	mov ebx,mat
	invoke zero_matrix,[ebx+8],[ebx+4] ; new matrix by the transpose dimensions of mat
	
	mov ecx,[ebx+4] ; rows
	push DWORD PTR [ebx+8] ; columns
	mov ebx,eax ; return matrix
	
	outer_loop:
		mov edx,[esp] ; columns
		inner_loop:
			dec edx
			dec ecx
			invoke matrix_get_element,mat,ecx,edx ; get element at mat[ecx,edx]
			invoke matrix_set_element,ebx,edx,ecx,eax ; set return matrix at [edx,ecx] to mat[ecx,edx]
			inc edx
			inc ecx
		dec edx
		jnz inner_loop ; for every column
	loop outer_loop ; for every row
	
	mov eax,ebx ; return matrix

	pop edx ; delete temp columns

	pop edx
	pop ecx
	pop ebx
	ret
matrix_transpose ENDP

matrix_concat_rows PROC,mat1:DWORD, mat2:DWORD ; concat mat1 and mat2 by the rows, mat1 is R(n,m), mat2 is R(k,m), return is R(n+k,m)
	push ebx
	push ecx
	push edx

	mov ebx,mat1
	mov ecx,[ebx+4]
	mov ebx,mat2
	add ecx,[ebx+4] ; ecx = mat1.rows+mat2.rows, amount of rows in the new matrix
	
	invoke zero_matrix,ecx,[ebx+8] ; new matrix R(n+k,m)
	mov edx,eax
	mov eax,mat1
	mov ecx,[eax+4] ; ecx = mat1.rows
	
	loop_1: ; load from mat1 to return matrix
		dec ecx
		invoke matrix_get_row,mat1,ecx ; copy row from mat1 to return matrix
		invoke matrix_set_row,edx,ecx,eax
		inc ecx
	loop loop_1 ; loop mat1.rows times

	mov ebx,mat1
	mov ebx,[ebx+4] ; mat1.rows
	mov ecx,mat2
	mov ecx,[ecx+4] ; mat2.rows
	
	loop_2: ; load from mat2 to return matrix
		dec ecx
		invoke matrix_get_row,mat2,ecx ; get row at ecx
		add ecx,ebx ; set row at mat1.rows+ecx to mat2[ecx]
		; rows at 0 to mat1.rows-1 have been filled by rows at mat1
		invoke matrix_set_row,edx,ecx,eax
		sub ecx,ebx
		inc ecx
	loop loop_2 ; loop for each row in mat2

	mov eax,edx ; return matrix

	pop edx
	pop ecx
	pop ebx
	ret
matrix_concat_rows ENDP

matrix_concat_columns PROC,mat1:DWORD,mat2:DWORD ; concat mat1 and mat2 by the columns, mat1 is R(n,m), mat2 is R(n,k), return is R(n,m+k)
	push ebx
	
	; linear identity
	; [a;b] = [a.T,b.T].T 
	
	invoke matrix_transpose,mat1; mat1.T
	mov ebx,eax
	
	invoke matrix_transpose,mat2; mat2.T
	push eax ; to delete later
	
	invoke matrix_concat_rows,ebx,eax ; eax = [mat1.T, mat2.T]
	
	invoke matrix_delete,ebx ; delete mat1.T
	pop ebx
	invoke matrix_delete,ebx ; delete mat2.T
	mov ebx,eax
	invoke matrix_transpose,eax ; transpose of [mat1.T, mat2.T]
	invoke matrix_delete,ebx ; delete [mat1.T, mat2.T]

	pop ebx
	ret
matrix_concat_columns ENDP
