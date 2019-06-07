include gui_data.inc
include learning_code.inc
.code

get_color_by_index PROC,ind:DWORD
	push ebx
	push ecx
	push edx
	mov ebx,28
	mov eax,ind
	xor edx,edx
	div ebx
	push eax

	invoke list_get_item,image_vector,ind
	push eax
	f_to_float [esp+4],[esp+4]
	f_div [esp+4],f_28
	f_mul [esp+4],[esp]

	
	push edx

	f_to_float [esp],[esp]
	f_div [esp],f_28
	f_mul [esp],[esp+4]

	f_mul [esp],f_255
	f_mul [esp+4],f_255
	f_mul [esp+8],f_255
	
	f_to_int [esp],[esp]
	f_to_int [esp+4],[esp+4]
	f_to_int [esp+8],[esp+8]

	pop edx
	mov al,dl
	shl eax,16
	pop edx
	mov ah,dl
	pop edx
	mov al,dl

	pop edx
	pop ecx
	pop ebx
	ret
get_color_by_index ENDP

get_index_by_point PROC,x:DWORD,y:DWORD ; returns the index of the coordinate in the image_vector list
	; index = y*28+x
	push edx
	push ecx
	
	sub x,DRAW_AREA_LEFT
	sub y,DRAW_AREA_TOP ; move to the beginning of the paint area
	
	xor edx,edx
	mov eax,y
	mov ecx,DRAW_AREA_DIMENSIONS ; dimension of the paint area
	div ecx ; y/dimensions
	mov edx,28
	mul edx ; eax = y/dimensions*28
	push eax
	
	xor edx,edx
	mov eax,x ; x/dimensions
	div ecx
	add [esp],eax ; y*28+x
	pop eax
	
	pop ecx
	pop edx
	ret
get_index_by_point ENDP

mk_button MACRO text,x,y,w,h,hwind,id ; creates and returns a new button with the specified text,coordinates, child of the window, and specified id
	invoke CreateWindowEx, 0, offset button_class, reparg(text),WS_CHILD or WS_VISIBLE, x, y, w, h, hwind, id, wcx.hInstance, NULL  
	EXITM < eax >
ENDM

; updated the color according to the brush update formula
get_fixed_color PROC,co:REAL4,dco:REAL4 ; c = c + (1-c)*dc, with c being a grayscale float between 0 and 1
	fld co
	fld1
	fld co
	fsubp st(1),st
	fld dco
	fmulp st(1),st
	faddp st(1),st
	st0_to_eax
	ret
get_fixed_color ENDP

set_delta_color MACRO delta ; only used in draw_to_vector, sets the pixel [px,py] to it's fixed color by delta
	invoke get_index_by_point,px,py
	mov ebx,eax
	invoke list_set,image_vector,ebx,rv(get_fixed_color,rv(list_get_item,image_vector,eax),delta)
ENDM
validate_pixel MACRO endlabel ;  only used in draw_to_vector, checks if the pixel [px,py] is in the drawing area bounds, if not it jumps to the specified label
	cmp px,20
	jl endlabel
	cmp py,20
	jl endlabel
	cmp px,300
	jg endlabel
	cmp py,300
	jg endlabel
ENDM
draw_to_vector PROC,px:DWORD,py:DWORD ; update every pixel in the rect {[px-1,py-1],[px+1,py+1]} by some delta color according to it's distance from [px,py]
	validate_pixel the_end
	set_delta_color float_1
	sub px,10
	validate_pixel after_0
	set_delta_color float_1
	after_0:
	add px,20
	validate_pixel after_1
	set_delta_color float_1
	after_1:
	sub px,10
	sub py,10
	validate_pixel after_2
	set_delta_color float_1
	after_2:
	add py,20
	validate_pixel after_3
	set_delta_color float_1
	after_3:
	sub px,10
	validate_pixel after_4
	set_delta_color float_07
	after_4:
	add px,20
	validate_pixel after_5
	set_delta_color float_07
	after_5:
	sub py,20
	validate_pixel after_6
	set_delta_color float_07
	after_6:
	sub px,20
	validate_pixel after_7
	set_delta_color float_07
	after_7:
	add px,10
	add py,10
	the_end:
	ret
draw_to_vector ENDP

get_rect_by_index PROC,index:DWORD ; get the rectangle on the screen by the index of the pixel in image_vector
	pusha
	
	mov temp_rect.left,DRAW_AREA_LEFT ; initialize
	mov temp_rect.top,DRAW_AREA_TOP
	
	mov eax,index
	mov ecx,28
	xor edx,edx
	div ecx ; x_index = index % 28
	mov ebx,eax ; y_index = index / 28
	
	mov eax,DRAW_AREA_DIMENSIONS
	mul edx ; pixel_y = y_index*dimension
	add temp_rect.left,eax
	
	mov eax,DRAW_AREA_DIMENSIONS
	mul ebx ; pixel_x = x_index*dimension
	add temp_rect.top,eax
	
	mov eax,temp_rect.top
	mov temp_rect.bottom,eax
	add temp_rect.bottom,DRAW_AREA_DIMENSIONS ; bottom = top+dimension
	
	mov eax,temp_rect.left
	mov temp_rect.right,eax
	add temp_rect.right,DRAW_AREA_DIMENSIONS ; right = left+dimension
	
	popa
	ret
get_rect_by_index ENDP

draw_vector_image PROC,value:DWORD,index:DWORD ; draws the pixel at the index, it's grayscale value is the argument "value"
	; turn from 0->1 grayscale float to 0->255 grayscale int
	fld value
	fld4 255.0
	fmulp st(1),st ; grayscale value * 255
	
	push DWORD PTR 0
	fistp DWORD PTR [esp] ; cast to int between range 0 -> 255
	
	pop eax ; al -> grayscale int
	mov ah,al
	shl eax,8 ; eax -> 0000, grayscale, grayscale, grayscale = RGB(grayscale,grayscale,grayscale)
	mov al,ah

	;invoke get_color_by_index,index

	invoke CreateSolidBrush,eax
	push eax
	
	invoke get_rect_by_index,index ; temp_rect = pixel rect by index
	mov eax,[esp] ; eax = brush
	
	invoke FillRect,hdc,offset temp_rect,eax ; draw the square with the brush
	
	pop eax
	invoke DeleteObject,eax ; delete the brush
	
	the_end:
	ret
draw_vector_image ENDP

WinMain proc ; main window function
	LOCAL msg:MSG

	new_list ; initialize vector image
	mov image_vector,eax
	invoke Alloc,28*28*4
	mov ebx,image_vector
	mov [ebx],eax
	mov [ebx+4],DWORD PTR 784

	mov m_list.count,10 ; initialize list for finding maximum probability

	;;;; WIN32 API REQUIREMENTS TO OPEN A WINDOW ;;; 
			;;; AKA THE BOARING PART ;;;
	
	mov ebx, offset wcx
	wc equ [ebx.WNDCLASSEX]
	
	mov wc.hInstance, rv(GetModuleHandle, 0) ; get the module handle to hInstance
	mov wc.hIcon, rv(LoadIcon, NULL, IDI_APPLICATION) ; default icon
	mov wc.hIconSm, eax ; same icon
	invoke RegisterClassEx, addr wc ; the window class needs to be registered
	invoke CreateWindowEx, NULL, wc.lpszClassName, offset window_name, ; create the window
    WS_OVERLAPPED or WS_CAPTION or WS_SYSMENU or WS_MINIMIZEBOX or WS_MAXIMIZEBOX or WS_VISIBLE,
    100, 100, 700, 500,        ; x, y, w, h
    NULL, NULL, wc.hInstance, NULL

	loop_label: ; main loop
        invoke GetMessage, addr msg, NULL, 0, 0
		test eax,eax ; if exit code
		jz exit_window
        invoke TranslateMessage, addr msg
        invoke DispatchMessage, addr msg
	jmp loop_label
	exit_window:
	exit msg.wParam ; using masm's exit macro
	ret
WinMain endp

list_to_vector MACRO list ; this macro turns a list into a row vector R(n) , used to turn the image_vector list to a vector
	mov eax,list
	invoke zero_matrix,1,[eax+4] ; matrix to size R(1,n)
	mov ebx,list
	mov ebx,[ebx] ; items
	mov ecx,[eax] ; pointer to rows
	mov [ecx],ebx ; first row = items
	EXITM < eax >
ENDM

hypot_get_max PROC,l:DWORD ; gets the index of the largest float in the list l to global_max_ind
	push ebx
	push ecx
	
	mov global_max_ind,0 ; initialize
	mov global_max,0
	
	mov ebx,l
	mov ebx,[ebx] ; items
	mov ecx,10 ; 10 times
	the_loop:
		push DWORD PTR [ebx]
		fld global_max
		fld DWORD PTR [esp]
		fcomi st,st(1) 
		jc aset_global_max ; if global_max > list[ebx]
			mov global_max_ind,10 ; correct index = 10-ecx
			sub global_max_ind,ecx
			fst global_max ; store maximum in global_max
		aset_global_max:
		fstp st
		fstp st ; pop twice from fpu
		add esp,4 ; pop from stack
		add ebx,4 ; next item
	loop the_loop ; repeat 10 times to find the highest probability
	
	pop ecx
	pop ebx
	ret	
hypot_get_max ENDP

render PROC,whnd:DWORD ; render the drawing area
	invoke GetDC,whnd ; get the graphics handle
	mov hdc,eax
	
	;invoke ResetDC,hdc,NULL
	xor ecx,ecx ; for ecx = 0 to MNIST_IMAGE_SIZE
	the_loop:
		push ecx
		invoke list_get_item,image_vector,ecx ; get the value at ecx
		invoke draw_vector_image,eax,ecx ; draw pixel at ecx
		pop ecx
		inc ecx
	cmp ecx,MNIST_IMAGE_SIZE
	jnz the_loop
	
	invoke ReleaseDC,whnd,hdc ; release graphics handle
	ret
render ENDP

WndProc proc hwind:HANDLE, uMsg:DWORD, wParam:WPARAM, lParam:LPARAM ; the window function that handles events
	local ps:PAINTSTRUCT ; used for painting events


	cmp uMsg,WM_CREATE ; if the window is created
	jnz after_0
		;;; create all components
		mov button_clear,mk_button("Clear",500,20,100,100,hwind,BUTTON_CLEAR_ID)
		
		mov button_predict,mk_button("Predict",500,130,100,100,hwind,BUTTON_PREDICT_ID)
		
		mov predict_label,rv(CreateWindowEx,0,reparg("EDIT"),NULL,WS_VISIBLE or WS_CHILD or  WS_BORDER, 530 , 250, 20, 20, hwind, 119, wcx.hInstance,NULL)
		
		invoke EnableWindow,predict_label,FALSE ; disable predict label
		
		
		invoke CreateWindowEx, 0, NULL, NULL,WS_CHILD or WS_VISIBLE or WS_BORDER, DRAW_AREA_LEFT, DRAW_AREA_TOP, DRAW_AREA_RIGHT, DRAW_AREA_BOTTOM, hwind, 125, wcx.hInstance, NULL
		mov paint_component,eax
		
		jmp the_end
	after_0:

	cmp uMsg,WM_DESTROY ; if window is being closed
	jnz after_1
		invoke PostQuitMessage, NULL ; quit after WM_CLOSE
		jmp the_end
	after_1:

	cmp uMsg,WM_LBUTTONDOWN
	jnz after_2
		mov is_mouse_down,TRUE ; set mousedown to true on press
		jmp the_end
	after_2:

	cmp uMsg,WM_LBUTTONUP
	jnz after_3
		mov is_mouse_down,FALSE ; set mousedown to false on release
		jmp the_end
	after_3:

	cmp uMsg,WM_MOUSEMOVE
	jnz after_4
		cmp is_mouse_down,0
		jz the_end ; skip if mouse isn't pressed
		
			mov ecx,lParam
			shr ecx,16 ; ecx = y coordinate = top 16 bits
			
			mov ebx,lParam 
			and ebx,00000ffffh ; ebx = x coordinate = bottom 16 bits
			
			add ebx,DRAW_AREA_DIMENSIONS/2
			add ecx,DRAW_AREA_DIMENSIONS/2 ; center both x and y coordinate
			
			invoke draw_to_vector,ebx,ecx ; draw by coordinate
			invoke render,hwind ; draw changes
		jmp the_end
	after_4:

	CMP uMsg,WM_PAINT ; paint drawing area in black
	jnz after_5
		invoke BeginPaint,hwind,addr ps
		mov hdc,eax
		invoke FillRect,hdc,addr paint_area,2 ; black, not sure why but it comes out black so...
		;; btw zero comes out white and 1 comes out gray, 3 will crush your computer.
		invoke EndPaint,hwind, addr ps
		jmp the_end
	after_5:

	cmp uMsg,WM_COMMAND ; if a component is pressed
	jnz after_6
		cmp wParam,BUTTON_CLEAR_ID ; if clear button is pressed
		jnz afterClear
		
			mov ebx,image_vector
			invoke Free,[ebx] ; delete items of the image vector
			invoke Alloc,28*28*4 ; allocate memory which is set to all 0
			mov ebx,image_vector
			mov [ebx],eax ; move the pointer at image_vector.items to the new memory allocated
			invoke render,hwind ; redraw the paint area (to all black)
			jmp the_end
		afterClear:
		
		cmp wParam,BUTTON_PREDICT_ID ; if predict button is pressed
		jnz afterPredict
			invoke hypot,list_to_vector(image_vector) ; get the neural network's output for the image vector as input
			
			mov edx,eax ; edx -> network output as vector
			mov ebx,eax
			mov ebx,[ebx] ; ebx -> pointer to rows
			mov ebx,[ebx] ; ebx -> first row (only row)
			mov m_list.items,ebx
			
			invoke hypot_get_max,offset m_list ; get maximum probability index (predicted digit) to global_max_ind
			invoke matrix_delete,edx ; delete output matrix
			
			mov eax,global_max_ind 
			add al,"0" ; digit to ascii
			mov number_string[0],al ; number_string = (string)global_max_ind
			
			invoke SendMessage,predict_label,WM_SETTEXT,0,offset number_string ; change the text to the predicted output
		afterPredict:
	after_6:
	
	invoke DefWindowProc, hwind, uMsg, wParam, lParam ; default processing
	
	the_end:
	ret
WndProc endp