///////////////////////////////////////
//////library import//////////////
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Wire.h>
#include <FS.h>
#include "SPIFFS.h"
#include <TimeLib.h>
#include <arduino_lmic.h>
#include <JPEGDecoder.h>
#include <lab_human_detection_inferencing.h>
#include <ArduCAM.h>
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>

/////////////////////////////////////////
////initiate camera configuration////////
#define FORMAT_SPIFFS_IF_FAILED true
#define FRAME_BUFFER_COLS           320 
#define FRAME_BUFFER_ROWS           240

#define CUTOUT_COLS                 EI_CLASSIFIER_INPUT_WIDTH
#define CUTOUT_ROWS                 EI_CLASSIFIER_INPUT_HEIGHT
const int cutout_row_start = (FRAME_BUFFER_ROWS - CUTOUT_ROWS) / 2;
const int cutout_col_start = (FRAME_BUFFER_COLS - CUTOUT_COLS)/ 2;

uint16_t* pixel_buffer;
uint8_t* jpeg_buffer;

const int CS = 34;
const int CAM_POWER_ON = A10;
ArduCAM myCAM(OV5642, CS);
static int i = 0;
void capture_resize_image(uint16_t*& pixel_buffer, size_t width, size_t height){
  
  uint32_t jpeg_length = 0;
  uint8_t temp = 0, temp_last = 0;

  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();

  // Start capture
  myCAM.start_capture();
  Serial.println(F("Star Capture"));

  while (!myCAM.get_bit(ARDUCHIP_TRIG , CAP_DONE_MASK));
  Serial.println(F("Capture Done."));
  delay(50);
  myCAM.clear_fifo_flag();

  jpeg_length = myCAM.read_fifo_length();
  jpeg_buffer = static_cast<uint8_t*>(heap_caps_malloc(jpeg_length * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM));

  if (jpeg_length == 0){
    Serial.println("No data in Arducam FIFO buffer");
    return;
  }
  i = 0;
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  size_t new_jpeg_length;
  for (int index = 0; index < jpeg_length; index++){
    temp_last = temp;
    temp = SPI.transfer(0x00);
    jpeg_buffer[index] = temp;
    if ((temp == 0xD9) && (temp_last == 0xFF)){
      // Calculate the new length
      new_jpeg_length = index + 1;
      // Resize the buffer to the new length
      jpeg_buffer = static_cast<uint8_t*>(heap_caps_realloc(jpeg_buffer, new_jpeg_length * sizeof(uint8_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM));
      break;
    }
  }

  delayMicroseconds(15);
  myCAM.CS_HIGH();

  // Check for the start marker (0xFFD8) at the beginning
  if (jpeg_buffer[0] != 0xFF || jpeg_buffer[1] != 0xD8) {
      Serial.println(F("Error: Invalid JPEG start marker"));
      //return 0;
  }

  // // Check for the end marker (0xFFD9) at the end
  if (jpeg_buffer[new_jpeg_length - 2] != 0xFF || jpeg_buffer[new_jpeg_length - 1] != 0xD9) {
      Serial.println(F("Error: Invalid JPEG end marker"));
     // return 0;
  }
  //jpeg store data

  JpegDec.decodeArray(jpeg_buffer, jpeg_length);

  // Crop the image by keeping a certain number of MCUs in each dimension
  const int keep_x_mcus = width / JpegDec.MCUWidth;
  const int keep_y_mcus = height / JpegDec.MCUHeight;

  // Calculate how many MCUs we will throw away on the x axis
  const int skip_x_mcus = JpegDec.MCUSPerRow - keep_x_mcus;
  // Roughly center the crop by skipping half the throwaway MCUs at the
  // beginning of each row
  const int skip_start_x_mcus = skip_x_mcus / 2;
  // Index where we will start throwing away MCUs after the data
  const int skip_end_x_mcu_index = skip_start_x_mcus + keep_x_mcus;
  // Same approach for the columns
  const int skip_y_mcus = JpegDec.MCUSPerCol - keep_y_mcus;
  const int skip_start_y_mcus = skip_y_mcus / 2;
  const int skip_end_y_mcu_index = skip_start_y_mcus + keep_y_mcus;
  
  uint16_t *pImg;

  while (JpegDec.read()) {
        // Skip over the initial set of rows
      if (JpegDec.MCUy < skip_start_y_mcus) {
        continue;
      }
      // Skip if we're on a column that we don't want
      if (JpegDec.MCUx < skip_start_x_mcus ||
          JpegDec.MCUx >= skip_end_x_mcu_index) {
        continue;
      }
      // Skip if we've got all the rows we want
      if (JpegDec.MCUy >= skip_end_y_mcu_index) {
        continue;
      }
      // Pointer to the current pixel
      pImg = JpegDec.pImage;

      // The x and y indexes of the current MCU, ignoring the MCUs we skip
      int relative_mcu_x = JpegDec.MCUx - skip_start_x_mcus;
      int relative_mcu_y = JpegDec.MCUy - skip_start_y_mcus;

      // The coordinates of the top left of this MCU when applied to the output
      // image
      int x_origin = relative_mcu_x * JpegDec.MCUWidth;
      int y_origin = relative_mcu_y * JpegDec.MCUHeight;

        // Loop through the MCU's rows and columns
        for (int mcu_row = 0; mcu_row < JpegDec.MCUHeight; mcu_row++) {
            // The y coordinate of this pixel in the output index
            int current_y = y_origin + mcu_row;

            for (int mcu_col = 0; mcu_col < JpegDec.MCUWidth; mcu_col++) {
                // Read the color of the pixel as 16-bit integer
                uint16_t color = *pImg++;

                //coloured 565
                uint8_t r, g, b;
                r = ((color & 0xF800) >> 11) * 8;
                g = ((color & 0x07E0) >> 5) * 4;
                b = ((color & 0x001F) >> 0) * 8;

                // Calculate index
                int current_x = x_origin + mcu_col;
                size_t index = (current_y * width) + current_x;

                // Store the RGB565 pixel to the buffer
                pixel_buffer[index + 0] = r;
                pixel_buffer[index + 1] = g;
                pixel_buffer[index + 2] = b;

            }
        }
    }
    heap_caps_free(jpeg_buffer);
    jpeg_buffer = nullptr;
    delay(1000);
}

////////////////////////////////////
// Edge Impulse standardized print method, used for printing results after inference
void ei_printf(const char *format, ...) {
    static char print_buf[1024] = { 0 };

    va_list args;
    va_start(args, format);
    int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
    va_end(args);

    if (r > 0) {
        Serial.write(print_buf);
    }
}
// Convert RBG565 pixels into RBG888 pixels
void r565_to_rgb(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (color & 0xF800) >> 8;
    *g = (color & 0x07E0) >> 3;
    *b = (color & 0x1F) << 3;
}
// Data ingestion helper function for grabbing pixels from a framebuffer into Edge Impulse
// This method should be used as the .get_data callback of a signal_t 
int get_camera_data(size_t offset, size_t length, float *out_ptr)
{
	size_t bytes_left = length;
	size_t out_ptr_ix = 0;
	uint8_t r, g, b;

	// read byte for byte
    while (bytes_left != 0)
    {
        // grab the value and convert to r/g/b
        uint8_t pixel = pixel_buffer[offset];

        r565_to_rgb(pixel, &r, &g, &b);

        // then convert to out_ptr format
        float pixel_f = (r << 16) + (g << 8) + b;
        out_ptr[out_ptr_ix] = pixel_f;


		// and go to the next pixel
		out_ptr_ix++;
		bytes_left--;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// TTN network logic
// For normal use, we require that you edit the sketch to replace FILLMEIN
// with values assigned by the TTN console. However, for regression tests,
// we want to be able to compile these scripts. The regression tests define
// COMPILE_REGRESSION_TEST, and in that case we define FILLMEIN to a non-
// working but innocuous value.
//
#ifdef COMPILE_REGRESSION_TEST
# define FILLMEIN 0
#else
# warning "You must replace the values marked FILLMEIN with real values from the TTN control panel!"
# define FILLMEIN (#dont edit this, edit the lines that use FILLMEIN)
#endif

static const u1_t PROGMEM APPEUI[8]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]={0xAA, 0x59, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70};
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
// The key shown here is the semtech default key.
static const u1_t PROGMEM APPKEY[16] = {0xBD, 0x1B, 0xE6, 0x03, 0x76, 0xE5, 0x69, 0x29, 0x3E, 0xC0, 0xC7, 0xFA, 0xE1, 0xAC, 0x61, 0xED};
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

// static uint8_t mydata[] = "Hello, world!";
// static uint8_t mydata[];
static osjob_t sendjob;
byte mydata[80];  

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 10;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 19,
    .dio = {18, 17, LMIC_UNUSED_PIN},
};

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}
uint32_t userUTCTime; // Seconds since the UTC epoch

void printDigits(int digits, char* buffer) {
    if (digits < 10) {
        sprintf(buffer, "0%d", digits);
    } else {
        sprintf(buffer, "%d", digits);
    }
}

uint32_t receivedNetworkTime = 0; // Global variable to store received network time

void user_request_network_time_callback(void *pVoidUserUTCTime, int flagSuccess) {
    // Explicit conversion from void* to uint32_t* to avoid compiler errors
    uint32_t *pUserUTCTime = (uint32_t *) pVoidUserUTCTime;

    // A struct that will be populated by LMIC_getNetworkTimeReference.
    // It contains the following fields:
    //  - tLocal: the value returned by os_GetTime() when the time
    //            request was sent to the gateway, and
    //  - tNetwork: the seconds between the GPS epoch and the time
    //              the gateway received the time request
    lmic_time_reference_t lmicTimeReference;

    if (flagSuccess != 1) {
        Serial.println(F("USER CALLBACK: Not a success"));
        return;
    }
    // Serial.println("first fs");
    // Populate "lmic_time_reference"
    flagSuccess = LMIC_getNetworkTimeReference(&lmicTimeReference);
    // Serial.println(flagSuccess);
    // Serial.println("2nd fs");
    if (flagSuccess != 1) {
        Serial.println(F("USER CALLBACK: LMIC_getNetworkTimeReference didn't succeed"));
        return;
    }

    // Update userUTCTime, considering the difference between the GPS and UTC
    // epoch, and the leap seconds
    // *pUserUTCTime = lmicTimeReference.tNetwork + 315964800;
    *pUserUTCTime = lmicTimeReference.tNetwork + 315964800 + 7200;  // UTC+2 summer time 

    // Update userUTCTime with the received network time
    receivedNetworkTime = lmicTimeReference.tNetwork + 315964800 + 7200;
    // Add the delay between the instant the time was transmitted and
    // the current time

    // Current time, in ticks
    ostime_t ticksNow = os_getTime();
    Serial.println("time");
    Serial.println(ticksNow);
    // Time when the request was sent, in ticks
    ostime_t ticksRequestSent = lmicTimeReference.tLocal;
    uint32_t requestDelaySec = osticks2ms(ticksNow - ticksRequestSent) / 1000;
    *pUserUTCTime += requestDelaySec;

    // Update the system time with the time read from the network
    setTime(*pUserUTCTime);

    // Serial.print(F("The current UTC time is: "));
    // Serial.print(hour());
    // printDigits(minute());
    // printDigits(second());
    // Serial.print(' ');
    // Serial.print(day());
    // Serial.print('/');
    // Serial.print(month());
    // Serial.print('/');
    // Serial.print(year());
    // Serial.println();
}

void do_send(osjob_t* j){
    unsigned long startTime = millis();
    int countsum = 0;
    size_t size = 320 * 240;
    pixel_buffer = static_cast<uint16_t*>(heap_caps_malloc(size * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM));
    capture_resize_image(pixel_buffer, EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT);

    signal_t signal;
    signal.total_length = CUTOUT_COLS * CUTOUT_ROWS;
    signal.get_data = &get_camera_data;

    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false /* debug */);
    ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms. ): \n",result.timing.dsp, result.timing.classification,result.timing.anomaly);
    bool bb_found = result.bounding_boxes[0].value > 0;
    uint8_t count = 0;
    for (size_t ix = 0; ix < result.bounding_boxes_count; ix++){
                auto bb = result.bounding_boxes[ix];
                if (bb.value == 0) {
                    continue;
                }
                // ei_printf("    %s (", bb.label);
                // ei_printf_float(bb.value);
                // ei_printf(") [ x: %u, y: %u, width: %u, height: %u ]\n", bb.x, bb.y, bb.width, bb.height);
                count ++;
    }
    if (bb_found){
        LMIC_requestNetworkTime(user_request_network_time_callback, &userUTCTime);
        char predictions[100]; // Assuming a maximum of 100 characters for predictions
        sprintf(predictions, "Predictions: ");
                
        for (size_t ix = 0; ix < result.bounding_boxes_count; ix++) {
            auto bb = result.bounding_boxes[ix];
            if (bb.value == 0) {
                continue;
            }
            char prediction[50]; // Assuming a maximum of 50 characters for each prediction
            sprintf(prediction, "(x: %u, y: %u)  ",bb.x, bb.y);
            strcat(predictions, prediction);
        }

        // Calculate and print the length of the payload in bytes
        int payloadLength = strlen(predictions) + 20; // 20 for "Count: " + count
        Serial.print("Payload length: ");
        Serial.println(payloadLength);

        // Send the prediction values and count in the packet payload
        sprintf((char*)mydata, "%s, Count: %d", predictions, count);

        LMIC_setTxData2(1, mydata, strlen((char*)mydata), 0);
        Serial.println(F("Packet queued"));
    }
    else{
        Serial.println(F("no human found"));
        continue;
        }
    heap_caps_free(pixel_buffer);
    pixel_buffer = nullptr;
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("netid: ");
              Serial.println(netid, DEC);
              Serial.print("devaddr: ");
              Serial.println(devaddr, HEX);
              Serial.print("AppSKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                printHex2(artKey[i]);
              }
              Serial.println("");
              Serial.print("NwkSKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                              Serial.print("-");
                      printHex2(nwkKey[i]);
              }
              Serial.println();
            }
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
	    // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     Serial.println(F("EV_RFU1"));
        ||     break;
        */
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.print(F("Received "));
              Serial.print(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    Serial.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}
void setup() {
    Serial.begin(115200);
    Serial.println(F("Starting"));
    // if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    //     Serial.println("SPIFFS Mount Failed");
    //     return;
    // }
    // Serial.println("SPIFFS Mount Successful");

    // Serial.println("SPIFFS mounted");
    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
        pinMode(VCC_ENABLE, OUTPUT);
        digitalWrite(VCC_ENABLE, HIGH);
        delay(1000);
    #endif
        // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    uint8_t vid, pid;
  
    pinMode(CS, OUTPUT);
    pinMode(CAM_POWER_ON , OUTPUT);
    digitalWrite(CAM_POWER_ON, HIGH);
    Wire.begin();
    Serial.begin(115200);

    Serial.println(F("ArduCAM Start!"));
    SPI.begin();
    myCAM.wrSensorReg16_8(0xff, 0x01);
    myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);

    if ((vid != 0x56) || (pid != 0x42)) {
        Serial.println(F("Can't find OV5642 module!"));
    }
    else
        Serial.println(F("OV5642 detected."));
    myCAM.set_format(JPEG);
    myCAM.InitCAM();
    myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);   //VSYNC is active HIGH
    myCAM.OV5642_set_JPEG_size(OV5642_320x240);
    // Start job (sending automatically starts OTAA too)
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();

}