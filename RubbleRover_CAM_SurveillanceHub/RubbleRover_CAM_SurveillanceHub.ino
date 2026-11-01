/*
  RubbleRover - ESP32-CAM High-Speed Video Satellite
  Static IP: 192.168.4.2 | Handshake to Master: 192.168.4.1
  Streams MJPEG video on /stream and searchlight on /light
*/

#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <esp_http_server.h>

// =================================================================================================
// AI-THINKER ESP32-CAM PIN DEFINITIONS
// =================================================================================================
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

// Tactical Searchlight Flash LED
#define PIN_FLASH_LED      4

// =================================================================================================
// TACTICAL NETWORK CREDENTIALS & STATIC IP
// =================================================================================================
const char* MASTER_AP_SSID = "RubbleRover-Tactical-AP";
const char* MASTER_AP_PASS = "RubbleRover2026";
const int   MASTER_CHANNEL = 1;

// Static IP Assignment on S3 Master AP
IPAddress sta_IP(192, 168, 4, 2);
IPAddress sta_Gateway(192, 168, 4, 1);
IPAddress sta_Subnet(255, 255, 255, 0);
IPAddress sta_DNS(192, 168, 4, 1);

// HTTP Server Handle
httpd_handle_t stream_httpd = NULL;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// =================================================================================================
// CAMERA INITIALIZATION
// =================================================================================================
esp_err_t initCamera() {
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
  config.frame_size = FRAMESIZE_VGA;     // 640x480 for crisp reconnaissance
  config.jpeg_quality = 12;              // 10-14 for high-speed streaming
  config.fb_count = 2;
  config.grab_mode = CAMERA_GRAB_LATEST; // Prevent latency buildup

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[CAMERA] Error 0x%x during initialization!\n", err);
    return err;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_vflip(s, 1);    // Sensor orientation adjustment
    s->set_hmirror(s, 0);
  }

  Serial.println("[CAMERA] OV2640 Initialized Successfully (640x480, Low Latency)!");
  return ESP_OK;
}

// =================================================================================================
// MJPEG VIDEO STREAM HANDLER
// =================================================================================================
static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      res = ESP_FAIL;
      break;
    }

    _jpg_buf_len = fb->len;
    _jpg_buf = fb->buf;

    res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    if (res != ESP_OK) { esp_camera_fb_return(fb); break; }

    size_t hlen = snprintf(part_buf, 64, _STREAM_PART, _jpg_buf_len);
    res = httpd_resp_send_chunk(req, part_buf, hlen);
    if (res != ESP_OK) { esp_camera_fb_return(fb); break; }

    res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    esp_camera_fb_return(fb);
    if (res != ESP_OK) break;
  }

  return res;
}

// SEARCHLIGHT CONTROL HANDLER
static esp_err_t light_handler(httpd_req_t *req) {
  char buf[32];
  if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
    char param[8];
    if (httpd_query_key_value(buf, "state", param, sizeof(param)) == ESP_OK) {
      int state = atoi(param);
      digitalWrite(PIN_FLASH_LED, state ? HIGH : LOW);
      Serial.printf("[LIGHT] Searchlight LED state: %d\n", state);
    }
  }
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, "OK", 2);
  return ESP_OK;
}

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.ctrl_port = 32768;

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t light_uri = {
    .uri       = "/light",
    .method    = HTTP_GET,
    .handler   = light_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    httpd_register_uri_handler(stream_httpd, &light_uri);
    Serial.println("[HTTP] Camera MJPEG Streaming Server online on port 80");
  }
}

// =================================================================================================
// RAPID HANDSHAKE REGISTRATION WITH MASTER ESP32-S3
// =================================================================================================
void registerWithMasterS3() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;
  if (client.connect("192.168.4.1", 80, 1000)) {
    client.print("GET /register_cam?ip=192.168.4.2 HTTP/1.1\r\n");
    client.print("Host: 192.168.4.1\r\n");
    client.print("Connection: close\r\n\r\n");
    Serial.println("[HANDSHAKE] Registered 192.168.4.2 with ESP32-S3 Master!");
    client.stop();
  }
}

// =================================================================================================
// SETUP
// =================================================================================================
void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n====================================================================");
  Serial.println("  RUBBLEROVER: ESP32-CAM HIGH-SPEED VIDEO SATELLITE");
  Serial.println("  Instant Handshake & Low-Latency Stream Architecture");
  Serial.println("====================================================================");

  // Configure Searchlight LED
  pinMode(PIN_FLASH_LED, OUTPUT);
  digitalWrite(PIN_FLASH_LED, LOW);

  // Initialize OV2640 Camera FIRST
  if (initCamera() != ESP_OK) {
    Serial.println("[FATAL] Camera initialization failed!");
  }

  // Optimize Wi-Fi for Instant AP Handshake
  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(50);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false); // Disable power saving for maximum streaming frame rate

  // Set Static IP on Master S3 Subnet (Eliminates DHCP negotiation delay)
  WiFi.config(sta_IP, sta_Gateway, sta_Subnet, sta_DNS);

  // Direct Channel-1 Association (Bypasses full RF band scan, connects in <300ms)
  Serial.printf("[WIFI-STA] Connecting immediately to Master AP: %s (Ch %d)...\n", MASTER_AP_SSID, MASTER_CHANNEL);
  WiFi.begin(MASTER_AP_SSID, MASTER_AP_PASS, MASTER_CHANNEL);

  unsigned long startConnect = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startConnect < 4000)) {
    delay(100);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WIFI-STA] CONNECTED in %lums! Node IP: %s\n", millis() - startConnect, WiFi.localIP().toString().c_str());
    registerWithMasterS3();
  } else {
    Serial.println("\n[WIFI-STA] S3 not yet online; running background reconnect in loop().");
  }

  // Start HTTP MJPEG Server
  startCameraServer();
}

// =================================================================================================
// MAIN LOOP
// =================================================================================================
void loop() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 1000) {
    lastCheck = millis();

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WIFI-STA] Fast reconnecting to Master AP...");
      WiFi.begin(MASTER_AP_SSID, MASTER_AP_PASS, MASTER_CHANNEL);
    } else {
      static bool registered = false;
      if (!registered) {
        registerWithMasterS3();
        registered = true;
      }
    }
  }

  delay(20);
}
