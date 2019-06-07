include \masm32\include\masm32rt.inc


.data

Node STRUCT
	value DWORD ? ; value of the node
	next DWORD ? ; pointer to the next node
Node ENDS

Queue STRUCT
	head DWORD ? ; first out (Node*)
	tail DWORD ? ; last in (Node*)
	count DWORD ? ; guess (unsigned int)
Queue ENDS

Stack STRUCT
	pointer DWORD ? ; Node*
	count DWORD ? ; unsigned int
Stack ENDS

List STRUCT
	items DWORD ? ; pointer to array
	count DWORD ? ; unsigned int
List ENDS

;;;; NODE ;;;;

new_node PROTO value:DWORD,next:DWORD

delete_node PROTO node:DWORD

;;;; QUEUE ;;;; 

queue_push PROTO queue:DWORD, value:DWORD

queue_pop PROTO queue:DWORD

delete_queue PROTO queue:DWORD

;;;; STACK ;;;;

stack_push PROTO queue:DWORD, value:DWORD

stack_pop PROTO queue:DWORD

delete_stack PROTO queue:DWORD

;;;; BOTH STACK AND QUEUE

peek PROTO object:DWORD

;;;; BIN TREE ;;;;


;;;; lIST ;;;; 

list_insert PROTO list:DWORD,value:DWORD

list_set PROTO list:DWORD, index:DWORD, value:DWORD

list_index_of PROTO list:DWORD, item:DWORD

list_delete_at PROTO list:DWORD, index:DWORD

list_get_item PROTO list:DWORD, index:DWORD

delete_list PROTO list:DWORD

.code

;;;; NODE ;;;;

new_node PROC ,value:DWORD, next:DWORD ; creates a node and returns a pointer
	invoke Alloc,SIZEOF Node ; allocate node

	push value
	pop DWORD PTR [eax] ; initialize value

	push next
	pop DWORD PTR [eax+4] ; initialize next

	ret
new_node ENDP

delete_node PROC, node:DWORD ; deletes the node and all it's children
	push ebx
	push edx
	push ecx
	push eax
	
	push node ; push the node to go to ebx at first
	delete_child_loop:
		pop ebx ; get the current node pointer
		push DWORD PTR [ebx+4] ; push the next pointer
		invoke Free,ebx ; delete the current node
	cmp DWORD PTR [esp],NULL ; stop if the next node is a nullptr
	jne delete_child_loop

	pop eax
	pop ecx
	pop edx
	pop ebx
	ret
delete_node ENDP

;;;; QUEUE ;;;;

queue_push PROC, queue:DWORD, value:DWORD ; add an element to the end of the queue
	pusha
	
	invoke new_node,value,NULL ; eax = new Node(value = value,next = NULL)

	mov ebx,queue
	inc DWORD PTR [ebx+8] ; count++
	cmp DWORD PTR [ebx+8],1 ; queue is empty
	je HandleNull

	mov edx,[ebx+4] ; edx is a pointer to tail
	mov [edx+4],eax ; tail.next = new node

	jmp final

	HandleNull: ; if head == null
	mov [ebx],eax ; queue.head is the new node

	final:
	mov [ebx+4],eax ; queue.tail is the new node
	popa
	ret
queue_push ENDP

queue_pop PROC,queue:DWORD ; retrives an element from the beginning of the queue
	mov eax,queue
	
	cmp DWORD PTR [eax+8],0 ; if count == 0 return null
	jne notNull
		mov eax,NULL
		ret
	notNull:
	
	dec DWORD PTR [eax+8] ; count--
	mov eax,[eax] ; pointer to head
	mov eax,[eax] ; head.value
	pusha

	mov ebx,queue
	mov edx,[ebx] ; edx = queue.head
	mov edx,[edx+4] ; head.next

	push edx
	invoke Free,[ebx] ; delete node
	pop edx

	mov [ebx],edx ; queue.head = queue.head.next

	popa
	finish:
	ret
queue_pop ENDP

delete_queue PROC, queue:DWORD
	pusha
	
	mov ebx,queue
	mov ecx,[ebx+8] ; queue.count
	push DWORD PTR[ebx]
	delete_nodes_loop:
		pop ebx
		push DWORD PTR [ebx+4] ; ebx = ebx.next
		invoke Free,ebx ; delete node
	loop delete_nodes_loop
	invoke Free,queue
	
	popa
	ret
delete_queue ENDP

;;;; STACK ;;;;

stack_push PROC, stack:DWORD, value:DWORD
	pusha
	mov ebx,stack
	invoke new_node,value,[ebx] ; eax = new Node(value = value,next = stack.pointer)
	mov ebx,stack
	inc DWORD PTR [ebx+4] ; count++
	mov [ebx],eax ; pointer = new Node(value,old pointer)

	final:
	popa
	ret
stack_push ENDP

stack_pop PROC,stack:DWORD
	mov eax,stack
	cmp DWORD PTR [eax+4],0 ; if count is 0
	jne notNull
		mov eax,NULL
		ret
	notNull:
	dec DWORD PTR [eax+4]
	mov eax,[eax] ; pointer to stack pointer
	mov eax,[eax] ; pointer.value
	pusha

	mov ebx,stack
	mov edx,[ebx] ; edx = stack.pointer
	mov edx,[edx+4] ; stack.next

	push edx ; push stack.pointer.next
	push ebx
	invoke Free,[ebx] ; delete node
	pop ebx
	pop DWORD PTR [ebx] ; stack.pointer = stack.pointer.next

	popa
	finish:
	ret
stack_pop ENDP

delete_stack PROC, stack:DWORD
	pusha
	
	mov ebx,stack
	mov ecx,[ebx+4] ; stack.count
	push DWORD PTR [ebx]
	delete_nodes_loop:
		pop ebx
		push DWORD PTR [ebx+4] ; ebx = ebx.next
		invoke Free,ebx ; delete node
	loop delete_nodes_loop
	invoke Free,stack
	
	popa
	ret
delete_stack ENDP

;;;; BOTH ;;;;

peek PROC,object:DWORD ; works on both Queue and Stack
	mov eax,object
	mov eax,[eax] ; pointer to head
	cmp eax,NULL
	je finish
	mov eax,[eax] ; head.value
	finish:
	ret
peek ENDP

;;;; LIST ;;;;

list_insert proc, list:DWORD,value:DWORD ; adds the value to the end of the list
	pusha
	
	mov ebx,list
	inc DWORD PTR [ebx+4] ; count++
	
	mov ecx,[ebx+4]
	shl ecx,2 ; count * sizeof dword
	invoke Alloc, ecx ; allocate new memory
	
	mov edi,eax ; destination = new array
	mov ebx,list
	mov esi,[ebx] ; source = old array
	mov ecx,[ebx+4] 
	dec ecx ; ecx = count-1
	
	push es
	push ds ; data to extra for movsd
	pop es ; flashbacks from codeguru :)
	rep movsd ; copy from old array to new
	pop es ; restore old extra segment
	
	push value
	pop DWORD PTR [edi] ; last element = value
	invoke Free, [ebx] ; delete old array
	
	mov ebx,list
	mov ecx,[ebx+4]
	dec ecx
	shl ecx,2 ; ecx *= 4
	sub edi,ecx ; beginning of the new array
	mov [ebx],edi
	
	popa
	ret
list_insert ENDP

list_get_item PROC,list:DWORD,index:DWORD ; return the item at the index
	push ebx
	
	mov ebx,list
	mov ebx,[ebx] ; items array pointer
	mov eax,index
	shl eax,2
	mov eax,[ebx+eax] ; linear address of items array[index]
	
	pop ebx
	ret
list_get_item ENDP

list_set PROC,list:DWORD,index:DWORD,value:DWORD ; sets the item at the index to the value
	push eax
	push ebx
	
	mov ebx,list
	mov ebx,[ebx] ; pointer to items array
	mov eax,index
	shl eax,2
	push value
	pop DWORD PTR [ebx+eax] ; linear address of items[index]

	pop ebx
	pop eax
	ret
list_set ENDP

list_index_of PROC, list:DWORD , item:DWORD ; returns the index of the first appearance of the item,-1 if item isn't found
	push ecx
	push ebx

	mov ebx,list
	mov ecx,[ebx+4] ; ecx -> count
	
	mov eax,item
	mov edi,[ebx] ; edi -> items
	
	inc ecx
	sub edi,4 ; adding later
	jmp check_loop ; first check then do
	
	loop_label:
		add edi,4
		cmp [edi],eax 
	check_loop:loopnz loop_label ; loop if list[i]!=item, break if item found
	
	jnz notFound 
	; if loop terminated in the middle, ZF will be marked, if loop was terminated because ecx = 0, ZF will not be marked
	
	sub edi,[ebx] ; edi = &item - &first_item = the distance in bytes between the item and the first item
	mov eax,edi
	shr eax,2 ; edi /= 4n = distance above in dwords = index
	jmp final
	
	notFound:
		mov eax,-1 ; case item wasn't found
		
	final:
	pop ebx
	pop ecx
	ret
list_index_of endp

list_delete_at PROC, list:DWORD, index:DWORD ; removes the item at the index from the list and shortens the list by 1 element
	pusha
	
	mov ebx,list
	push DWORD PTR [ebx+4] ; count
	
	mov ecx,[esp]
	dec ecx
	shl ecx,2
	invoke Alloc,ecx ; new array to size count-1
	
	mov edi,eax ; edi = new array
	mov ebx,list
	mov esi,[ebx] ; esi = old array
	
	mov ecx,index
	push es
	push ds ; data to extra (CodeGuru!!!)
	pop es
	rep movsd ; copy from old array to new array right until the index of the item to delete
	
	mov ecx,index
	inc ecx
	sub [esp+4],ecx ; count - index - 1
	mov ecx,[esp+4] ; ecx = items left to copy
	add esi,4 ; skip over the item deleted
	rep movsd ; continue to copy from old array to new
	
	pop es
	pop ecx ; list.count - index - 1
	add ecx,index ; list.count -1
	shl ecx,2 ; byte size of new array
	sub edi,ecx ; point to the beginning of the array
	
	invoke Free,[ebx] ; delete old array
	mov ebx,list
	mov [ebx],edi ; set items as new array
	dec DWORD PTR [ebx+4] ; count--
	
	popa
	ret
list_delete_at ENDP

delete_list PROC,list:DWORD ; deletes the list
	push eax
	
	mov eax,list
	invoke Free,[eax] ; delete the item array
	invoke Free,list
	
	pop eax
	ret
delete_list endp

;;;; MACROS ;;;;

new_stack MACRO
	invoke Alloc,SIZEOF Stack
ENDM

new_queue MACRO
	invoke Alloc,NULL,SIZEOF Queue
ENDM

stack_peek MACRO stack
	push stack
	call peek
ENDM

queue_peek MACRO queue
	push queue
	call peek
ENDM

new_list MACRO
	invoke Alloc,SIZEOF List
ENDM
