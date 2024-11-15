#define CAMERA_MODEL_XIAO_ESP32S3  // Define the camera model
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <SD.h>  // Use SD instead of SD_MMC
#include <FS.h>  // Include FS to handle file operations
#include <esp_camera.h>
#include <base64.h>
#include "camera_pins.h"  // Include your camera pin configuration

// WiFi credentials
#define WIFI_SSID "OJ"
#define WIFI_PASSWORD "OMAJAM125"

// Define the upload URL
const char *upload_url = "https://memento.noahcardoza.dev/api/upload/";

// SD card configurations
#define SD_CS_PIN 21  // Using GPIO 21 as specified in the provided information

// Timing constants
#define WIFI_TIMEOUT 15000  // 15 seconds
#define PHOTO_INTERVAL 5000  // 5 seconds
#define WIFI_RECONNECT_INTERVAL 15000  // 15 seconds
#define LED_BLINK_INTERVAL 1000  // Blinking interval of 1 second

bool wifi_connected = false;
unsigned long lastWiFiReconnectTime = 0;
unsigned long lastPhotoTime = 0;
unsigned long lastLEDBlinkTime = 0;
bool ledState = false;

void setup() {
  Serial.begin(115200);

  // Initialize digital pin LED_BUILTIN as an output
  pinMode(LED_BUILTIN, OUTPUT);

  // Turn on the built-in LED to indicate WiFi connection attempt
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);

  // Initialize the camera
  if (!initializeCamera()) {
    Serial.println("Failed to initialize the camera");
    return;
  }

  // Attempt to connect to WiFi
  connectToWiFi();

  // If WiFi is not connected, initialize the SD card
  if (!wifi_connected) {
    Serial.println("\nFailed to connect to WiFi, initializing SD card...");

    if (!SD.begin(SD_CS_PIN)) {
      Serial.println("SD card initialization failed!");
      return;
    }

    // Create "MomentoCaptures" directory if it doesn't exist
    if (!SD.exists("/MomentoCaptures")) {
      SD.mkdir("/MomentoCaptures");
      Serial.println("Created directory: /MomentoCaptures");
    }
  }

  // Start with the LED off
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  // Handle LED blinking
  if (millis() - lastLEDBlinkTime >= LED_BLINK_INTERVAL) {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    lastLEDBlinkTime = millis();
  }

  // Handle photo capturing every 5 seconds
  if (millis() - lastPhotoTime >= PHOTO_INTERVAL) {
    Serial.println("Taking a photo...");  // Debug statement to indicate photo capture
    takeAndProcessPhoto();
    lastPhotoTime = millis();
  }

  // Handle WiFi reconnection without blocking
  if (!wifi_connected && millis() - lastWiFiReconnectTime >= WIFI_RECONNECT_INTERVAL) {
    Serial.println("Attempting to reconnect to WiFi...");
    attemptWiFiReconnect();  // Non-blocking reconnection attempt
    lastWiFiReconnectTime = millis();
  }
}

void attemptWiFiReconnect() {
  // Check if WiFi is already connecting or connected
  if (WiFi.status() == WL_CONNECTED) {
    wifi_connected = true;
    Serial.println("\nConnected to WiFi");
    return;
  }

  // Start WiFi connection if not already connected
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long startAttemptTime = millis();

  // Non-blocking attempt to connect to WiFi
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT) {
    delay(10);  // Use a short, non-blocking delay to avoid disrupting other tasks
  }

  wifi_connected = (WiFi.status() == WL_CONNECTED);

  if (wifi_connected) {
    Serial.println("\nConnected to WiFi");
  } else {
    Serial.println("Failed to connect to WiFi");
  }
}

bool initializeCamera() {
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

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
  }
  config.jpeg_quality = 10;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return false;
  }

  // Get the camera sensor object
  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);

  return true;
}

void takeAndProcessPhoto() {
  unsigned long photoStartTime = millis();
  camera_fb_t *fb = esp_camera_fb_get();
  
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Start a manual blinking loop to ensure the LED blinks while processing
  while (millis() - photoStartTime < PHOTO_INTERVAL) {
    // Check if it's time to toggle the LED
    if (millis() - lastLEDBlinkTime >= LED_BLINK_INTERVAL) {
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
      lastLEDBlinkTime = millis();
    }

    // Break out of the loop after one photo is processed
    if (millis() - photoStartTime >= PHOTO_INTERVAL / 2) {
      break;
    }
  }

  // Proceed with sending or saving the photo
  if (wifi_connected) {
    // Attempt to send the photo to the server
    if (!sendPhotoToServer(fb->buf, fb->len)) {
      Serial.println("Failed to send photo to server");
    } else {
      Serial.println("Photo successfully sent to server");
    }
  } else {
    // Save the photo to the SD card if not connected to WiFi
    savePhotoToSD(fb->buf, fb->len);
  }

  esp_camera_fb_return(fb);
}

bool sendPhotoToServer(uint8_t *image_data, size_t len) {
  HTTPClient http;
  http.begin(upload_url);
  http.addHeader("Content-Type", "application/json");

  String base64Image = base64::encode(image_data, len);
  String jsonPayload = "{\"image\":\"" + base64Image + "\"}";

  // Simulate blinking while sending the photo
  unsigned long sendStartTime = millis();
  unsigned long lastBlinkTime = millis();
  int httpResponseCode = 0;

  // Start sending the photo in a blocking manner, but toggle the LED while waiting
  while ((millis() - sendStartTime) < WIFI_TIMEOUT) { // Use WIFI_TIMEOUT as a time limit
    // Check if it's time to toggle the LED
    if (millis() - lastBlinkTime >= LED_BLINK_INTERVAL) {
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
      lastBlinkTime = millis();
    }

    // Attempt to send the POST request
    httpResponseCode = http.POST(jsonPayload);

    // Break the loop if the request was completed
    if (httpResponseCode != 0) {
      break;
    }
  }

  // Debug information
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  http.end();
  return (httpResponseCode == 200);
}

void savePhotoToSD(uint8_t *image_data, size_t len) {
  static int photoNumber = 0;
  String path = "/MomentoCaptures/photo" + String(photoNumber++) + ".jpg";
  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  file.write(image_data, len);
  file.close();
  Serial.println("Photo saved to SD card: " + path);
}