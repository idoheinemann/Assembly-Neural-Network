include string_functions.inc

.data

regex_match PROTO string:DWORD, patt:DWORD

.code

regex_match PROC, string:DWORD, patt:DWORD
	invoke str_length,string
	push eax
	invoke str_length,patt
	push eax
	new_stack
	push eax
	push DWORD PTR 0 ; string pointer
	push DWORD PTR 0 ; patt pointer





	ret

regex_match ENDP