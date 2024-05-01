#include <Arduino.h>
#include <SPI.h>
#include "FS.h"
#include "SPIFFS.h"

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}
// Initialize variables
bool image_start_found = false;
uint8_t image_counter = 0;
uint8_t* image_data = nullptr;
size_t image_data_size = 0;


void setup() {
  Serial.begin(115200);
  
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    while (1) {}  // Stop program execution if SPIFFS initialization fails
  }
  
  // const char* folder_path = "/received_images"; // Folder path to save images
  // // Create folder if it doesn't exist
  // if (!SPIFFS.exists(folder_path)) {
  //   SPIFFS.mkdir(folder_path);
  // }
  listDir(SPIFFS, "/", 0);

  // Replace "received_image.jpg" with the desired file name
  // File file = SPIFFS.open("/text_file.txt", "w");
  // if (!file) {
  //   Serial.println("Failed to open file for writing");
  //   while (1) {}  // Stop program execution if file opening fails
  // }
}

uint8_t temp = 0, temp_last = 0;
byte buf[256];
static int i = 0;
bool is_header = false;
uint8_t* jpeg_buffer;

void writeFile(const char* path, uint8_t* data, size_t length) {
  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Error opening file for writing");
    return;
  }

  if (file.write(data, length) != length) {
    Serial.println("Error writing to file");
  } else {
    Serial.println("File written successfully");
  }

  file.close();
}

void printFileContents(const char* path) {
  File file = SPIFFS.open(path, FILE_READ);
  if (!file) {
    Serial.println("Error opening file for reading");
    return;
  }

  Serial.println("File contents:");
  while (file.available()) {
    Serial.write(file.read());
  }
  Serial.println("End of file contents");

  file.close();
}



void saveImage(uint8_t* data, size_t size) {
  // Save the received image data to a local file
  char filename[30];
  snprintf(filename, sizeof(filename), "/image_%d.jpg", image_counter++);
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Error opening file for writing");
    return;
  }

  if (file.write(data, size) != size) {
    Serial.println("Error writing to file");
  } else {
    Serial.print("Image saved as ");
    Serial.println(filename);
  }

  file.close();
}

void loop() {
  // Read from serial port
  while (Serial.available()) {
    uint8_t byteRead = Serial.read();
    
    if (image_start_found) {
      // Append data to image_data buffer
      uint8_t* new_image_data = (uint8_t*)realloc(image_data, image_data_size + 1);
      if (new_image_data) {
        image_data = new_image_data;
        image_data[image_data_size++] = byteRead;
      } else {
        Serial.println("Memory allocation failed!");
        return;
      }

      // Search for the end of image marker (0xFFD9)
      if (byteRead == 0xD9 && image_data_size >= 2 && image_data[image_data_size - 2] == 0xFF) {
        // End of image received, save the image
        saveImage(image_data, image_data_size);
        
        // Reset variables for next image
        image_start_found = false;
        image_data_size = 0;
        free(image_data);
        image_data = nullptr;
      }
    } else {
      // Search for the start of image marker (0xFFD8)
      if (byteRead == 0xD8 && Serial.peek() == 0xFF) {
        // Start of image found
        image_start_found = true;
        // Allocate memory for image_data buffer
        image_data = (uint8_t*)malloc(2);
        if (!image_data) {
          Serial.println("Memory allocation failed!");
          return;
        }
        image_data[0] = 0xFF;
        image_data[1] = 0xD8;
        image_data_size = 2;
      }
    }
  }
}


// incomplete
// void loop() {
//   listDir(SPIFFS, "/", 0);
//   static uint8_t* buffer = nullptr; // Pointer to hold received data
//   static size_t bytesRead = 0;

//   // Read from serial port
//   while (Serial.available()) {
//     uint8_t byteRead = Serial.read();
//     buffer = (uint8_t*)realloc(buffer, bytesRead + 1); // Dynamically allocate memory
//     if (buffer) {
//       buffer[bytesRead++] = byteRead;
//     } else {
//       Serial.println("Memory allocation failed!");
//       return;
//     }
//   }

//   // Check for end of image data
//   static const uint8_t endMarker[] = {0xFF, 0xD9}; // End of JPEG marker
//   static int endMarkerIndex = 0;
//   for (size_t i = 0; i < bytesRead; i++) {
//     if (buffer[i] == endMarker[endMarkerIndex]) {
//       endMarkerIndex++;
//       if (endMarkerIndex >= sizeof(endMarker)) {
//         // End of image received, process the image
//         Serial.println("End of image received");
//         Serial.print("Length of buffer: ");
//         Serial.println(bytesRead);
//         // Save image to SPIFFS
//         writeFile("/received_image.jpg", buffer, bytesRead);
//         // ProcessImage(buffer, bytesRead); // Call a function to process the image
//         free(buffer); // Free dynamically allocated memory
//         buffer = nullptr; // Reset pointer
//         bytesRead = 0; // Reset byte count
//         endMarkerIndex = 0; // Reset end marker index
//         break;
//       }
//     } else {
//       endMarkerIndex = 0;
//     }
//   }
//   printFileContents("/received_image.jpg");
//   //Serial.println(buffer);
// }



// working but not able to check the last bytes
// void loop(){
//   listDir(SPIFFS, "/", 0);
//   if (Serial.available() > 0) {
//     // Create or open the file for writing
//     File file = SPIFFS.open("/image_file.jpg", "w");  // "a" stands for append mode

//     if (!file) {
//       Serial.println("Error opening file for writing");
//       return;
//     }

//     // Read and write image data byte by byte
//     while (Serial.available() > 0) {
//       char receivedByte = Serial.read();

//       // Check if it's the start of a JPEG image (0xFF)
//       // if (receivedByte == 0xFF) {
//       //   // Check if the previous byte was also 0xFF, indicating the start of a new marker
//       //   if (file.size() > 0 && file.read() == 0xFF) {
//       //     // Rewind the file pointer by 1 byte to avoid skipping the marker
//       //     file.seek(file.position() - 1);
//       //   }
//       // }

//       // Write the byte to the file
//       file.write(receivedByte);

      
//       // // Check if it's the end of a JPEG image (0xD9)
//       // if (receivedByte == 0xD9) {
//       //   file.close();
//       //   Serial.println("Image received and saved as /received_image.jpg.");
//       //   // Optional: Add additional processing here if needed
//       // }
//     }

//     // Close the file
//     file.close();
//   }
//   // delay(100);
//   // Serial.println("read the file ");
//   // File dir = SPIFFS.open("/");
//   // File file = dir.openNextFile();
//   // while (file){
//   //   Serial.print("Rending file: ");
//   //   Serial.println(file.name());
//   //   Serial.println(file.read());
//   //   file.close();
//   //   file = dir.openNextFile();
//   // }
// }
  

    // Optional: Add a delay or perform additional processing as needed
    // delay(10);
  //  File dir = SPIFFS.open("/");
  //  File file = dir.openNextFile();
  //  jpeg_buffer = static_cast<uint8_t*>(heap_caps_malloc(20000 * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM));
    // if(file){
    //   int bytesRead = file.readBytes(reinterpret_cast<char*>(jpeg_buffer), file.size());
    //   file.close();
    //   Serial.printf("Read %d bytes from file\n", bytesRead);
    // } 

// void loop(){
//   listDir(SPIFFS, "/", 0);
//   if (Serial.available() > 0){
//     File file = SPIFFS.open("/image_new.jpg", "a");

//     while(Serial.available() > 0){
//       temp_last = temp;
//       temp = Serial.read();
//       Serial.println("read one byte");
//       if ( (temp == 0xD9) && (temp_last == 0xFF) ){
//         buf[i++] = temp;
//         file.write(buf, i);
//         //Close the file
//         file.close();
//         Serial.println("found end and close");
//         is_header = false;
//         i = 0;
//       }
//     if (is_header == true)
//     {
//       //Write image data to buffer if not full
//       if (i < 256)
//         buf[i++] = temp;
//       else
//       {
//         //resize_image(buf,320,240,buf,96,96,3);
//         //Write 256 bytes image data to file
//         file.write(buf, 256);
//         Serial.println("it goes on and on");
//         i = 0;
//         buf[i++] = temp;
//       }
//     }
//   else if ((temp == 0xD8) & (temp_last == 0xFF))
//     {
//       is_header = true;
//       buf[i++] = temp_last;
//       buf[i++] = temp;
//     }

//     }
//   }
//   delay(1000);
// }





// void loop(){
//     listDir(SPIFFS, "/", 0);
//     if (Serial.available() > 0){
//       File file = SPIFFS.open("/image.jpg", "w");
//       while (Serial.available() > 0) {
//         char receivedByte = Serial.read();
//         Serial.println(receivedByte);
//         // Check if it's the start of a JPEG image
//         if (receivedByte == 0xFF) {
//         // Check if the previous byte was also 0xFF, indicating the start of a new marker
//           if (file.size() > 0 && file.read() == 0xFF) {
//             // Rewind the file pointer by 1 byte to avoid skipping the marker
//             file.seek(file.position() - 1);
//           }
//         }
//         // Write the byte to the file
//         file.write(receivedByte);
//         // Check if it's the end of a JPEG image (0xD9)
//         if (receivedByte == 0xD9) {
//           file.close();
//           Serial.println("Image received and saved as /received_image.jpg.");
//         }
//       }
          
//       // Close the file
//       file.close();

//       // Optional: Add a delay or perform additional processing as needed
//       delay(10);
//     }
// }


// void loop(){
//   listDir(SPIFFS, "/", 0);
//   if (Serial.available() > 0) {
//     // Create or open the file for writing
//     File file = SPIFFS.open("/image.jpg", "a");  // "a" stands for append mode

//     if (!file) {
//       Serial.println("Error opening file for writing");
//       return;
//     }

//     // Read and write image data byte by byte
//     // while (Serial.available() > 0) {
//     //   char receivedByte = Serial.read();
//     //   //Serial.println(receivedByte);
//     //   file.write(receivedByte);
//     // }

//     // Read and write image data byte by byte
//     // while (Serial.available() > 0) {
//     //   char receivedByte = Serial.read();
//     //   // Check if it's the start of a JPEG image
//     //   if (receivedByte == 0xFF && file.size() == 0) {
//     //     file.write(receivedByte); // Write the first byte
//     //   } else if (receivedByte == 0xD9 && file.size() > 2) {
//     //     file.write(receivedByte); // Write the last byte
//     //     // Close the file
//     //     file.close();
//     //     Serial.println("Image received and saved as /received_image.jpg.");
//     //   } else {
//     //     file.write(receivedByte); // Write other bytes
//     //   }
//     // }
//     // // Close the file
//     // file.close();

  

//     // Optional: Add a delay or perform additional processing as needed
//    while (Serial.available() > 0) {
//       char receivedByte = Serial.read();
      
//       // Check if it's the start of a JPEG image
//       if (receivedByte == 0xFF) {
//         // Check if the previous byte was also 0xFF, indicating the start of a new marker
//         if (file.size() > 0 && file.read() == 0xFF) {
//           // Rewind the file pointer by 1 byte to avoid skipping the marker
//           file.seek(file.position() - 1);
//         }
//       }

//       // Write the byte to the file
//       file.write(receivedByte);

//       // Check if it's the end of a JPEG image (0xD9)
//       if (receivedByte == 0xD9) {
//         file.close();
//         Serial.println("Image received and saved as /received_image.jpg.");
//       }
//     }
    
//     // Close the file
//     file.close();

//     // Optional: Add a delay or perform additional processing as needed
//     delay(10);
//   }
//  File dir = SPIFFS.open("/");
//  File file = dir.openNextFile();
//  while(file){

//  }
// }


















// void loop() {
//   listDir(SPIFFS, "/", 0);
//   if (Serial.available() > 0) {
//     File file = SPIFFS.open("/received_image.jpg", "a");

//     //Serial.println();
//     // file.write(buffer, bytesRead);
//     // file.close();
//     // uint8_t buffer[128];
//     // size_t bytesRead = Serial.readBytes(buffer, 128);
    
//     // if (bytesRead > 0) {
//     //   // Write the received data to the file
//     //   File file = SPIFFS.open("/received_image.jpg", "a");
//     //   Serial.println(bytesRead);
//     //   file.write(buffer, bytesRead);
//     //   file.close();
//     // }
//   }
// }











///////////////////////////////////////////////////////////////////////////
// char pname[20];
// static int k = 10;
// const int CHUNK_SIZE = 128;


// void storeImageData() {
//   // Create a file in SPIFFS
//   File file = SPIFFS.open("/image.jpg", "w");
//   if (!file) {
//     Serial.println("Error opening file for writing");
//     return;
//   }

//   // Read data from serial and write it to the file
//   while (Serial.available()>0) {
//     char c = Serial.read();
//     file.write(c);
//   }

//   // Close the file
//   file.close();

//   Serial.println("Image data stored in flash");
// }

// void sendStoredImage() {
//   // Open the stored image file
//   File file = SPIFFS.open("/image.jpg", "r");
//   if (!file) {
//     Serial.println("Error opening file for reading");
//     return;
//   }

//   // Read the file and send it over serial
//   while (file.available()) {
//     char c = file.read();
//     Serial.write(c);
//   }

//   // Close the file
//   file.close();

//   Serial.println("END");  // Add an explicit end marker after sending the image
//   Serial.flush();         // Ensure all data is sent before moving on
//   delay(1000);            // Delay for stability (adjust if needed)
// }

// void sendImage(const char* filename) {
//   File file = SPIFFS.open(filename, "r");
//   if (!file) {
//     Serial.println("Failed to open file for reading");
//     return;
//   }

//   Serial.println("Sending image data...");

//   // Read and send the image data over Serial
//   while (file.available()) {
//     char currentByte = file.read();
//     Serial.write(currentByte);
//     //Serial.write(file.read());
//     //Serial.println(file.read());
//   }

//   file.close();
//   Serial.println("Image data sent.");
// }



// void receiveAndStoreImage(const char* filename) {
//   File file = SPIFFS.open(filename, "w");
//   if (!file) {
//     Serial.println("Failed to open file for writing");
//     return;
//   }

//   Serial.println("Receiving image data...");



//   while (Serial.available()>0) {
//     //if (Serial.available()) {
//       char usb_data = Serial.read();
//       Serial.println("read data serial connection");
//       temp_last = temp;
//       temp = usb_data;

//       if ((temp == 0xD9) && (temp_last == 0xFF)) {
//         buf[i++] = temp;
//         file.write(reinterpret_cast<const uint8_t*>(buf), i);
//         file.close();
//         Serial.println("Image save OK.");
//         is_header = false;
//         i = 0;
//       }

//       if (is_header) {
//         if (i < 256)
//           buf[i++] = temp;
//         else {
//           file.write(reinterpret_cast<const uint8_t*>(buf), 256);
//           i = 0;
//           buf[i++] = temp;
//         }
//       } else if ((temp == 0xD8) && (temp_last == 0xFF)) {
//         is_header = true;
//         buf[i++] = temp_last;
//         buf[i++] = temp;
//       }
//     }
//   //}
// }
// void sendFileContents(File file) {
//   if (file) {
//     while (file.available()) {
//       Serial.write(file.read());
//     }
//     Serial.println();  // Add a newline to indicate the end of the file
//   } else {
//     Serial.println("Failed to open file!");
//   }
// }


// void receiveAndStoreImage(fs::FS &fs, const char * path) {
//   // File file = SPIFFS.open(filename, "w");
//   Serial.printf("Reading Image: %s\r\n", path);
//   File file = fs.open(path);
//   if (!file) {
//     Serial.println("Failed to open file for writing");
//     while (1);
//   }

//   Serial.println("Receiving image data...");

//   bool is_header = false;
//   char temp_last = 0;
//   char temp = 0;
//   char buf[256];
//   int i = 0;

//   while (true) {
//     // Read JPEG data from USB
//     if (Serial.available()) {
      

//       Serial.println("serial is working");
//       char usb_data = Serial.read();

//       temp_last = temp;
//       temp = usb_data;

//       if ((temp == 0xD9) && (temp_last == 0xFF)) {
//         buf[i++] = temp;
//         file.write(reinterpret_cast<const uint8_t*>(buf), i);
//         file.close();
//         Serial.println("Image save OK.");
//         is_header = false;
//         i = 0;
//       }

//       if (is_header) {
//         if (i < 256)
//           buf[i++] = temp;
//         else {
//           file.write(reinterpret_cast<const uint8_t*>(buf), 256);
//           i = 0;
//           buf[i++] = temp;
//         }
//       } else if ((temp == 0xD8) && (temp_last == 0xFF)) {
//         is_header = true;
//         buf[i++] = temp_last;
//         buf[i++] = temp;
//       }
//     }
//   }
// }
// void setup() {
//   Serial.begin(115200);

//   if (!SPIFFS.begin(true)) {
//     Serial.println("Error mounting SPIFFS");
//     return;
//   }
  

//   // Additional setup code, if needed
// }
// to store image
// void loop(){
//   listDir(SPIFFS, "/", 0);
//   //sprintf((char*)pname, "/%05d.jpg", k);
//   //receiveAndStoreImage("/data.jpg");
//   storeImageData();
//   k++;
//   // sendFileContents();
//   // File dir = SPIFFS.open("/");
//   // File file = dir.openNextFile();
//   // listDir(SPIFFS, "/", 0);
//   // while(file){
//   //   Serial.print("Sending file: ");
//   //   Serial.println(file.name());
//   //   sendFileContents(file);
//   //   //file.flush();
//   //   // Close the file
//   //   file.close();
//   //   file = dir.openNextFile();
//   // }
//   // k++;
//   //delay(3000);
// }


// send data main loop 
// void loop(){
//   File dir = SPIFFS.open("/");
//   File file = dir.openNextFile();
//   listDir(SPIFFS, "/", 0);
//   while(file){
//     Serial.print("Sending file: ");
//     Serial.println(file.name());
//     sendFileContents(file);
//     //file.flush();
//     // Close the file
//     file.close();
//     file = dir.openNextFile();
//   }
// }

//logic to send image 
// void loop(){
//   File file = SPIFFS.open("/received_image.jpg", "r");
//   if (!file) {
//     Serial.println("Error opening file for reading");
//     return;
//   }
//   // Send the start marker
//   Serial.write('S');
//   // Send the data length (4 bytes, big-endian)
//   int dataLength = file.size();
//   for (int i = 3; i >= 0; --i) {
//     Serial.write((dataLength >> (8 * i)) & 0xFF);
//   }
//   // Send the entire image data
//   while (file.available()) {
//     char c = file.read();
//     Serial.write(c);
//   }
//   Serial.write('E');
//   Serial.println("Image data sent back to PC");

// }

// void saveImageData(const char* data, size_t length) {
//   static File file;
//   static bool imageInProgress = false;

//   // Check for JPEG start marker (0xFFD8)
//   if (length >= 2 && data[0] == 0xFF && data[1] == 0xD8) {
//     Serial.println("JPEG start marker (0xFFD8) found. Starting image capture.");
//     if (file) {
//       file.close();
//     }
//     file = SPIFFS.open("/captured_image.jpg", "w");
//     imageInProgress = true;
//   }

//   // Check for JPEG end marker (0xFFD9)
//   if (length >= 2 && data[length - 2] == 0xFF && data[length - 1] == 0xD9 && imageInProgress) {
//     Serial.println("JPEG end marker (0xFFD9) found. Image capture complete.");
//     if (file) {
//       file.close();
//     }
//     imageInProgress = false;
//   }

//   // Save the data to the file if image capture is in progress
//   // if (imageInProgress) {
//     if (file) {
//       size_t bytesWritten = file.write(reinterpret_cast<const uint8_t*>(data), length);
//       if (bytesWritten != length) {
//         Serial.println("Error writing to file!");
//       }
//     } else {
//       Serial.println("Error: File not open!");
//     }
//   }


// void receiveAndStoreImage(fs::FS &fs, const char * path) {
//   //File file = SPIFFS.open("/captured_image.jpg", "w");
//   File file ;
//   file = fs.open(path, FILE_WRITE);
//   if (!file) {
//     Serial.println("Failed to open file for writing");
//     return;
//   }

//   Serial.println("Receiving image data...");

//   // Read data from Serial until specific end bytes are detected
//   while (true) {
//     char data = Serial.read();
//     file.write(data);

//     // Check for end of image (e.g., 0xFFD9 for JPEG)
//     if (data == 0xFF) {
//       char nextByte = Serial.read();
//       file.write(nextByte);

//       if (nextByte == 0xD9) {
//         Serial.println("End of image detected.");
//         break;
//       }
//     }
//   }

//   file.close();
//   Serial.println("Image data received and saved.");
// }

// bool is_header = false;
// uint8_t temp_last = 0;
// uint8_t temp = 0;
// byte buf[256];
// int i = 0;

// void loop (){
//   listDir(SPIFFS, "/", 0);
//   File dir = SPIFFS.open("/");
//   File file = dir.openNextFile();
//   listDir(SPIFFS, "/", 0);
//   while(file){
//     Serial.print("Sending file: ");
//     Serial.println(file.name());
//     sendFileContents(file);
//     //file.flush();
//     // Close the file
//     file.close();
//     file = dir.openNextFile();
//   }

// }



/// @brief /working logic to accepts image
// void loop(){
//   listDir(SPIFFS, "/", 0);
//   if (Serial.available() > 0) {
//     // Create or open the file for writing
//     File file = SPIFFS.open("/image.jpg", "a");  // "a" stands for append mode

//     if (!file) {
//       Serial.println("Error opening file for writing");
//       return;
//     }

//     // Read and write image data byte by byte
//     // while (Serial.available() > 0) {
//     //   char receivedByte = Serial.read();
//     //   //Serial.println(receivedByte);
//     //   file.write(receivedByte);
//     // }

//     // Read and write image data byte by byte
//     while (Serial.available() > 0) {
//       char receivedByte = Serial.read();
//       // Check if it's the start of a JPEG image
//       if (receivedByte == 0xFF && file.size() == 0) {
//         file.write(receivedByte); // Write the first byte
//       } else if (receivedByte == 0xD9 && file.size() > 2) {
//         file.write(receivedByte); // Write the last byte
//         // Close the file
//         file.close();
//         Serial.println("Image received and saved as /received_image.jpg.");
//       } else {
//         file.write(receivedByte); // Write other bytes
//       }
//     }
//     // Close the file
//     file.close();

//     // Optional: Add a delay or perform additional processing as needed
//     delay(10);
//   }
// }

// // to read data
// void loop() {
//   // Wait for a request from the PC to send the stored image
//   // while (!Serial.available()) {
//   //   delay(10);
//   // }

//   // Open the file for reading
//   //  listDir(SPIFFS, "/", 0);
//   // File dir = SPIFFS.open("/");
//   // File file = dir.openNextFile();
//   // listDir(SPIFFS, "/", 0);
//   // while(file){

//   // }
//   File file = SPIFFS.open("/image.jpg", "r");
//   if (!file) {
//     Serial.println("Error opening file for reading");
//     return;
//   }

//   // Read and send image data byte by byte
//   int count = 0;
//   while (file.available()) {
//     char readByte = file.read();
//     Serial.write(readByte);
//     count++;
//   }

//   // Close the file
//   file.close();
//   Serial.println(count);
//   delay(1000);
// }

// void loop(){
//   //sprintf((char*)pname, "/%05d.jpg", k);
//   listDir(SPIFFS, "/", 0);
//   File file = SPIFFS.open("/image.jpg", "w");
//   if (!file) {
//     Serial.println("Failed to open file for writing");
//     //return;
//   }
//   Serial.println("created a file");
//   // Read data from Serial until specific end bytes are detected
//   while (true) {
//     temp_last = temp;
//     temp = Serial.read();
//     //Serial.println(temp);
//     // Check for end of image (e.g., 0xFFD9 for JPEG)
//     if ( (temp == 0xD9) && (temp_last == 0xFF) ){
//       buf[i++] = temp;
//       file.write(buf, i);
//       //Close the file
//       file.close();
//       Serial.println(F("Image save OK."));
//       //delay(5000);
//       is_header = false;
//       i = 0;

//     }
//     if (is_header == true){
//       if (i < 256)
//         buf[i++] = temp;
//       else
//         file.write(buf, 256);
//         i = 0;
//         buf[i++] = temp;
//     } 
  
//     else if ((temp == 0xD8) & (temp_last == 0xFF))
//     {
//       is_header = true;
//       buf[i++] = temp_last;
//       buf[i++] = temp;
//     }
//   }
//   file.close();
//   Serial.println("Image data received and saved.");
//   //receiveAndStoreImage(SPIFFS, pname);
  
//   delay(1000);
// }



//logic to read image from serial to flash
// void loop() {
//     // Read incoming data and store it in flash

// if (Serial.available() > 0) {
//     // Read data in chunks
//     char buffer[256];
//     static size_t bufferIndex = 0;
//     while (Serial.available() > 0 && bufferIndex < CHUNK_SIZE) {
//       Serial.println("data was connected");
//       buffer[bufferIndex] = Serial.read();
//       bufferIndex++;
//   }
//     if (bufferIndex == CHUNK_SIZE) {
//       Serial.println("data was connected");
//     // Buffer is full, process the data
//     saveImageData(buffer, bufferIndex);
//     bufferIndex = 0;  // Reset buffer index for the next chunk
//   }
//   }
//   listDir(SPIFFS, "/", 0);
// }
  //Serial.println(data);
  //delay(100);
  // if (data == 0xFF) {
  //   // Check for JPEG start marker (0xFFD8)
  //   if (Serial.peek() == 0xD8) {
  //     Serial.println("JPEG start marker (0xFFD8) found. Starting image capture.");
  //     if (file) {
  //       file.close();
  //     }
  //     file = SPIFFS.open("/captured_image.jpg", "w");
  //     imageInProgress = true;
  //   }

  //   // Check for JPEG end marker (0xFFD9)
  //   if (Serial.peek() == 0xD9 && imageInProgress) {
  //     Serial.println("JPEG end marker (0xFFD9) found. Image capture complete.");
  //     if (file) {
  //       file.close();
  //     }
  //     imageInProgress = false;
  //   }
  // }

  // // Save the data to the file if image capture is in progress
  // if (imageInProgress && file) {
  //   file.write(data);
  // }
  // listDir(SPIFFS, "/", 0);
  // Additional loop code, if needed



