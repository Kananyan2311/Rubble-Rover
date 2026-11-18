/*
  RubbleRover v2 (Router + AP Edition) - ESP32-CAM Independent Surveillance Hub
  Network Architecture: Independent Dual Mode (AP + Optional Router STA)
    1. Standalone Camera Access Point (AP): Always broadcasts "RubbleRover-CAM-AP" at http://192.168.5.1
    2. Wi-Fi Router Station Mode (STA): Connects to your Wi-Fi Router / Hotspot (via code or Web UI)
  Endpoints:
    - http://192.168.5.1/         : Standalone Neo-Brutalist Camera Surveillance Web Console
    - http://192.168.5.1:81/stream: Live High-FPS MJPEG Video Stream (also on port 80 /stream)
    - http://192.168.5.1/capture  : Single JPEG Frame Capture
    - http://192.168.5.1/light    : Tactical Searchlight Control (/light?state=1 or 0)
    - http://192.168.5.1/status   : JSON Telemetry (Router IP, AP IP, Camera state, Searchlight)
    - http://192.168.5.1/set_wifi : Dynamic Router Wi-Fi Configuration (/set_wifi?ssid=...&pass=...)
*/

#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <esp_http_server.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

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
// DUAL WI-FI CONFIGURATION (STANDALONE CAMERA AP + OPTIONAL ROUTER STA)
// =================================================================================================
// 1. External Wi-Fi Router / Mobile Hotspot Credentials
//    (Leave as "YOUR_WIFI_ROUTER_SSID" to start in pure, rock-solid AP mode at 192.168.5.1;
//     you can also enter your Router Wi-Fi live on the http://192.168.5.1 web page!)
String routerSSID = "YOUR_WIFI_ROUTER_SSID";     // <-- Optional: Enter your Wi-Fi Router SSID
String routerPass = "YOUR_WIFI_ROUTER_PASSWORD"; // <-- Optional: Enter your Wi-Fi Router Password

// Optional: S3 Master Controller IP on the router for automatic registration
String masterS3RouterIP = "";

// 2. Simultaneous Standalone Camera Access Point (Always active at http://192.168.5.1)
const char* CAM_AP_SSID = "RubbleRover-CAM-AP";
const char* CAM_AP_PASS = "RubbleRover2026";

IPAddress ap_IP(192, 168, 5, 1);
IPAddress ap_Gateway(192, 168, 5, 1);
IPAddress ap_Subnet(255, 255, 255, 0);

bool searchlightState = false;
bool cameraReady = false;
bool pendingWifiConnect = false;

// HTTP Server Handles (Port 80 for Console/Control, Port 81 for Dedicated Stream)
httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// =================================================================================================
// STANDALONE CAMERA SURVEILLANCE WEB CONSOLE (HTML)
// =================================================================================================
static const char CAM_INDEX_HTML[] = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>RubbleRover - Camera Surveillance Hub v2</title>
  <style>
    :root {
      --bg-dots: #F5C518;
      --bg-shell: #FFFDF7;
      --border: 3px solid #121212;
      --shadow: 5px 5px 0px #121212;
      --yellow: #FFD21E;
      --green: #4ECCA3;
      --red: #FF6B6B;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: system-ui, -apple-system, sans-serif; }
    body {
      background-color: var(--bg-dots);
      background-image: radial-gradient(#121212 1.5px, transparent 1.5px);
      background-size: 18px 18px;
      display: flex; justify-content: center; align-items: center;
      min-height: 100vh; padding: 14px; color: #121212;
    }
    .card {
      background: var(--bg-shell); border: var(--border); box-shadow: var(--shadow);
      border-radius: 20px; width: 100%; max-width: 720px; padding: 16px;
      display: flex; flex-direction: column; gap: 12px;
    }
    .header {
      display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 8px;
      background: #FAF4E6; border: 2px solid #121212; border-radius: 12px; padding: 10px 14px;
    }
    .title { font-size: 1.05rem; font-weight: 900; }
    .badge {
      background: var(--green); border: 2px solid #121212; border-radius: 8px;
      padding: 4px 10px; font-size: 0.78rem; font-weight: 900;
    }
    .viewport {
      background: #121212; border: var(--border); border-radius: 14px;
      aspect-ratio: 4/3; width: 100%; overflow: hidden; position: relative;
      display: flex; justify-content: center; align-items: center; color: #FFD21E;
      font-weight: 800; text-align: center;
    }
    .viewport img { width: 100%; height: 100%; object-fit: cover; display: block; }
    .controls { display: flex; gap: 10px; flex-wrap: wrap; }
    .btn {
      flex: 1; min-width: 140px; padding: 12px; font-size: 0.95rem; font-weight: 900;
      background: var(--yellow); border: 2px solid #121212; box-shadow: 3px 3px 0px #121212;
      border-radius: 10px; cursor: pointer; text-align: center;
    }
    .btn:active { transform: translate(2px, 2px); box-shadow: 1px 1px 0px #121212; }
    .btn.active { background: var(--green); }
    .info-box {
      background: #FFFFFF; border: 2px solid #121212; border-radius: 10px;
      padding: 10px 12px; font-size: 0.82rem; font-weight: 700; display: grid; gap: 4px;
    }
    .wifi-form {
      display: flex; gap: 6px; flex-wrap: wrap; align-items: center;
      background: #FAF4E6; border: 2px solid #121212; border-radius: 10px; padding: 10px;
    }
    .wifi-form input {
      flex: 1; min-width: 120px; padding: 8px; border: 2px solid #121212;
      border-radius: 6px; font-weight: 700;
    }
  </style>
</head>
<body>
  <div class="card">
    <div class="header">
      <div class="title">RUBBLEROVER // CAM SURVEILLANCE HUB v2</div>
      <div class="badge" id="statusBadge">CAMERA ONLINE</div>
    </div>
    <div class="viewport">
      <img id="camFeed" src="" alt="Loading MJPEG Video Stream..." onerror="fallbackStream()">
    </div>
    <div class="controls">
      <button class="btn" id="btnLight" onclick="toggleLight()">SEARCHLIGHT: OFF</button>
      <button class="btn" onclick="reloadStream()">RELOAD VIDEO</button>
    </div>
    <div class="info-box">
      <div>AP Network: <b>RubbleRover-CAM-AP</b> | AP IP: <b id="apIpTxt">192.168.5.1</b></div>
      <div>Router Status / IP: <b id="staIpTxt">Checking...</b></div>
      <div>Stream URL (for S3 HUD): <code id="streamUrlTxt">http://192.168.5.1:81/stream</code></div>
    </div>
    <div class="wifi-form">
      <input type="text" id="ssidIn" placeholder="Router Wi-Fi SSID">
      <input type="password" id="passIn" placeholder="Router Password">
      <button class="btn" style="flex:0 0 auto; min-width:110px; padding:8px 12px;" onclick="saveWifi()">JOIN ROUTER</button>
    </div>
  </div>
  <script>
    let lightOn = false;
    let usePort81 = true;
    const host = window.location.hostname || '192.168.5.1';

    function startStream() {
      const url = usePort81 ? `http://${host}:81/stream` : `http://${host}/stream`;
      document.getElementById('camFeed').src = `${url}?t=${Date.now()}`;
      document.getElementById('streamUrlTxt').innerText = url;
    }
    function fallbackStream() {
      if (usePort81) {
        usePort81 = false;
        setTimeout(startStream, 500);
      } else {
        usePort81 = true;
        setTimeout(startStream, 1500);
      }
    }
    function reloadStream() {
      startStream();
    }
    function toggleLight() {
      lightOn = !lightOn;
      fetch(`/light?state=${lightOn ? 1 : 0}`)
        .then(() => {
          const b = document.getElementById('btnLight');
          b.innerText = `SEARCHLIGHT: ${lightOn ? 'ON' : 'OFF'}`;
          b.classList.toggle('active', lightOn);
        }).catch(() => {});
    }
    function saveWifi() {
      const s = encodeURIComponent(document.getElementById('ssidIn').value.trim());
      const p = encodeURIComponent(document.getElementById('passIn').value);
      if (!s) return alert('Enter Router SSID');
      fetch(`/set_wifi?ssid=${s}&pass=${p}`)
        .then(r => r.json())
        .then(() => alert('Connecting to Router! Check Serial Monitor or Router Status in 6 seconds.'))
        .catch(() => {});
    }
    setInterval(() => {
      fetch('/status').then(r => r.json()).then(d => {
        document.getElementById('apIpTxt').innerText = d.ap_ip || '192.168.5.1';
        document.getElementById('staIpTxt').innerText = d.sta_ip || 'AP-ONLY (No Router)';
        const badge = document.getElementById('statusBadge');
        badge.innerText = d.cam_ok ? 'CAMERA ONLINE' : 'CAM SENSOR ERR';
        badge.style.background = d.cam_ok ? '#4ECCA3' : '#FF6B6B';
      }).catch(() => {});
    }, 2500);

    window.addEventListener('load', () => {
      setTimeout(startStream, 200);
    });
  </script>
</body>
</html>)rawliteral";

// =================================================================================================
// CAMERA INITIALIZATION (WITH AUTOMATIC PSRAM / DRAM FALLBACK)
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

  // Power-cycle camera sensor cleanly if PWDN pin is available
  if (PWDN_GPIO_NUM != -1) {
    pinMode(PWDN_GPIO_NUM, OUTPUT);
    digitalWrite(PWDN_GPIO_NUM, HIGH);
    delay(10);
    digitalWrite(PWDN_GPIO_NUM, LOW);
    delay(10);
  }

  if (psramFound()) {
    Serial.println("[CAMERA] PSRAM detected: Using VGA (640x480) double-buffered mode.");
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    Serial.println("[CAMERA] No PSRAM enabled: Using CIF (400x296) DRAM single-buffer fallback.");
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 15;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[CAMERA] Error 0x%x during initialization! Retrying with QVGA DRAM...\n", err);
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 16;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
    err = esp_camera_init(&config);
    if (err != ESP_OK) {
      Serial.printf("[CAMERA] FATAL Error 0x%x! Check ribbon cable & remove GPIO 0 jumper.\n", err);
      cameraReady = false;
      return err;
    }
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_vflip(s, 1);
    s->set_hmirror(s, 0);
  }

  cameraReady = true;
  Serial.println("[CAMERA] OV2640 Initialized Successfully!");
  return ESP_OK;
}

// =================================================================================================
// HTTP HANDLERS
// =================================================================================================
static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Connection", "close");
  return httpd_resp_send(req, CAM_INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t favicon_handler(httpd_req_t *req) {
  httpd_resp_set_status(req, "204 No Content");
  httpd_resp_set_hdr(req, "Connection", "close");
  return httpd_resp_send(req, NULL, 0);
}

static esp_err_t capture_handler(httpd_req_t *req) {
  if (!cameraReady) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    return httpd_resp_send(req, "Camera Offline", HTTPD_RESP_USE_STRLEN);
  }
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_set_status(req, "500 Internal Server Error");
    return httpd_resp_send(req, "Capture Failed", HTTPD_RESP_USE_STRLEN);
  }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Connection", "close");
  esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return res;
}

static esp_err_t stream_handler(httpd_req_t *req) {
  if (!cameraReady) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, "Camera sensor offline - check OV2640 ribbon cable", HTTPD_RESP_USE_STRLEN);
  }

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
      delay(20);
      continue;
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

static esp_err_t light_handler(httpd_req_t *req) {
  char buf[32];
  if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
    char param[8];
    if (httpd_query_key_value(buf, "state", param, sizeof(param)) == ESP_OK) {
      int state = atoi(param);
      searchlightState = (state != 0);
      digitalWrite(PIN_FLASH_LED, searchlightState ? HIGH : LOW);
      Serial.printf("[LIGHT] Searchlight LED state: %d\n", searchlightState ? 1 : 0);
    }
  }
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Connection", "close");
  return httpd_resp_send(req, "OK", 2);
}

static esp_err_t status_handler(httpd_req_t *req) {
  char json[200];
  String staIP = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "DISCONNECTED";
  String apIP = WiFi.softAPIP().toString();
  snprintf(json, sizeof(json),
           "{\"ap_ip\":\"%s\",\"sta_ip\":\"%s\",\"cam_ok\":%s,\"light\":%d,\"uptime_ms\":%lu}",
           apIP.c_str(), staIP.c_str(), cameraReady ? "true" : "false",
           searchlightState ? 1 : 0, (unsigned long)millis());
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Connection", "close");
  return httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

// URL-decode helper for Wi-Fi credentials
String urlDecode(const char *src) {
  String decoded = "";
  char a, b;
  while (*src) {
    if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
      if (a >= 'a') a -= 'a' - 'A';
      if (a >= 'A') a -= ('A' - 10); else a -= '0';
      if (b >= 'a') b -= 'a' - 'A';
      if (b >= 'A') b -= ('A' - 10); else b -= '0';
      decoded += (char)(16 * a + b);
      src += 3;
    } else if (*src == '+') {
      decoded += ' ';
      src++;
    } else {
      decoded += *src++;
    }
  }
  return decoded;
}

static esp_err_t set_wifi_handler(httpd_req_t *req) {
  char buf[160];
  if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
    char ssidParam[64] = {0};
    char passParam[64] = {0};
    if (httpd_query_key_value(buf, "ssid", ssidParam, sizeof(ssidParam)) == ESP_OK) {
      httpd_query_key_value(buf, "pass", passParam, sizeof(passParam));
      routerSSID = urlDecode(ssidParam);
      routerPass = urlDecode(passParam);
      pendingWifiConnect = true;
    }
  }
  String resp = "{\"status\":\"CONNECTING\",\"ssid\":\"" + routerSSID + "\"}";
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Connection", "close");
  return httpd_resp_send(req, resp.c_str(), HTTPD_RESP_USE_STRLEN);
}

void startCameraServer() {
  // 1. Port 80 Server: Web Console (/), Searchlight (/light), Status (/status), Wi-Fi (/set_wifi), Capture (/capture), Stream (/stream)
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.ctrl_port = 32768;
  config.stack_size = 8192;          // Prevent stack overflow on String/printf handlers
  config.max_open_sockets = 3;       // Keep within ESP32 10-socket LWIP limit across 2 servers
  config.lru_purge_enable = true;    // Automatically purge stale/half-open browser sockets
  config.max_uri_handlers = 8;

  httpd_uri_t index_uri    = { .uri = "/",            .method = HTTP_GET, .handler = index_handler,    .user_ctx = NULL };
  httpd_uri_t favicon_uri  = { .uri = "/favicon.ico", .method = HTTP_GET, .handler = favicon_handler,  .user_ctx = NULL };
  httpd_uri_t light_uri    = { .uri = "/light",       .method = HTTP_GET, .handler = light_handler,    .user_ctx = NULL };
  httpd_uri_t status_uri   = { .uri = "/status",      .method = HTTP_GET, .handler = status_handler,   .user_ctx = NULL };
  httpd_uri_t set_wifi_uri = { .uri = "/set_wifi",    .method = HTTP_GET, .handler = set_wifi_handler, .user_ctx = NULL };
  httpd_uri_t capture_uri  = { .uri = "/capture",     .method = HTTP_GET, .handler = capture_handler,  .user_ctx = NULL };
  httpd_uri_t stream_uri   = { .uri = "/stream",      .method = HTTP_GET, .handler = stream_handler,   .user_ctx = NULL };

  esp_err_t err80 = httpd_start(&camera_httpd, &config);
  if (err80 == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &favicon_uri);
    httpd_register_uri_handler(camera_httpd, &light_uri);
    httpd_register_uri_handler(camera_httpd, &status_uri);
    httpd_register_uri_handler(camera_httpd, &set_wifi_uri);
    httpd_register_uri_handler(camera_httpd, &capture_uri);
    httpd_register_uri_handler(camera_httpd, &stream_uri);
    Serial.println("[HTTP] Port 80 Web Console & Control Server ONLINE (http://192.168.5.1)");
  } else {
    Serial.printf("[HTTP] ERROR starting Port 80 server: 0x%x\n", err80);
  }

  // 2. Port 81 Server: Dedicated Non-Blocking MJPEG Stream Server (:81/stream)
  httpd_config_t stream_config = HTTPD_DEFAULT_CONFIG();
  stream_config.server_port = 81;
  stream_config.ctrl_port = 32769;
  stream_config.stack_size = 8192;
  stream_config.max_open_sockets = 2; // Dedicated stream sockets
  stream_config.lru_purge_enable = true;

  esp_err_t err81 = httpd_start(&stream_httpd, &stream_config);
  if (err81 == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    Serial.println("[HTTP] Port 81 Dedicated MJPEG Stream Server ONLINE (http://192.168.5.1:81/stream)");
  } else {
    Serial.printf("[HTTP] ERROR starting Port 81 server: 0x%x\n", err81);
  }
}

void connectToRouter(const String& ssid, const String& pass) {
  if (ssid.length() == 0 || ssid == "YOUR_WIFI_ROUTER_SSID") return;

  Serial.printf("[WIFI-STA] Connecting to Router: %s ...\n", ssid.c_str());
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long startConnect = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startConnect < 6000)) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WIFI-STA] CONNECTED to Router! Camera Router IP: http://%s\n",
                  WiFi.localIP().toString().c_str());
    if (masterS3RouterIP.length() > 0) {
      WiFiClient client;
      if (client.connect(masterS3RouterIP.c_str(), 80, 1000)) {
        String myIP = WiFi.localIP().toString();
        client.print("GET /register_cam?ip=" + myIP + " HTTP/1.1\r\n");
        client.print("Host: " + masterS3RouterIP + "\r\n");
        client.print("Connection: close\r\n\r\n");
        Serial.printf("[HANDSHAKE] Registered Camera Router IP (%s) with S3 Master (%s)!\n",
                      myIP.c_str(), masterS3RouterIP.c_str());
        client.stop();
      }
    }
  } else {
    // Stop continuous background channel-hopping so the AP at 192.168.5.1 never drops packets!
    Serial.println("[WIFI-STA] Router not found. Keeping AP rock-solid on http://192.168.5.1");
    WiFi.disconnect(false);
  }
}

// =================================================================================================
// SETUP
// =================================================================================================
void setup() {
  // Disable brownout detector so Wi-Fi + Camera power spikes never reset the board
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(100);

  Serial.println("\n====================================================================");
  Serial.println("  RUBBLEROVER v2: ESP32-CAM INDEPENDENT DUAL WI-FI SURVEILLANCE HUB");
  Serial.println("  Mode: Standalone Camera AP (192.168.5.1) + Optional Router STA");
  Serial.println("====================================================================");

  // Configure Searchlight LED
  pinMode(PIN_FLASH_LED, OUTPUT);
  digitalWrite(PIN_FLASH_LED, LOW);

  // Initialize OV2640 Camera FIRST
  initCamera();

  // Start Standalone Camera Access Point (192.168.5.1)
  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(50);

  bool hasRouterConfig = (routerSSID.length() > 0 && routerSSID != "YOUR_WIFI_ROUTER_SSID");
  WiFi.mode(hasRouterConfig ? WIFI_AP_STA : WIFI_AP);
  WiFi.setSleep(false);

  // Configure and start SoftAP on Channel 1 with static IP 192.168.5.1
  WiFi.softAPConfig(ap_IP, ap_Gateway, ap_Subnet);
  WiFi.softAP(CAM_AP_SSID, CAM_AP_PASS, 1, 0, 4);
  delay(100);
  WiFi.softAPConfig(ap_IP, ap_Gateway, ap_Subnet);

  Serial.print("[WIFI-AP] Online! SSID: ");
  Serial.print(CAM_AP_SSID);
  Serial.print(" | Open in browser: http://");
  Serial.println(WiFi.softAPIP());

  // Start Dual-Port HTTP Servers immediately so http://192.168.5.1 responds right away
  startCameraServer();

  // Connect to External Wi-Fi Router / Hotspot if configured
  if (hasRouterConfig) {
    connectToRouter(routerSSID, routerPass);
  } else {
    Serial.println("[WIFI-STA] Running in pure AP Mode at http://192.168.5.1 (Use Web UI or routerSSID to join a router).");
  }
}

// =================================================================================================
// MAIN LOOP
// =================================================================================================
void loop() {
  if (pendingWifiConnect) {
    pendingWifiConnect = false;
    delay(200);
    connectToRouter(routerSSID, routerPass);
  }

  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat >= 5000) {
    lastHeartbeat = millis();
    String staIP = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "AP-ONLY";
    Serial.printf("[CAM-HEARTBEAT] AP:http://%s (Clients:%d) | Router:%s | CamOK:%s\n",
                  WiFi.softAPIP().toString().c_str(),
                  WiFi.softAPgetStationNum(),
                  staIP.c_str(),
                  cameraReady ? "YES" : "NO");
  }

  delay(50);
}
