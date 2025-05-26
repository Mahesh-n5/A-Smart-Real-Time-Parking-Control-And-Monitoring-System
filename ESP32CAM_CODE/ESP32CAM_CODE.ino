#include "WiFi.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <FS.h>
#include <HTTPClient.h>

const char* ssid = "your_wifi_username";
const char* password = "your_wifi_password";

AsyncWebServer server(80);
bool takeNewPhoto = false;

#define FILE_PHOTO "/photo.jpg"

#define TRIG_PIN 14  
#define ECHO_PIN 15 

#define MIN_DISTANCE 10
#define MAX_DISTANCE 50

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// HTML Page
const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE HTML><html>
<head><meta name="viewport" content="width=device-width, initial-scale=1">
<style>body { text-align:center; } .vert { margin-bottom: 10%; } .hori{ margin-bottom: 0%; }</style>
</head>
<body>
  <div id="container">
    <h2>ESP32-CAM Photo Capture</h2>
    <p>
      <button onclick="capturePhoto()">CAPTURE PHOTO</button>
      <button onclick="location.reload();">REFRESH PAGE</button>
    </p>
  </div>
  <div><img src="saved-photo" id="photo" width="70%"></div>
</body>
<script>
  function capturePhoto() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', "/capture", true);
    xhr.send();
  }
</script>
</html>)rawliteral";

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi Connected!");
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    ESP.restart();
  }
  Serial.println("SPIFFS Initialized");

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera Init Failed: 0x%x", err);
    ESP.restart();
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  });
  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request) {
    takeNewPhoto = true;
    request->send(200, "text/plain", "Taking Photo...");
  });
  server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, FILE_PHOTO, "image/jpg", false);
  });
  server.begin();
}
void loop() {
  // Measure Distance
  float distance = measureDistance();
  Serial.printf("Distance: %.2f cm\n", distance);

  if (distance >= MIN_DISTANCE && distance <= MAX_DISTANCE) {
    takeNewPhoto = true;
  }

  if (takeNewPhoto) {
    capturePhotoSaveSpiffs();
    takeNewPhoto = false;
  }

  delay(100);
}

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2; // cm
}

bool checkPhoto(fs::FS &fs) {
  File f_pic = fs.open(FILE_PHOTO);
  return (f_pic.size() > 100);
}

void capturePhotoSaveSpiffs() {
  camera_fb_t *fb = NULL;
  bool ok = false;

  do {
    Serial.println("Capturing Photo...");
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera Capture Failed");
      return;
    }

    if (SPIFFS.exists(FILE_PHOTO)) {
      SPIFFS.remove(FILE_PHOTO);
    }

    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to Open File for Writing");
      esp_camera_fb_return(fb);
      return;
    }

    size_t bytesWritten = file.write(fb->buf, fb->len);
    file.close();
    esp_camera_fb_return(fb);

    if (bytesWritten != fb->len) {
      Serial.println("File Write Incomplete");
    } else {
      Serial.printf("Photo Saved! Size: %d bytes\n", bytesWritten);
      ok = checkPhoto(SPIFFS);
    }
  } while (!ok);

  sendPhotoToPC();
}

void sendPhotoToPC() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi Not Connected");
    return;
  }

  HTTPClient http;
  String serverUrl = "http://Update to PC's IP:port/upload"; // 
  http.begin(serverUrl);

  File file = SPIFFS.open(FILE_PHOTO, FILE_READ);
  if (!file) {
    Serial.println("Failed to Open Photo for Reading");
    http.end();
    return;
  }

  size_t fileSize = file.size();
  if (fileSize == 0) {
    Serial.println("Photo File is Empty");
    file.close();
    http.end();
    return;
  }

  uint8_t *buffer = (uint8_t *)malloc(fileSize);
  if (!buffer) {
    Serial.println("Failed to Allocate Memory");
    file.close();
    http.end();
    return;
  }

  size_t bytesRead = file.read(buffer, fileSize);
  file.close();

  if (bytesRead != fileSize) {
    Serial.println("Failed to Read File into Buffer");
    free(buffer);
    http.end();
    return;
  }

  http.addHeader("Content-Type", "image/jpeg");
  int httpCode = http.POST(buffer, fileSize);

  if (httpCode > 0) {
    Serial.printf("Photo Uploaded! HTTP Code: %d\n", httpCode);
  } else {
    Serial.printf("Upload Failed. Error: %d\n", httpCode);
  }

  http.end();
  free(buffer); // Critical: Free allocated memory!
}
