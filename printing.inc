include gui.inc

.code

inner_function PROC,x:REAL4 ; used by get_square_error to calculate the square error
	fld square_error ; load existing square error
	fld x
	fld x
	fmulp st(1),st ; x^2
	faddp st(1),st ; add x^2 to existing square error
	fstp square_error ; store the result in global square error
	ret
inner_function endp

get_square_error PROC,output:DWORD,labels:DWORD ; get the rounded square error to the global square error
	pusha
	mov square_error,0 ; initialize
	invoke matrix_minus,output,labels ; error
	invoke matrix_elementwize,eax,offset inner_function ; add error^2 to global square error
	invoke matrix_delete,eax ; delete the error matrix
	f_to_int square_error,square_error ; cast to int
	popa
	ret
get_square_error ENDP

print_square_error PROC,output:DWORD,labels:DWORD ; print the square error of the output and the labels
	push eax

	invoke get_square_error,output,labels
	invoke int_to_string,square_error ; get the square error and cast it to a string
	push eax
	invoke StdOut,eax
	linedown ; print and go down a line
	pop eax
	invoke Free,eax ; delete the string

	pop eax
	ret
print_square_error ENDP

show_progress PROC ; print the square error of a new batch
	invoke load_new_batch
	invoke hypot,images_matrix
	invoke print_square_error,eax,labels_matrix
	invoke matrix_delete,eax
	ret
show_progress ENDP

display_test_data PROC ; shows a comparison between the test data labels and the network's result
	
	;; initialize handles
	mov test_image_file_handler,fopen("t10k-images.idx3-ubyte")
	mov test_label_file_handler,fopen("t10k-labels.idx1-ubyte")
	mov eax,fread(test_image_file_handler,offset buffer,16) ; skip the first 4 integers (file type image, images count, image dimension 1, image dimension 2)
	mov eax,fread(test_label_file_handler,offset buffer,8) ; skip the first 2 integers (file type label,image count)
	
	;;;; test ;;;; 
	mov ecx,100 ; loop 100 times
	second_loop:
		push ecx
		
		mov eax,fread(test_image_file_handler,offset mnist_images_buffer,100*MNIST_IMAGE_SIZE) ; read 100 images and 100 labels to the buffers
		mov eax,fread(test_label_file_handler,offset mnist_labels_buffer,100)

		call mnist_data_to_matrix ; move the data to the matrices
		call mnist_label_to_matrix
		
		invoke hypot,images_matrix ; get the neural network's output for the images as input
		push eax ; need to delete it later
		
		mov edx,labels_matrix
		mov edx,[edx]
		mov edx,[edx] ; first element pointer
		mov ebx,eax
		mov ebx,[ebx]
		mov ebx,[ebx] ; first element pointer
		push byte ptr 100 ; counter
		
		the_print_loop:
		push ebx
		push edx
		
			mov m_list.items,ebx ; items = pointer to elements
			invoke hypot_get_max,offset m_list ; get maximum probability (digit predicted)
			invoke int_to_string,global_max_ind
			push eax
			invoke StdOut,eax ; print digit predicted
			pop eax
			invoke Free,eax ; delete string

			invoke StdOut,offset gap
		
			mov edx,[esp] ; edx -> labels matrix
			mov m_list.items,edx
			invoke hypot_get_max,offset m_list ; find maximum label (true label)
			invoke int_to_string,global_max_ind
			push eax
			invoke StdOut,eax ; print correct output
			pop eax
			invoke Free,eax ; delete string
		
			linedown

			invoke Sleep,500 ; wait 0.5 seconds until next print

		pop edx
		pop ebx
		add ebx,40 ; skip SIZEOF REAL4 * 10 elements in the matrix
		add edx,40
		dec byte ptr [esp] ; counter
		jnz the_print_loop

		pop eax
		invoke matrix_delete,eax
		
		inc esp ; delete counter

		pop ecx
		dec ecx
	jnz second_loop
	ret
display_test_data ENDP