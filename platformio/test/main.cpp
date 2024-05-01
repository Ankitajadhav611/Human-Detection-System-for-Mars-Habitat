#include <Arduino.h>
#include <FS.h>
#include "SPIFFS.h"

// put function declarations here:
int myFunction(int, int);
static uint8_t frameCount = 0;

void sendFileContents(File file) {
  if (file) {
    while (file.available()) {
      Serial.write(file.read());
    }
    Serial.println();  // Add a newline to indicate the end of the file
  } else {
    Serial.println("Failed to open file!");
  }
}

void sendFileContents_marker(File file, const char *filename, const char *marker) {
  //File file = fs.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Send the marker to indicate the file type
  Serial.write(marker);
  

  if (file) {
    while (file.available()) {
      Serial.write(file.read());
    }
    Serial.write(file.name());
    Serial.println();  // Add a newline to indicate the end of the file
  } else {
    Serial.println("Failed to open file!");
  }
  //file.close();
}


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

void setup() {
  // put your setup code here, to run once:
   Serial.begin(115200);

  if (!SPIFFS.begin()) {
    //Serial.println("SPIFFS initialization failed!");
    return;
  }
}


void loop() {
  // put your main code here, to run repeatedly:
  File dir = SPIFFS.open("/");
  File file = dir.openNextFile();
  //listDir(SPIFFS, "/", 0);
  while(file){
    //Serial.print("Sending file: ");
    //Serial.println(file.name());
    if (String(file.name()).endsWith(".jpg")){
      //Serial.println(file.name());
      sendFileContents_marker(file, file.name(), "IMG");
    }
    else if (String(file.name()).endsWith(".txt")){
      sendFileContents_marker(file, file.name(), "TXT");
      //Serial.println("found text");
    }
    // Send file contents
    //sendFileContents(file);
    //file.flush();
    // Close the file
    file.close();
    // if (SPIFFS.remove(file.name())) {
    //   Serial.println("File deleted successfully!");
    // } else {
    //   Serial.println("Failed to delete file!");
    // }
    //file.remove();

    // Open the next file
    file = dir.openNextFile();
    //delay(5000);
    frameCount++;
  }
    // Add a delay between sending files
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}