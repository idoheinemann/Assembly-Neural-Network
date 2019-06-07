include data_structs.inc

.const

DRAW_AREA_TOP equ 20
DRAW_AREA_LEFT equ 20
DRAW_AREA_BOTTOM equ 300
DRAW_AREA_RIGHT equ 300
DRAW_AREA_DIMENSIONS equ (DRAW_AREA_BOTTOM-DRAW_AREA_TOP)/28

.data

;;;; MAIN WINDOW ;;;;

txClass BYTE "window class", 0 ; class name, will be registered below
window_name BYTE "MNIST hand writing recognizer",0 ; title of the window

wcx WNDCLASSEX <WNDCLASSEX, CS_HREDRAW or CS_VREDRAW, WndProc, 0, 0, 1, 2, 3, COLOR_BTNFACE+1, 0, txClass, 4> ; the window class object
; basically something win32 api requires you to do in order to open a window


;;;; COMPONENTS ;;;;

button_class BYTE "button",0 ; name of the button class

button_clear HANDLE ? ; the handle and id of the clear button
BUTTON_CLEAR_ID equ 101

button_predict HANDLE ? ; the handle and id of the predict button
BUTTON_PREDICT_ID equ 102

paint_component HANDLE ? ; the handle to the component representing the drawing area

predict_label HANDLE ? ; the handle to the prediction label

is_mouse_down BYTE ? ; boolean

paint_area RECT<DRAW_AREA_LEFT,DRAW_AREA_TOP,DRAW_AREA_RIGHT,DRAW_AREA_BOTTOM> ; rectangle representing the drawing area

temp_rect RECT<> ; for function to return rectangle values through

hdc HDC ? ; graphics handle

image_vector DWORD ? ; list of all 28*28 pixels as floats between 0 (black) and 1 (white)

m_list List<> ; list used to find the maximum probability, meaning the predicted digit

number_string BYTE 0,0 ; the predicted digit as a string

f_28 REAL4 28.0
f_255 REAL4 255.0