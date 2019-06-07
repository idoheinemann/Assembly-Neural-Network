include data_structs.inc

.data

bubble_sort PROTO list:DWORD ; sorts the list from smallest item to largest using bubble sort
insert_sorted PROTO list:DWORD,value:DWORD ; inserts the new value to the list while keeping it sorted from smallest to largest
list_map PROTO list:DWORD,func:DWORD ; returns a new list, function must be cdecl and can accept item,index
list_filter PROTO list:DWORD,func:DWORD ; returns a new list, function must be cdecl and can accept item,index
list_concat PROTO l1:DWORD,l2:DWORD ; returns a new list

.code

bubble_sort PROC,list:DWORD
	pusha
	mov ebx,list
	mov ecx,[ebx+4] ; ecx = count
	jmp check_loop_first
	bubble_loop:
		push ecx
		mov edx,ecx
		dec edx
		invoke list_get_item,list,ecx
		xchg ebx,eax;ebx = list[edx+1]
		invoke list_get_item,list,edx ; list[edx]
		cmp eax,ebx ; if list[edx]>list[edx+1]
		jle afterSort
		sort_loop:
			invoke list_set,list,ecx,eax ; list[edx+1] = list[edx]  ; BTW there is a temp variable
			invoke list_set,list,edx,ebx ; list[edx] = list[edx+1]
			; replace order of items
			mov edx,ecx
			inc ecx
			mov ebx,list
			cmp ecx,[ebx+4]
			je afterSort
			invoke list_get_item,list,ecx
			xchg ebx,eax;ebx = list[edx+1]
			invoke list_get_item,list,edx ; list[edx]
			cmp eax,ebx ; if list[edx]>list[edx+1]
			jg sort_loop
		afterSort:

		pop ecx
	check_loop_first:
	loop bubble_loop
	popa
	ret
bubble_sort ENDP

insert_sorted PROC,list:DWORD,value:DWORD
	pusha
	mov ecx,list
	mov ecx,[ecx+4]
	inc ecx
	shl ecx,2
	invoke Alloc,ecx
	mov ebx,eax
	mov edx,list

	push [edx+4]
	inc DWORD PTR [edx+4]
	
	check_insert_loop_1:
		cmp ecx,[esp]
		je after_first_insert
		invoke list_get_item,list,ecx
		cmp eax,value
		jg after_first_insert
		shl ecx,2
		mov [ebx+ecx],eax
		shr ecx,2
		inc ecx
	jmp check_insert_loop_1

	after_first_insert:
		shl ecx,2
		mov eax,value
		mov [ebx+ecx],eax
		shr ecx,2
	insert_loop_2:
		cmp ecx,[esp]
		je finish
		invoke list_get_item,list,ecx
		shl ecx,2
		mov [ebx+ecx+4],eax
		shr ecx,2
		inc ecx
	jmp insert_loop_2
	finish:
	mov ss:[esp], ebx
	invoke Free,[edx]
	mov edx,list
	pop DWORD PTR [edx]

	
	popa
	ret
insert_sorted ENDP

list_map PROC,list:DWORD,func:DWORD
	push ebx
	push ecx
	push edx

	new_list
	push eax
	mov ecx,list
	mov ecx,[ecx+4]
	shl ecx,2
	invoke Alloc,ecx
	mov ebx,[esp]
	mov [ebx],eax
	mov ecx,list
	mov ecx,[ecx+4]
	mov [ebx+4],ecx
	inc ecx
	jmp check_loop
	for_each_loop:
		invoke list_get_item,list,ecx
		push ebx
		push ecx
		push ecx ; both protect and argument
		dec DWORD PTR [esp]
		push eax ; argument for func
		call func
		add esp,8 ; clear stack
		pop ecx
		pop ebx
		invoke list_set,ebx,ecx,eax
	check_loop:
	loop for_each_loop

	pop eax
	pop edx
	pop ecx
	pop ebx

	ret
list_map ENDP

list_filter PROC,list:DWORD,func:DWORD
	push ebx
	push ecx
	push edx

	new_list
	mov ebx, eax
	mov ecx,list
	mov ecx,[ecx+4]
	inc ecx
	jmp check_loop_1
	for_each_loop:
		dec ecx
		invoke list_get_item,list,ecx
		inc ecx
		push eax
		push ebx
		push ecx ; both protect and argument
		push eax ; argument for func
		call func
		pop ecx ; clear stack from arguments
		pop ecx
		pop ebx
		test eax,eax ; if function returned false
		jz check_loop
		mov eax,[esp]
		invoke list_insert,ebx,eax
	check_loop:
	pop eax
	check_loop_1:
	loop for_each_loop

	mov eax,ebx
	pop edx
	pop ecx
	pop ebx

	ret
list_filter ENDP

list_concat PROC l1:DWORD,l2:DWORD
	push ebx
	push ecx
	push esi
	push edi

	new_list
	push eax
	mov ebx,l1
	mov ecx,[ebx+4]
	mov ebx,l2
	add ecx,[ebx+4]
	shl ecx,2
	push ecx

	invoke Alloc,ecx
	mov edi,eax
	mov esi,l1
	push es
	push ds
	pop es

	mov ecx,[esi+4]
	mov esi,[esi]
	rep movsd

	mov esi, l2
	mov ecx,[esi+4]
	mov esi,[esi]
	rep movsd

	pop es

	pop ecx
	sub edi,ecx
	pop eax
	mov [eax],edi
	shr ecx,2
	mov [eax+4],ecx

	pop edi
	pop esi
	pop ecx
	pop ebx
	ret
list_concat ENDP

