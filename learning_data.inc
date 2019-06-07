include \masm32\include\masm32rt.inc

.data

MNIST_IMAGE_SIZE equ 28*28 ; size of an image in bytes
BATCH_SIZE equ 100 ; size of a train batch

;; the handles to the layer load files
layer0_file_handle HANDLE ?
layer1_file_handle HANDLE ?
layer2_file_handle HANDLE ?
layer3_file_handle HANDLE ?

;; the names of the layer load files
layer0_file_name BYTE "layer0.nnl",0
layer1_file_name BYTE "layer1.nnl",0
layer2_file_name BYTE "layer2.nnl",0
layer3_file_name BYTE "layer3.nnl",0

; the name of the train data files
images_file_name BYTE "train-images.idx3-ubyte",0
labels_file_name BYTE "train-labels.idx1-ubyte",0

layer0 DWORD ? ; input, (28*28+1)x256
layer1 DWORD ? ; hidden, 256x128
layer2 DWORD ? ; hidden, 128x128
layer3 DWORD ? ; output, 128x10

images_matrix DWORD ? ; pointer to the current input matrix
labels_matrix DWORD ? ; pointer to the current output labels

mnist_image_file_handler HANDLE ? ; handle to the image data byte file
mnist_label_file_handler HANDLE ? ; handle to the label byte file

test_image_file_handler HANDLE ? ; handle to the test image data byte file
test_label_file_handler HANDLE ? ; handle to the test label byte file

mnist_images_buffer byte BATCH_SIZE*MNIST_IMAGE_SIZE dup(?)  ; a buffer to which the image information is loaded to from the file
mnist_labels_buffer byte BATCH_SIZE dup(?) ; a buffer to which the label information is loaded to from the file

buffer byte 100 dup(?) ; a buffer for various uses

normalization_factor REAL4 0.00392156862745098 ; 1/255, used to normalize the image matrix from range 0-255 to range 0-1
alpha REAL4 0.002 ; learning rate

; float constants
float_1 REAL4 1.0
float_07 REAL4 0.7
float_05 REAL4 0.5
float_02 REAL4 0.2