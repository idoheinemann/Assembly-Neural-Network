include learning_data.inc
include string_functions.inc
include linear.inc

.data


linedown MACRO
	invoke StdOut,offset endl
endm

; end of line and gap, respectively
endl byte 13,10,0
gap byte " ",0

; variables representing temporary square error, maximum probability and index of the probability, meaning the number
square_error DWORD ?
global_max REAL4 -1.0
global_max_ind DWORD ?

.code

; standart format:
;  each layer file starts with 2 dwords indicating the amount of rows and columns of the matrix, respectively
;  the rest of the file is the matrix information where each 4 bytes represent a REAL4 value of a weight

reserve_layer PROC layer:DWORD,layer_file_handle:HANDLE,fname:DWORD ; reserve the given layer in the given file by the standart format
	fclose layer_file_handle ; the file was opened during initialization
	invoke DeleteFileA,fname
	invoke CreateFileA,fname,GENERIC_READ or GENERIC_WRITE,NULL,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL ; delete and re-open to empty the content
	mov layer_file_handle,eax
	mov ebx,layer
	mov edx,offset buffer
	mov ecx,[ebx+4] ; rows of the matrix
	mov [edx],ecx ; write the rows to the buffer
	mov ecx,fwrite(layer_file_handle,offset buffer,4) ; write to the file from the buffer using the macro from the built-in library macros.asm
	mov ebx,layer
	mov eax,[ebx+4]
	shl eax,2 ; eax = layer.rows*4
	push eax ; the value will be needed later to calculate the byte size of the matrix (4*rows*columns)
	mov edx,offset buffer
	mov ecx,[ebx+8]
	mov [edx],ecx ; write to the buffer and then to the file the columns of the matrix
	mov ecx,fwrite(layer_file_handle,offset buffer,4)
	mov ebx,layer
	mov ecx,[ebx+8] ; ecx = layer.columns
	pop eax ; 4*rows
	mul ecx ; eax = 4*rows*columns , the byte size of the matrix
	mov ebx,[ebx]
	mov ebx,[ebx] ; ebx = pointer to the first number in the matrix
	mov eax,fwrite(layer_file_handle,ebx,eax) ; write the entire matrix to the file
	ret
reserve_layer ENDP

forwardpass MACRO input,layer,var ; feed forward from the input through the layer to the output (var)
	invoke matrix_mul,input,layer ; matrix multiplication between the input and the layer
	invoke matrix_elementwize,eax,offset sigmoid ; apply nonliniarity
	mov var,eax ; initialize output
endm

load_layer_file PROC,layer:DWORD,hnd:HANDLE ; loads the layer from an existing file according to the standart format, layer is a pointer to a pointer to a matrix, hnd is a pointer to a handle
	pusha
	mov eax,fread(hnd,offset buffer,4) ; read the rows to the buffer
	mov ecx,offset buffer
	push DWORD PTR [ecx] ; push the rows
	mov eax,fread(hnd,offset buffer,4) ; read columns
	mov ecx, offset buffer
	push DWORD PTR [ecx] ; push columns
	pop ecx ; columns
	pop edx ; rows
	invoke zero_matrix,edx,ecx ; new empty matrix
	mov ebx,layer ; addres of the layer
	mov [ebx],eax ; mov to the layer, which is a pointer to a matrix, the pointer to the new empty matrix
	shl ecx,2 ; *=4
	mov eax,edx
	mul ecx ; eax = rows*columns*4, byte size of the matrix
	mov ebx,[ebx] ; pointer to the matrix
	mov ebx,[ebx] ; pointer to the matrix data
	mov ecx,fread(hnd,[ebx],eax) ; read the entire byte data to the matrix
	popa
	ret
load_layer_file ENDP

sigmoid PROC,x:REAL4 ; the logistic function
	; the sigmoid function is the non-liniarity function used
	; it is 1/(1+exp(-x)) or exp(x)/(1+exp(x))
	invoke exp,x 
	fld_eax
	fld1
	fld_eax
	faddp st(1),st ; e^x+1
	fdivp st(1),st ; e^x/(e^x+1)
	st0_to_eax ; retirn through eax
	ret
sigmoid ENDP

sigtag PROC,y:REAL4 ; derivative of the logistic function with respect to it's output
	; the derivative of the logistic function f(x) is 
	; f'(x) = f(x)*(1-f(x))
	; as a transformation from the result of f(x) to f'(x) the function can be writen as
	; y = f(x)
	; f'(y) = y*(1-y)
	fld y
	fld1
	fld y
	fsubp st(1),st ; 1-y
	fmulp st(1),st
	st0_to_eax ; return through eax
	ret
sigtag ENDP

sub_05 PROC,x:REAL4 ; initialize the weight as a random number between -1 and 1 instead of 0 and 1
	; x = 2x-1
	fld x
	fld x
	faddp st(1),st ; 2x
	fld1
	fsubp st(1),st ; 2x-1
	st0_to_eax ; return through eax
	ret
sub_05 endp

mnist_data_to_matrix PROC ; load from the buffer, which is an unsigned byte representation of the matrix, to the matrix of real4 values between 0 and 1
	push ebx
	push ecx
	push edx
	push eax

	xor eax,eax
	xor ecx,ecx
	mov ebx,offset mnist_images_buffer
	the_loop:
		xor edx,edx ; counter for the row
		inner_loop:
			mov al,[ebx+edx] ; eax = 0-255, byte representing the grayscale value of the pixel
			push eax
			fild REAL4 PTR [esp] 
			fstp REAL4 PTR [esp] ; value
			push edx ; column
			push ecx ; row
			push images_matrix ; matrix
			call matrix_set_element
		inc edx
		cmp edx,MNIST_IMAGE_SIZE
		jne inner_loop
		add ebx,MNIST_IMAGE_SIZE
	inc ecx ; counter for the column
	cmp ecx,BATCH_SIZE
	jnz the_loop
	invoke matrix_scalar_mul,images_matrix,normalization_factor ; normalize by 1/255 to get numbers between 0 and 1

	pop eax
	pop edx
	pop ecx
	pop ebx
	ret
mnist_data_to_matrix endp

mnist_label_to_matrix PROC ; load from the buffer, which is an unsigned byte representation of the labels of numbers between 0-9, to the labels matrix of real4 values
	push ebx
	push ecx
	push edx
	push eax

	invoke matrix_delete,labels_matrix
	invoke zero_matrix,100,10
	mov labels_matrix,eax ; reset matrix to all 0 values
	xor eax,eax
	xor ecx,ecx
	mov ebx,offset mnist_labels_buffer
	the_loop:
		mov al,[ebx] ; byte representing the correct number
		fld1
		sub esp,4
		fstp DWORD PTR [esp] ; push 1.0
		push eax ; column as the byte
		push ecx ; row
		push labels_matrix
		call matrix_set_element ; set correct label to 1
		inc ebx
	inc ecx ; row counter
	cmp ecx,BATCH_SIZE
	jnz the_loop

	pop eax
	pop edx
	pop ecx
	pop ebx
	ret
ret
mnist_label_to_matrix ENDP

initialize_values PROC ; the name is pretty much self-explaining
	;; open files
	mov mnist_image_file_handler,fopen("train-images.idx3-ubyte")
	mov mnist_label_file_handler,fopen("train-labels.idx1-ubyte")

	;; initialize weights
	invoke CreateFile,offset layer0_file_name,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL
	mov layer0_file_handle,eax
	invoke GetLastError ; if file exists
	cmp eax, ERROR_FILE_EXISTS
	je load_layer_0 ; load file if file exist, else create new random matrix
		invoke random_matrix,MNIST_IMAGE_SIZE+1,256 ; the +1 represents the bias layer only existing on the first layer
		invoke matrix_elementwize,eax,offset sub_05 ; normalize from a range of 0 -> 1 to a range of -1 -> 1
		mov layer0,eax
		jmp start_layer_1
	load_layer_0:
		mov layer0_file_handle,fopen("layer0.nnl")
		invoke load_layer_file,offset layer0, layer0_file_handle
	
	; repeat the above for all 4 matrices
	start_layer_1:
	invoke CreateFile,offset layer1_file_name,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL
	mov layer1_file_handle,eax
	invoke GetLastError
	cmp eax, ERROR_FILE_EXISTS
	je load_layer_1
		invoke random_matrix,256,256 ; no bias layer, it becomes obselete when dealing with such large data
		invoke matrix_elementwize,eax,offset sub_05
		mov layer1,eax
		jmp start_layer_2
	load_layer_1:
		mov layer1_file_handle,fopen("layer1.nnl")
		invoke load_layer_file,offset layer1, layer1_file_handle


	start_layer_2:
	invoke CreateFile,offset layer2_file_name,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL
	mov layer2_file_handle,eax
	invoke GetLastError
	cmp eax, ERROR_FILE_EXISTS
	je load_layer_2
		invoke random_matrix,256,128  ; no bias layer, it becomes obselete when dealing with such large data
		invoke matrix_elementwize,eax,offset sub_05
		mov layer2,eax
		jmp start_layer_3
	load_layer_2:
		mov layer2_file_handle,fopen("layer2.nnl")
		invoke load_layer_file,offset layer2, layer2_file_handle
	
	start_layer_3:
	
	invoke CreateFile,offset layer3_file_name,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL
	mov layer3_file_handle,eax
	invoke GetLastError
	cmp eax, ERROR_FILE_EXISTS
	je load_layer_3
		invoke random_matrix,128,10  ; no bias layer, it becomes obselete when dealing with such large data
		invoke matrix_elementwize,eax,offset sub_05
		mov layer3,eax
		jmp init_input
	load_layer_3:
		mov layer3_file_handle,fopen("layer3.nnl")
		invoke load_layer_file,offset layer3, layer3_file_handle

	init_input:
	;; initialize input matrices
	invoke ones_matrix,100,MNIST_IMAGE_SIZE+1 ; +1 for the bias input
	mov images_matrix,eax
	invoke zero_matrix,100,10
	mov labels_matrix,eax

	;; initilize read location for files
	mov eax,fread(mnist_image_file_handler,offset buffer,16) ; skip the first 4 integers (file type image, images count, image dimention 1, image dimention 2)
	mov eax,fread(mnist_label_file_handler,offset buffer,8) ; skip the first 2 integers (file type label,image count)

	
	ret
initialize_values ENDP

load_new_batch PROC ; load a fresh mini-butch from the file to the matrices
	pusha
	;; load new values to the buffers
	mov eax,fread(mnist_image_file_handler,offset mnist_images_buffer,BATCH_SIZE*MNIST_IMAGE_SIZE)
	mov eax,fread(mnist_label_file_handler,offset mnist_labels_buffer,BATCH_SIZE)
	
	; load from buffer to matrix
	call mnist_data_to_matrix 
	call mnist_label_to_matrix
	popa
	ret
load_new_batch ENDP

iteration PROC ; preform a gradient decent iteration on the weights
	local tmp1:DWORD,tmp2:DWORD,tmp3:DWORD,output:DWORD ; temporary output for each layer
	local delta3:DWORD,delta2:DWORD,delta1:DWORD,delta0:DWORD ; temporary delta for neurons

	pusha

	invoke load_new_batch

	;; forward pass
	forwardpass images_matrix,layer0,tmp1
	forwardpass tmp1,layer1,tmp2
	forwardpass tmp2,layer2,tmp3
	forwardpass tmp3,layer3,output
	
	;; calculate delta

	invoke matrix_minus,labels_matrix,output ; error
	mov delta3,eax
	invoke matrix_element_function,output,offset sigtag
	invoke matrix_elementwize_mul,delta3,eax
	invoke matrix_delete,eax
	; output layer delta = error.*sigtag(output), .* stands for elementwize multiplication

	invoke matrix_element_function,tmp3,offset sigtag
	mov delta2,eax
	invoke matrix_transpose,layer3
	mov ebx,eax
	invoke matrix_mul,delta3,eax
	invoke matrix_delete,ebx
	invoke matrix_elementwize_mul,delta2,eax 
	invoke matrix_delete,eax
	; layer2 delta = sigma(layer3_delta*layer3) .* sigtag(output)

	invoke matrix_element_function,tmp2,offset sigtag
	mov delta1,eax
	invoke matrix_transpose,layer2
	mov ebx,eax
	invoke matrix_mul,delta2,eax
	invoke matrix_delete,ebx
	invoke matrix_elementwize_mul,delta1,eax 
	invoke matrix_delete,eax
	; layer1 delta = sigma (layer2_delta*layer2) .* sigtag(output)

	invoke matrix_element_function,tmp1,offset sigtag
	mov delta0,eax
	invoke matrix_transpose,layer1
	mov ebx,eax
	invoke matrix_mul,delta1,eax
	invoke matrix_delete,ebx
	invoke matrix_elementwize_mul,delta0,eax 
	invoke matrix_delete,eax
	; layer0 delta = sigma (layer1_delta*layer1) .* sigtag(output)


	;; readjust the weights

	;; deltaWeight = deltaNeuron*input
	
	invoke matrix_transpose,tmp3
	mov ebx,eax
	invoke matrix_mul,eax,delta3 ; inp1'
	invoke matrix_delete,ebx
	invoke matrix_scalar_mul,eax,alpha
	invoke matrix_add,layer3,eax
	invoke matrix_delete,eax
	;; layer3

	invoke matrix_transpose,tmp2
	mov ebx,eax
	invoke matrix_mul,eax,delta2
	invoke matrix_delete,ebx
	invoke matrix_scalar_mul,eax,alpha
	invoke matrix_add,layer2,eax
	invoke matrix_delete,eax
	;; layer2


	invoke matrix_transpose,tmp1
	mov ebx,eax
	invoke matrix_mul,eax,delta1
	invoke matrix_delete,ebx
	invoke matrix_scalar_mul,eax,alpha
	invoke matrix_add,layer1,eax
	invoke matrix_delete,eax
	;; layer1


	invoke matrix_transpose,images_matrix
	mov ebx,eax
	invoke matrix_mul,eax,delta0
	invoke matrix_delete,ebx
	invoke matrix_scalar_mul,eax,alpha
	invoke matrix_add,layer0,eax
	invoke matrix_delete,eax
	;; layer0

	;; delete locals
	invoke matrix_delete,tmp1
	invoke matrix_delete,tmp2
	invoke matrix_delete,tmp3
	invoke matrix_delete,output
	invoke matrix_delete,delta0
	invoke matrix_delete,delta1
	invoke matrix_delete,delta2
	invoke matrix_delete,delta3

	popa
	ret
iteration ENDP

hypot PROC,x:DWORD ; feed forward the network and return the result
	local tmp1:DWORD,tmp2:DWORD,tmp3:DWORD 
	; forward feed everything
	forwardpass x,layer0,tmp1
	forwardpass tmp1,layer1,tmp2
	forwardpass tmp2,layer2,tmp3
	
	invoke matrix_mul,tmp3,layer3
	invoke matrix_elementwize,eax,offset sigmoid ; forward feed last layer

	; delete temporary matrices
	invoke matrix_delete,tmp1
	invoke matrix_delete,tmp2
	invoke matrix_delete,tmp3
	ret
hypot ENDP

learn PROC ; loop through the entire data and reajust the weights

	mov ecx,4 ; times to loop through the whole data
	outer_L:
	push ecx
		mov ecx,(60000/BATCH_SIZE) ; go through all the data
		the_loop:
			invoke iteration
		loop the_loop
		mov eax,fseek(mnist_image_file_handler,16,BEGIN) ; return file pointers to original location
		mov eax,fseek(mnist_label_file_handler,8,BEGIN)
	pop ecx
	dec ecx
	jnz outer_L

	; reserve the layers in the files
	invoke reserve_layer, layer0, layer0_file_handle,offset layer0_file_name
	invoke reserve_layer, layer1, layer1_file_handle,offset layer1_file_name
	invoke reserve_layer, layer2, layer2_file_handle,offset layer2_file_name
	invoke reserve_layer, layer3, layer3_file_handle,offset layer3_file_name

	
	ret
learn ENDP