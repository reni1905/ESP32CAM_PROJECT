//======================================== Including the libraries.
#include <WiFi.h>
#include <WiFiClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> // Make sure to install this library: ArduinoJson by Benoit Blanchon
//======================================== 

//======================================== CAMERA_MODEL_AI_THINKER GPIO.
#define PWDN_GPIO_NUM       32
#define RESET_GPIO_NUM      -1
#define XCLK_GPIO_NUM       0
#define SIOD_GPIO_NUM       26
#define SIOC_GPIO_NUM       27

#define Y9_GPIO_NUM         35
#define Y8_GPIO_NUM         34
#define Y7_GPIO_NUM         39
#define Y6_GPIO_NUM         36
#define Y5_GPIO_NUM         21
#define Y4_GPIO_NUM         19
#define Y3_GPIO_NUM         18
#define Y2_GPIO_NUM         5
#define VSYNC_GPIO_NUM      25
#define HREF_GPIO_NUM       23
#define PCLK_GPIO_NUM       22
//======================================== 

// LED Flash PIN (GPIO 4)
#define FLASH_LED_PIN 4

// LED Status Pin
#define LED_RED_PIN 13   // Menggunakan GPIO 13 yang aman untuk output

// Push Button PIN (GPIO 16) 
#define BUTTON_PIN 12

//======================================== OLED Display Configuration
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // OLED I2C address (biasanya 0x3C atau 0x3D)

// Custom I2C pins untuk OLED 
#define SDA_PIN 14  // Pin SDA untuk OLED
#define SCL_PIN 15  // Pin SCL untuk OLED

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//======================================== 

//======================================== Insert network credentials.
const char* ssid = "ren";
const char* password = "12345678";
//======================================== 

//======================================== Variables for Button Control.
bool buttonState = HIGH;      // Current button state
bool lastButtonState = HIGH;      // Previous button state
bool captureRequested = false;    // Flag to indicate capture is requested
unsigned long lastDebounceTime = 0;  // Last time the output pin was toggled
unsigned long debounceDelay = 50;    // Debounce time; increase if the output flickers
//======================================== 

//======================================== Variables for Status Management.
enum CameraStatus {
  INITIALIZING,
  CONNECTING_WIFI,
  READY,
  CAPTURING,
  UPLOADING,
  SUCCESS,
  ERROR_CAPTURE,
  ERROR_UPLOAD,
  PROCESSING_BARCODE,
  DISPLAY_RESULT,
  ERROR_PROCESSING
};

CameraStatus currentStatus = INITIALIZING;
//======================================== 

// Server Address or Server IP.
String serverName = "10.53.235.35";  //--> Change with your server computer's IP address or your Domain name.
// The file path "upload_img.php" on the server folder.
String serverPathUpload = "/ESP32CAM/upload_img.php";
// The file path "barcode_processor.php" on the server folder.
String serverPathProcess = "/ESP32CAM/barcode_processor.php";
// Server Port.
const int serverPort = 80;

// Variable to set capture photo with LED Flash.
bool LED_Flash_ON = true;

// Initialize WiFiClient.
WiFiClient client;

// Variables to store barcode result
String productBarcode = "";
String productName = "";
String productStatus = "";

// Forward declarations
void processBarcodeOnServer(String filename);
void checkButton();
void sendPhotoToServer();
void updateOLED();
void setLEDStatus(String status);
//________________________________________________________________________________ setLEDStatus()
void setLEDStatus(String status) {
  digitalWrite(LED_RED_PIN, LOW);   // Matikan LED merah
  
  if (status == "boikot") {
    digitalWrite(LED_RED_PIN, HIGH);
    Serial.println("LED Merah Menyala: Produk Boikot");
  } 
}

//________________________________________________________________________________ updateOLED()
void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Header
  display.println("ESP32-CAM Status");
  display.println("================");

  // Status berdasarkan kondisi saat ini
  switch (currentStatus) {
    case INITIALIZING:
      display.println("Status: INITIALIZING");
      display.println("Setting up camera...");
      setLEDStatus("off");
      break;
      
    case CONNECTING_WIFI:
      display.println("Status: CONNECTING");
      display.print("WiFi: ");
      display.println(ssid);
      display.println("Please wait...");
      setLEDStatus("off");
      break;
      
    case READY:
      display.println("Status: READY");
      display.println("Press button to");
      display.println("capture photo");
      display.println("");
      display.print("WiFi: ");
      display.println(WiFi.localIP());
      setLEDStatus("off");
      break;
      
    case CAPTURING:
      display.println("Status: CAPTURING");
      display.println("Taking photo...");
      display.println("Please wait");
      setLEDStatus("off");
      break;
      
    case UPLOADING:
      display.println("Status: UPLOADING");
      display.println("Sending to server...");
      display.println("Please wait");
      setLEDStatus("off");
      break;
      
    case SUCCESS:
      display.println("Status: SUCCESS");
      display.println("Photo uploaded!");
      display.println("Ready to process...");
      setLEDStatus("off");
      break;
      
    case PROCESSING_BARCODE:
      display.println("Status: PROCESSING");
      display.println("Decoding barcode...");
      display.println("Please wait");
      setLEDStatus("off");
      break;

    case DISPLAY_RESULT:
      display.println("Status: RESULT");
      display.print("Barcode: ");
      display.println(productBarcode);
      display.print("Product: ");
      display.println(productName);
      display.print("Status: ");
      display.println(productStatus);
      setLEDStatus(productStatus); // Panggil fungsi LED di sini
      break;
      
    case ERROR_CAPTURE:
      display.println("Status: ERROR");
      display.println("Camera capture");
      display.println("failed!");
      display.println("Restarting...");
      setLEDStatus("off");
      break;
      
    case ERROR_UPLOAD:
      display.println("Status: ERROR");
      display.println("Upload failed!");
      display.println("Check connection");
      display.println("Press to retry");
      setLEDStatus("off");
      break;

    case ERROR_PROCESSING:
      display.println("Status: ERROR");
      display.println("Processing failed!");
      display.println("Check connection");
      display.println("Press to retry");
      setLEDStatus("off");
      break;
  }

  display.display();
}
//________________________________________________________________________________ 

//________________________________________________________________________________ sendPhotoToServer()
void sendPhotoToServer() {
  Serial.println();
  Serial.println("-----------");
  
  //---------------------------------------- Pre capture for accurate timing.
  Serial.println("Taking a photo...");
  currentStatus = CAPTURING;
  updateOLED();

  if (LED_Flash_ON == true) {
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(1000);
  }
  
  // Capturing 4 frames to improve image quality
  for (int i = 0; i <= 3; i++) {
    camera_fb_t * fb = esp_camera_fb_get();
    if(!fb) {
      Serial.println("Camera capture failed");
      currentStatus = ERROR_CAPTURE;
      updateOLED();
      delay(3000);
      ESP.restart(); // Restart on camera error
      return;
    }
    esp_camera_fb_return(fb);
    delay(200);
  }
  
  camera_fb_t * fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    currentStatus = ERROR_CAPTURE;
    updateOLED();
    delay(3000);
    ESP.restart(); // Restart on camera error
    return;
  }
  
  if (LED_Flash_ON == true) digitalWrite(FLASH_LED_PIN, LOW);
  
  Serial.println("Taking a photo was successful.");
  //---------------------------------------- 

  // Update status to uploading
  currentStatus = UPLOADING;
  updateOLED();

  Serial.println("Connecting to server: " + serverName);

  if (client.connect(serverName.c_str(), serverPort)) {
    Serial.println("Connection successful!");    
      
    String post_data = "--dataMarker\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"esp32_cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String head =  post_data;
    String boundary = "\r\n--dataMarker--\r\n";
    
    uint32_t imageLen = fb->len;
    uint32_t dataLen = head.length() + boundary.length();
    uint32_t totalLen = imageLen + dataLen;
    
    client.println("POST " + serverPathUpload + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=dataMarker");
    client.println();
    client.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n = 0; n < fbLen; n = n + 1024) {
      if (n + 1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen % 1024 > 0) {
        size_t remainder = fbLen % 1024;
        client.write(fbBuf, remainder);
      }
    }
    client.print(boundary);
    
    esp_camera_fb_return(fb);
    
    String responseBody = "";
    bool inBody = false;
    unsigned long timeout = millis();
    while (client.connected() && (millis() - timeout < 10000)) {
        if (client.available()) {
            char c = client.read();
            if (inBody) {
                responseBody += c;
            } else {
                if (c == '\n') {
                    if (responseBody.length() == 1) { // Empty line after headers
                        inBody = true;
                    }
                    responseBody = "";
                } else if (c != '\r') {
                    responseBody += c;
                }
            }
            timeout = millis();
        }
    }
    client.stop();

    // Parse JSON response from upload_img.php
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, responseBody);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      currentStatus = ERROR_UPLOAD;
      updateOLED();
    } else {
      if (doc["status"] == "success") {
        Serial.println("Photo uploaded successfully.");
        currentStatus = SUCCESS;
        updateOLED();
        String filename = doc["filename"];
        processBarcodeOnServer(filename);
      } else {
        Serial.println("Upload failed. Server response:");
        Serial.println(doc["message"].as<String>());
        currentStatus = ERROR_UPLOAD;
        updateOLED();
      }
    }
  } else {
    client.stop();
    esp_camera_fb_return(fb);
    Serial.println("Connection to server failed.");
    currentStatus = ERROR_UPLOAD;
    updateOLED();
  }
}
//________________________________________________________________________________ 

//________________________________________________________________________________ processBarcodeOnServer()
void processBarcodeOnServer(String filename) {
  currentStatus = PROCESSING_BARCODE;
  updateOLED();
  delay(1000);

  HTTPClient http;
  String serverUrl = "http://" + serverName + serverPathProcess + "?image=" + filename;
  
  Serial.println("Connecting to process barcode: " + serverUrl);
  
  http.begin(client, serverUrl);
  
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("HTTP Response Code: " + String(httpCode));
    Serial.println("Server Response: " + payload);

    // Parse JSON response from barcode_processor.php
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      productName = "Parsing error";
      productStatus = "N/A";
    } else {
      if (doc["status"] == "success") {
        productBarcode = doc["barcode"].as<String>();
        productName = doc["nama_produk"].as<String>();
        productStatus = doc["produk_status"].as<String>();
      } else {
        productName = doc["message"].as<String>();
        productStatus = "N/A";
      }
    }
    currentStatus = DISPLAY_RESULT;
    updateOLED();
    // Tambahan: Tambahkan delay dan kembali ke mode READY
    delay(5000); // Tampilkan hasil selama 5 detik
    currentStatus = READY;
    updateOLED();
  } else {
    Serial.println("HTTP GET failed. Error: " + http.errorToString(httpCode));
    currentStatus = ERROR_PROCESSING;
    updateOLED();
  }
  
  http.end();
}
//________________________________________________________________________________ 

//________________________________________________________________________________ checkButton()
void checkButton() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        if (currentStatus == READY || currentStatus == ERROR_UPLOAD || currentStatus == ERROR_PROCESSING) {
          captureRequested = true;
          Serial.println("Button pressed! Capture requested.");
        }
      }
    }
  }
  lastButtonState = reading;
}
//________________________________________________________________________________ 

//________________________________________________________________________________ VOID SETUP()
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  Serial.println();
  pinMode(FLASH_LED_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Wire.begin(SDA_PIN, SCL_PIN);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  } else {
    Serial.println("OLED initialized successfully");
    display.clearDisplay();
    display.display();
  }
  currentStatus = INITIALIZING;
  updateOLED();
  delay(2000);
  WiFi.mode(WIFI_STA);
  Serial.println();
  currentStatus = CONNECTING_WIFI;
  updateOLED();
  Serial.println();
  Serial.print("Connecting to : ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  int connecting_process_timed_out = 20;
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
    if(connecting_process_timed_out == 0) {
      Serial.println();
      Serial.print("Failed to connect to ");
      Serial.println(ssid);
      Serial.println("Restarting the ESP32 CAM.");
      delay(1000);
      ESP.restart();
    }
  }
  Serial.println();
  Serial.print("Successfully connected to ");
  Serial.println(ssid);
  Serial.println();
  Serial.print("Set the camera ESP32 CAM...");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 8;
    config.fb_count = 1;
  }
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println();
    Serial.println("Restarting the ESP32 CAM.");
    delay(1000);
    ESP.restart();
  }
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_SXGA);
  Serial.println();
  Serial.println("Set camera ESP32 CAM successfully.");
  Serial.println();
  Serial.println("ESP32-CAM ready! Press the button to capture and send photo to server.");
  currentStatus = READY;
  updateOLED();
}
//________________________________________________________________________________ 

//________________________________________________________________________________ VOID LOOP()
void loop() {
  checkButton();
  if (captureRequested) {
    captureRequested = false;
    sendPhotoToServer();
  }
  delay(10);
}
//________________________________________________________________________________
