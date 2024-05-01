import serial
import io
from PIL import Image
import os
import time 

# Folder to save images
folder_path = 'test_2304'

# Create the folder if it doesn't exist
if not os.path.exists(folder_path):
    os.makedirs(folder_path)
# Adjust the COM port and baud rate based on your ESP32 configuration  COM5 esps2
ser = serial.Serial('COM5', baudrate=115200)

# Create a BytesIO object to store the received image data
image_data = bytearray()


# Flag to indicate if the start marker has been found
image_start_found = False

image_counter = 1
while True:
        data = ser.read(8)
        #print(data)
        
        # elif b'IMG' in data:
        #     data = ser.read(64)
        #     print(data)
             
        if not data:
            break

        if b'TXT' in data:
            data = ser.read(74)
            print(data)
            #continue
        #print(data)
        # 
        # Search for the JPEG start marker (0xFFD8)
        if b'\xFF\xD8' in data:
            image_start_found = True
            # Discard any data before the start marker
            data = data[data.index(b'\xFF\xD8'):]
        
        if image_start_found:
            # Append data to the BytesIO object
            image_data.extend(data)
            #image_data.seek(0)

            # Search for the JPEG end marker (0xFFD9)
            if b'\xFF\xD9' in image_data:
                # Save the received image data to a local file
                filename = f'{folder_path}/picture6_{image_counter}.jpg'
                with open(filename, 'wb') as file:
                # print(image_data)
                    file.write(image_data)
                print(f"Image {image_counter} received and saved as {filename}.")

                    # Increment the counter for the next image
                image_counter += 1
                
                # Reset for the next image
                image_start_found = False
                image_data.clear()  
        #time.sleep(3)
        # if not isinstance(data, bytes):
        #     text_data = ser.readline().decode('utf-8').strip()
        #     print(text_data)
            # image_data.seek(0)
            # image_data.truncate()

# # Read data from the serial port
# while True:
#     data = ser.read(1024)

#     # Break the loop if no more data is available
#     if not data:
#         break

#     # Write data to the BytesIO object
#     image_data.extend(data)

#     # Check for the end of the JPEG image data
#     if b'\xFF\xD9' in data:
#         # Save the received image data to a BytesIO object
#         image_stream = io.BytesIO(image_data)

#         # Create an Image object from the received image data
#         try:
#             img = Image.open(image_stream)
#             img.show()
#             img.save('received_image.jpg')
#             print("Image received and saved.")
#         except Exception as e:
#             print(f"Error opening/saving the image: {e}")

#         # Clear the BytesIO object for the next image
#         image_data.clear()

# # Close the serial port
# ser.close()

