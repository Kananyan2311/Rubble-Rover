/*
  RubbleRover v2 (Router + AP Edition) - ESP32-CAM Independent Surveillance Hub
  Network Architecture: Independent Dual Mode (WIFI_AP_STA)
    1. Wi-Fi Router Station Mode (STA): Connects to your Wi-Fi Router / Hotspot
    2. Standalone Camera Access Point (AP): Broadcasts "RubbleRover-CAM-AP" at 192.168.5.1
  Endpoints:
    - http://<CAM_IP>/         : Standalone Neo-Brutalist Camera Surveillance Web Console
    - http://<CAM_IP>/stream   : Live 640x480 MJPEG Video Stream (also on port 81 :81/stream)
    - http://<CAM_IP>/light    : Tactical Searchlight Control (/light?state=1 or 0)
    - http://<CAM_IP>/status   : JSON Telemetry (Router IP, AP IP, Searchlight state)
    - http://<CAM_IP>/set_wifi : Dynamic Router Wi-Fi Configuration (/set_wifi?ssid=...&pass=...)
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
// DUAL WI-FI CONFIGURATION (ROUTER STA + STANDALONE CAMERA AP)
// =================================================================================================
// 1. External Wi-Fi Router / Mobile Hotspot Credentials
String routerSSID = "YOUR_WIFI_ROUTER_SSID";     // <-- Enter your Wi-Fi Router / Hotspot SSID
String routerPass = "YOUR_WIFI_ROUTER_PASSWORD"; // <-- Enter your Wi-Fi Router / Hotspot Password

// Optional: If S3 Master Controller is on the same router and you know its IP, enter it here
// for automatic camera IP registration (or leave empty to use standalone / manual IP).
String masterS3RouterIP = "";

// 2. Simultaneous Standalone Camera Access Point (Always active at 192.168.5.1)
const char* CAM_AP_SSID = "RubbleRover-CAM-AP";
const char* CAM_AP_PASS = "RubbleRover2026";

IPAddress ap_IP(192, 168, 5, 1);
IPAddress ap_Gateway(192, 168, 5, 1);
IPAddress ap_Subnet(255, 255, 255, 0);

bool searchlightState = false;

// HTTP Server Handles (Port 80 for Console/Control + Stream, Port 81 for Dedicated Stream)
httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// =================================================================================================
// STANDALONE CAMERA SURVEILLANCE WEB CONSOLE (HTML)
// =================================================================================================
static const char CAM_INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
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
    .title { font-size: 1.1rem; font-weight: 900; }
    .badge {
      background: var(--green); border: 2px solid #121212; border-radius: 8px;
      padding: 4px 10px; font-size: 0.78rem; font-weight: 900;
    }
    .viewport {
      background: #121212; border: var(--border); border-radius: 14px;
      aspect-ratio: 4/3; width: 100%; overflow: hidden; position: relative;
      display: flex; justify-content: center; align-items: center;
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
      <div class="badge" id="statusBadge">LIVE STREAM</div>
    </div>
    <div class="viewport">
      <img id="camFeed" src="" alt="Connecting to MJPEG Stream...">
    </div>
    <div class="controls">
      <button class="btn" id="btnLight" onclick="toggleLight()">SEARCHLIGHT: OFF</button>
      <button class="btn" onclick="reloadStream()">RELOAD VIDEO</button>
    </div>
    <div class="info-box">
      <div>AP Network: <b>RubbleRover-CAM-AP</b> | AP IP: <b id="apIpTxt">192.168.5.1</b></div>
      <div>Router Status / IP: <b id="staIpTxt">Checking...</b></div>
      <div>Direct Stream URL: <code id="streamUrlTxt">http://192.168.5.1:81/stream</code></div>
    </div>
    <div class="wifi-form">
      <input type="text" id="ssidIn" placeholder="Router Wi-Fi SSID">
      <input type="password" id="passIn" placeholder="Router Password">
      <button class="btn" style="flex:0 0 auto; min-width:110px; padding:8px 12px;" onclick="saveWifi()">JOIN ROUTER</button>
    </div>
  </div>
  <script>
    let lightOn = false;
    const host = window.location.hostname || '192.168.5.1';
    const streamUrl = `http://${host}:81/stream`;
    document.getElementById('camFeed').src = streamUrl;
    document.getElementById('streamUrlTxt').innerText = streamUrl;

    function reloadStream() {
      document.getElementById('camFeed').src = `${streamUrl}?t=${Date.now()}`;
    }
    function toggleLight() {
      lightOn = !lightOn;
      fetch(`/light?state=${lightOn ? 1 : 0}`)
        .then(() => {
          const b = document.getElementById('btnLight');
          b.innerText = `SEARCHLIGHT: ${lightOn ? 'ON' : 'OFF'}`;
          b.classList.toggle('active', lightOn);
        });
    }
    function saveWifi() {
      const s = encodeURIComponent(document.getElementById('ssidIn').value.trim());
      const p = encodeURIComponent(document.getElementById('passIn').value);
      if (!s) return alert('Enter Router SSID');
      fetch(`/set_wifi?ssid=${s}&pass=${p}`)
        .then(r => r.json())
        .then(() => alert('Connecting to Router! Check Serial Monitor or status in 5 seconds.'));
    }
    setInterval(() => {
      fetch('/status').then(r => r.json()).then(d => {
        document.getElementById('apIpTxt').innerText = d.ap_ip || '192.168.5.1';
        document.getElementById('staIpTxt').innerText = d.sta_ip || 'DISCONNECTED';
      }).catch(() => {});
    }, 2000);
  </script>
</body>
</html>)rawliteral";

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
// HTTP HANDLERS
// =================================================================================================
static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, CAM_INDEX_HTML, strlen(CAM_INDEX_HTML));
}

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
  httpd_resp_send(req, "OK", 2);
  return ESP_OK;
}

static esp_err_t status_handler(httpd_req_t *req) {
  String staIP = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "DISCONNECTED";
  String json = "{";
  json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"sta_ip\":\"" + staIP + "\",";
  json += "\"light\":" + String(searchlightState ? 1 : 0) + ",";
  json += "\"uptime_ms\":" + String(millis());
  json += "}";
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, json.c_str(), json.length());
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
      Serial.printf("[WIFI-STA] Connecting to new Router SSID: %s\n", routerSSID.c_str());
      WiFi.begin(routerSSID.c_str(), routerPass.c_str());
    }
  }
  String resp = "{\"status\":\"CONNECTING\",\"ssid\":\"" + routerSSID + "\"}";
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, resp.c_str(), resp.length());
}

void startCameraServer() {
  // 1. Port 80 Server: Web Console (/), Searchlight (/light), Status (/status), Wi-Fi (/set_wifi), and fallback (/stream)
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.ctrl_port = 32768;

  httpd_uri_t index_uri    = { .uri = "/",         .method = HTTP_GET, .handler = index_handler,    .user_ctx = NULL };
  httpd_uri_t light_uri    = { .uri = "/light",    .method = HTTP_GET, .handler = light_handler,    .user_ctx = NULL };
  httpd_uri_t status_uri   = { .uri = "/status",   .method = HTTP_GET, .handler = status_handler,   .user_ctx = NULL };
  httpd_uri_t set_wifi_uri = { .uri = "/set_wifi", .method = HTTP_GET, .handler = set_wifi_handler, .user_ctx = NULL };
  httpd_uri_t stream_uri   = { .uri = "/stream",   .method = HTTP_GET, .handler = stream_handler,   .user_ctx = NULL };

  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &light_uri);
    httpd_register_uri_handler(camera_httpd, &status_uri);
    httpd_register_uri_handler(camera_httpd, &set_wifi_uri);
    httpd_register_uri_handler(camera_httpd, &stream_uri);
    Serial.println("[HTTP] Camera Web Console & Control Server online on Port 80");
  }

  // 2. Port 81 Server: Dedicated Non-Blocking MJPEG Stream Server (:81/stream)
  httpd_config_t stream_config = HTTPD_DEFAULT_CONFIG();
  stream_config.server_port = 81;
  stream_config.ctrl_port = 32769;

  if (httpd_start(&stream_httpd, &stream_config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    Serial.println("[HTTP] Dedicated High-FPS MJPEG Stream Server online on Port 81 (/stream)");
  }
}

// Optional Handshake with S3 Master if on the same router and masterS3RouterIP is configured
void registerWithMasterOnRouter() {
  if (WiFi.status() != WL_CONNECTED || masterS3RouterIP.length() == 0) return;

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

// =================================================================================================
// SETUP
// =================================================================================================
void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n====================================================================");
  Serial.println("  RUBBLEROVER v2: ESP32-CAM INDEPENDENT DUAL WI-FI SURVEILLANCE HUB");
  Serial.println("  Mode: Simultaneous Wi-Fi Router (STA) + Standalone Camera AP");
  Serial.println("====================================================================");

  // Configure Searchlight LED
  pinMode(PIN_FLASH_LED, OUTPUT);
  digitalWrite(PIN_FLASH_LED, LOW);

  // Initialize OV2640 Camera FIRST
  if (initCamera() != ESP_OK) {
    Serial.println("[FATAL] Camera initialization failed!");
  }

  // Dual Wi-Fi Mode: Simultaneous Standalone Camera AP + External Wi-Fi Router
  WiFi.persistent(false);
  WiFi.disconnect(true);
  delay(50);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);

  // 1. Start Always-On Standalone Camera Access Point (192.168.5.1)
  WiFi.softAPConfig(ap_IP, ap_Gateway, ap_Subnet);
  WiFi.softAP(CAM_AP_SSID, CAM_AP_PASS, 6, 0, 4);
  Serial.print("[WIFI-AP] Online! SSID: ");
  Serial.print(CAM_AP_SSID);
  Serial.print(" | Camera AP Console: http://");
  Serial.println(WiFi.softAPIP());

  // 2. Connect to External Wi-Fi Router / Hotspot (if configured)
  if (routerSSID.length() > 0 && routerSSID != "YOUR_WIFI_ROUTER_SSID") {
    Serial.printf("[WIFI-STA] Connecting to Wi-Fi Router: %s ...\n", routerSSID.c_str());
    WiFi.begin(routerSSID.c_str(), routerPass.c_str());

    unsigned long startConnect = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startConnect < 6000)) {
      delay(250);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WIFI-STA] CONNECTED to Router! Camera Router IP: http://%s\n",
                    WiFi.localIP().toString().c_str());
      registerWithMasterOnRouter();
    } else {
      Serial.println("[WIFI-STA] Router not reached yet; background auto-reconnect active.");
    }
  } else {
    Serial.println("[WIFI-STA] Router SSID not configured. Connect to RubbleRover-CAM-AP (http://192.168.5.1) or set ROUTER_SSID.");
  }

  // Start Dual-Port HTTP Servers
  startCameraServer();
}

// =================================================================================================
// MAIN LOOP
// =================================================================================================
void loop() {
  // Background Router Reconnect & Status Heartbeat
  static unsigned long lastRouterRetry = 0;
  if (millis() - lastRouterRetry >= 10000) {
    lastRouterRetry = millis();
    if (routerSSID.length() > 0 && routerSSID != "YOUR_WIFI_ROUTER_SSID" && WiFi.status() != WL_CONNECTED) {
      Serial.printf("[WIFI-STA] Background reconnecting to Router: %s...\n", routerSSID.c_str());
      WiFi.begin(routerSSID.c_str(), routerPass.c_str());
    }
  }

  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat >= 5000) {
    lastHeartbeat = millis();
    String staIP = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "DISCONNECTED";
    Serial.printf("[CAM-HEARTBEAT] AP:http://%s (Clients:%d) | Router:%s | Stream: /stream\n",
                  WiFi.softAPIP().toString().c_str(),
                  WiFi.softAPgetStationNum(),
                  staIP.c_str());
  }

  delay(50);
}
