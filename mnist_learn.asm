include printing.inc

.code 
main proc
	invoke initialize_values
	invoke WinMain
	;invoke learn
	invoke display_test_data

	ret
main endp
end main