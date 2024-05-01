import serial
import time
import os

ser = serial.Serial('COM7', 115200) 
# len_str = "hello world long \n\r"
# print(len(len_str))
# ser.write(b"hello world \n\r")
# counter = 10
# while (counter !=0) :
#     image_file_path = 'test_image/image1_15.jpg'
#     with open(image_file_path, 'rb') as f:
#         image_data = f.read()
#         length = len(image_data)
#         #print(image_data)
#         ser.write(image_data)
#     ####time.sleep(5)
#     print("send count ",counter)
#     counter -=1

folder_path = 'test_image'

files = os.listdir(folder_path)
counter = 2
while (counter !=0) :
    image_files = [f for f in files if f.lower().endswith(('.png', '.jpg'))]
    for image_file in image_files:
        image_path = os.path.join(folder_path, image_file)
        with open(image_path, 'rb') as f:
            image_data = f.read()
            length = len(image_data)
            print(length)
            ser.write(image_data)
            time.sleep(5)
        time.sleep(20)
    print("send count ",counter)
    counter -=1
        






