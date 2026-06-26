/*
  ESP32 BLE WiFi Provisioning + STM32 UART JSON + Chunked Cloud Upload

  - BLE GATT WiFi provisioning using CBOR credentials
  - Stores WiFi credentials in Preferences
  - GATT server stays active, advertising restarts on connect/disconnect
  - Improved stable WiFi reconnect logic
  - Event-driven WiFi handling
  - NTP sync after WiFi connects
  - UART receives STM32 session_end JSON
  - Builds full final session JSON
  - Uploads JSON using 3-phase HTTPS PUT:
      1) Init
      2) Append
      3) Complete
*/

/*
  WiFi reliability improvements (ESP32-C3 specific)

  Changes made:
  - Replaced aggressive WiFi.disconnect(true,true) with safer soft disconnect
  - Added event-driven WiFi handling using WiFi.onEvent()
  - Prevented overlapping WiFi.begin() calls while already connecting
  - Added proper 15s connection timeout handling
  - Enabled WiFi auto reconnect
  - Disabled WiFi persistent flash writes
  - Disabled WiFi sleep for better BLE + TLS stability
  - Reduced TX power slightly for more stable RF behavior
  - Added hostname before DHCP/connect
  - Removed polling-based connection detection

  Purpose:
  Improve connection reliability on ESP32-C3 Super Mini,
  prevent "sta is connecting, cannot set config" errors,
  reduce reconnect race conditions,
  and improve long-term WiFi stability.
*/


#include <WiFi.h>
#include <Preferences.h>
#include <NimBLEDevice.h>
#include "time.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "mbedtls/sha1.h"
#include "mbedtls/md5.h"
#include <Arduino.h>

// ---------- UART ----------
#define RX_PIN 4
#define TX_PIN 5
#define UART_BAUD 921600
HardwareSerial MyUART(1);

// ---------- BLE UUIDs ----------
static const char* WIFI_SERVICE_UUID  = "e46a6e40-1008-4001-1333-df45c65ae082";
static const char* WIFI_SETTINGS_UUID = "e46a6e41-1008-4001-1333-df45c65ae082";
static const char* WIFI_CONN_UUID     = "e46a6e42-1008-4001-1333-df45c65ae082";

// ---------- BLE ----------
NimBLECharacteristic* chSettings = nullptr;
NimBLECharacteristic* chConn = nullptr;
NimBLEServer* bleServer = nullptr;

// ---------- NVS ----------
Preferences prefs;

// ---------- Device Info ----------
const char* SENSOR_ID = "en-1sensor-2f7a9c1e4b3d6058-6a1d9c4e2b3f5087";
const char* ASSIGNED_ID = "00001";
const char* STORE_ID = "288140a4-c756-4137-9ee6-1988a5c5779a";
const char* CAMPAIGN_ID = "CAMP-SUMMER26";

// ---------- Cloud ----------
const char* CLOUD_FILES_BASE = "https://maysam-api.vusionrail.com/hub/v1/files/";
const size_t CHUNK_SIZE = 1024;
const char* CONTENT_TYPE = "application/vusion.telemetry.sensor";
const char* API_KEY = "a0da378ee03d4ff1a8dd165616f30db7";

// ---------- NTP ----------
const char* NTP_SERVER = "20.113.173.199";
const long GMT_OFFSET_SEC = 0;
const int DAYLIGHT_OFFSET_SEC = 0;
bool ntpSynced = false;

// ---------- App Status ----------
enum Status {
  NOT_CONFIGURED = 0,
  WAITING        = 1,
  CONNECTING     = 2,
  CONNECTED      = 3,
  FAILED         = 4
};

uint8_t currentStatus = NOT_CONFIGURED;

// ---------- WiFi State ----------
bool pendingWifiConnect = false;
String pendingSsid = "";
String pendingPass = "";

int retryCount = 0;
const int MAX_RETRIES = 3;

unsigned long nextAttemptMs = 0;
unsigned long connectStartMs = 0;

// Allow router enough time before retrying connection
const unsigned long WIFI_CONNECT_TIMEOUT = 15000;

bool wifiConnected = false;

// ---------- Session State ----------
String latestJSON = "{}";
String latestRaw = "{}";
int rawSize = 0;
int realSessionSize = 0;

int sessionCounter = 0;
String lastSessionDate = "";

// ============================================================================
// STATUS
// ============================================================================
void setStatus(uint8_t s, bool notify = true) {

  currentStatus = s;

  Serial.print("APP STATUS: ");

  switch (s) {
    case NOT_CONFIGURED: Serial.println("NOT_CONFIGURED"); break;
    case WAITING:        Serial.println("WAITING"); break;
    case CONNECTING:     Serial.println("CONNECTING"); break;
    case CONNECTED:      Serial.println("CONNECTED"); break;
    case FAILED:         Serial.println("FAILED"); break;
  }

  if (chConn) {

    chConn->setValue(&currentStatus, 1);

    if (notify) {
      Serial.println("Sending ConnectionDetails INDICATE");
      chConn->indicate();
    }
  }
}

// ============================================================================
// WIFI EVENTS
// ============================================================================

// Event-based WiFi state handling
void WiFiEvent(WiFiEvent_t event) {

  switch(event) {

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("EVENT: STA CONNECTED");
      break;

    // WiFi connected and IP acquired
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:

      Serial.println("EVENT: GOT IP");
      Serial.println(WiFi.localIP());

      wifiConnected = true;
      retryCount = 0;

      setStatus(CONNECTED);

      syncNTP();

      break;

    // WiFi disconnected -> restart reconnect cycle
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:

      Serial.println("EVENT: DISCONNECTED");

      wifiConnected = false;
      ntpSynced = false;

      if (retryCount < MAX_RETRIES) {
        pendingWifiConnect = true;
        nextAttemptMs = millis() + 3000;
      }

      break;

    default:
      break;
  }
}

// ============================================================================
// BLE ADVERTISING
// ============================================================================
void startAdvertisingSafe() {

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();

  if (!adv) return;

  adv->start();

  Serial.println("BLE advertising started/restarted");
}

class ServerCallbacks : public NimBLEServerCallbacks {

public:

  void onConnect(NimBLEServer* server) {
    Serial.println("BLE client connected");
    delay(100);
    startAdvertisingSafe();
  }

  void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) {
    (void)connInfo;
    onConnect(server);
  }

  void onDisconnect(NimBLEServer* server) {
    Serial.println("BLE client disconnected");
    delay(100);
    startAdvertisingSafe();
  }

  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) {
    (void)connInfo;
    (void)reason;
    onDisconnect(server);
  }
};

// ============================================================================
// CBOR PARSER
// ============================================================================
bool parseWifiCbor(const uint8_t* buf, size_t len,
                   uint8_t& auth,
                   String& ssid,
                   String& pass) {

  size_t i = 0;

  auth = 0;
  ssid = "";
  pass = "";

  if (len < 1) return false;
  if ((buf[i++] >> 5) != 5) return false;

  while (i < len) {

    if (i >= len) return false;
    if ((buf[i] >> 5) != 3) return false;

    int keyLen = buf[i++] & 0x1F;

    if (i + keyLen > len) return false;

    String key = "";

    for (int j = 0; j < keyLen; j++) {
      key += (char)buf[i++];
    }

    if (key == "auth") {

      if (i >= len) return false;

      auth = buf[i++];
    }
    else if (key == "ssid") {

      if (i >= len) return false;

      int l = buf[i++] & 0x1F;

      if (i + l > len) return false;

      for (int j = 0; j < l; j++) {
        ssid += (char)buf[i++];
      }
    }
    else if (key == "psk") {

      if (i >= len) return false;

      int l = buf[i++] & 0x1F;

      if (i + l > len) return false;

      for (int j = 0; j < l; j++) {
        pass += (char)buf[i++];
      }
    }
    else {
      Serial.println("Unknown CBOR key");
      return false;
    }
  }

  if (auth != 1 && auth != 2) return false;
  if (ssid.length() == 0) return false;

  return true;
}

// ============================================================================
// BLE SETTINGS CALLBACK
// ============================================================================
class SettingsCallback : public NimBLECharacteristicCallbacks {

  void handleWrite(NimBLECharacteristic* c) {

    Serial.println("BLE WRITE TRIGGERED");

    std::string rx = c->getValue();

    Serial.printf("RAW LEN: %d\n", rx.length());

    if (rx.empty()) return;

    uint8_t auth = 0;
    String ssid = "";
    String pass = "";

    if (!parseWifiCbor((const uint8_t*)rx.data(),
                       rx.size(),
                       auth,
                       ssid,
                       pass))
    {
      Serial.println("CBOR parse FAILED");
      setStatus(FAILED);
      return;
    }

    Serial.println("==== CBOR DECODED ====");
    Serial.print("AUTH: ");
    Serial.println(auth);
    Serial.println("SSID: " + ssid);
    Serial.println("PSK : " + pass);
    Serial.println("======================");

    prefs.begin("wifi", false);
    prefs.putUChar("auth", auth);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();

    Serial.println("Sending Settings INDICATE / echo raw CBOR");

    c->setValue((uint8_t*)rx.data(), rx.length());
    c->indicate();

    pendingSsid = ssid;
    pendingPass = pass;

    retryCount = 0;
    nextAttemptMs = millis();

    wifiConnected = false;
    ntpSynced = false;

    pendingWifiConnect = true;
  }

public:

  void onWrite(NimBLECharacteristic* c) {
    handleWrite(c);
  }

  void onWrite(NimBLECharacteristic* c,
               NimBLEConnInfo& ci)
  {
    (void)ci;
    handleWrite(c);
  }
};

// ============================================================================
// BLE SETUP
// ============================================================================
void setupBLE() {

  NimBLEDevice::init("MotionSensor1");

  String mac = NimBLEDevice::getAddress().toString().c_str();
  mac.toUpperCase();

  Serial.println("SENSOR_ID: " + String(SENSOR_ID));
  Serial.println("BLE_MAC: " + mac);

  bleServer = NimBLEDevice::createServer();
  bleServer->setCallbacks(new ServerCallbacks());

  NimBLEService* svc = bleServer->createService(WIFI_SERVICE_UUID);

  chSettings = svc->createCharacteristic(
    WIFI_SETTINGS_UUID,
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::WRITE |
    NIMBLE_PROPERTY::INDICATE
  );

  chSettings->setCallbacks(new SettingsCallback());

  auto d1 = chSettings->createDescriptor(
    "2901",
    NIMBLE_PROPERTY::READ,
    32
  );

  d1->setValue("Settings");

  chConn = svc->createCharacteristic(
    WIFI_CONN_UUID,
    NIMBLE_PROPERTY::READ |
    NIMBLE_PROPERTY::INDICATE
  );

  auto d2 = chConn->createDescriptor(
    "2901",
    NIMBLE_PROPERTY::READ,
    32
  );

  d2->setValue("Connection Details");

  svc->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();

  adv->addServiceUUID(WIFI_SERVICE_UUID);
  adv->setName("MotionSensor1");

  adv->setMinInterval(800);
  adv->setMaxInterval(1600);

  startAdvertisingSafe();
}

// ============================================================================
// TIME HELPERS
// ============================================================================
String iso8601(time_t t, int ms = 0) {

  struct tm tm_utc;

  gmtime_r(&t, &tm_utc);

  char buf[40];

  snprintf(buf,
           sizeof(buf),
           "%04d-%02d-%02dT%02d:%02d:%02d",
           tm_utc.tm_year + 1900,
           tm_utc.tm_mon + 1,
           tm_utc.tm_mday,
           tm_utc.tm_hour,
           tm_utc.tm_min,
           tm_utc.tm_sec);

  String s(buf);

  if (ms > 0) {

    char m[8];

    snprintf(m, sizeof(m), ".%03d", ms);

    s += m;
  }

  s += "Z";

  return s;
}

String makeSessionID(time_t nowT) {

  struct tm tm_utc;

  gmtime_r(&nowT, &tm_utc);

  char dateBuf[16];

  snprintf(dateBuf,
           sizeof(dateBuf),
           "%04d%02d%02d",
           tm_utc.tm_year + 1900,
           tm_utc.tm_mon + 1,
           tm_utc.tm_mday);

  String currentDate = String(dateBuf);

  if (currentDate != lastSessionDate) {
    sessionCounter = 0;
    lastSessionDate = currentDate;
  }

  sessionCounter++;

  return "sen_" + currentDate + "_" + String(sessionCounter);
}

void syncNTP() {

  if (ntpSynced) return;

  Serial.println("NTP syncing...");

  configTime(GMT_OFFSET_SEC,
             DAYLIGHT_OFFSET_SEC,
             NTP_SERVER);

  for (int i = 0; i < 20; i++) {

    if (time(nullptr) > 1700000000) {

      Serial.println("NTP OK");

      ntpSynced = true;

      return;
    }

    delay(250);
  }

  Serial.println("NTP timeout");
}

// ============================================================================
// HASH HELPERS
// ============================================================================
String sha1Hex(const uint8_t* data, size_t len) {

  unsigned char hash[20];

  mbedtls_sha1(data, len, hash);

  char hex[41];

  for (int i = 0; i < 20; i++) {
    sprintf(hex + i * 2, "%02x", hash[i]);
  }

  hex[40] = '\0';

  return String(hex);
}

String md5Hex(const uint8_t* data, size_t len) {

  unsigned char hash[16];

  mbedtls_md5(data, len, hash);

  char hex[33];

  for (int i = 0; i < 16; i++) {
    sprintf(hex + i * 2, "%02x", hash[i]);
  }

  hex[32] = '\0';

  return String(hex);
}

// ============================================================================
// WIFI HELPERS
// ============================================================================
bool wifiBusyConnecting() {

  wl_status_t s = WiFi.status();

  return (s == WL_IDLE_STATUS);
}

// ============================================================================
// WIFI ATTEMPT
// ============================================================================
void startWifiAttempt() {

  Serial.println("Starting WiFi attempt...");
  Serial.println("Connecting to SSID: " + pendingSsid);

  WiFi.mode(WIFI_STA);

// Disable modem sleep for better BLE + WiFi stability
  WiFi.setSleep(false);

// Set DHCP hostname before connecting
  WiFi.setHostname("MotionSensor1");

// Slightly reduced TX power improves stability on some ESP32-C3 boards
  WiFi.setTxPower(WIFI_POWER_11dBm);

// Soft disconnect only (avoid full STA teardown race conditions)
  WiFi.disconnect(false, false);

  delay(100);

  WiFi.begin(pendingSsid.c_str(),
             pendingPass.c_str());

// Track connect attempt start time for timeout handling
  connectStartMs = millis();

  setStatus(CONNECTING);
}

// ============================================================================
// CLOUD UPLOAD
// ============================================================================
bool uploadJsonAsChunks(const String& jsonStr,
                        const String& fileId,
                        const String& fullSha1_expected)
{
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  String fixedJson = jsonStr;
  fixedJson.trim();
  fixedJson += "\r\n";

  const uint8_t* data = (const uint8_t*)fixedJson.c_str();
  const size_t totalSize = fixedJson.length();

  String url = String(CLOUD_FILES_BASE) + fileId;

  String fullSha1 = sha1Hex(data, totalSize);

  // INIT
  if (!http.begin(client, url)) return false;

  http.addHeader("X-Device-Id", SENSOR_ID);
  http.addHeader("X-Assigned-Id", ASSIGNED_ID);
  http.addHeader("X-Registered-Store-Id", STORE_ID);
  http.addHeader("Content-Type", CONTENT_TYPE);
  http.addHeader("Content-Length", "0");
  http.addHeader("X-Api-Key", API_KEY);

  int code = http.PUT("");

  http.end();

  Serial.printf("[CLOUD] Init HTTP %d\n", code);

  if (!(code == 200 || code == 201)) return false;

  // APPEND
  size_t offset = 0;

  while (offset < totalSize) {

    size_t chunkLen = min(CHUNK_SIZE,
                          totalSize - offset);

    size_t start = offset;
    size_t end = offset + chunkLen - 1;

    String chunkSha1 = sha1Hex(data + start,
                               chunkLen);

    String rangeHeader =
      "bytes=" + String(start) + "-" + String(end);

    if (!http.begin(client, url)) return false;

    http.addHeader("X-Device-Id", SENSOR_ID);
    http.addHeader("X-Assigned-Id", ASSIGNED_ID);
    http.addHeader("X-Registered-Store-Id", STORE_ID);
    http.addHeader("Content-Type", CONTENT_TYPE);
    http.addHeader("X-Upload-Action", "append");
    http.addHeader("Content-Range", rangeHeader);
    http.addHeader("X-SHA1-Checksum", chunkSha1);
    http.addHeader("X-Api-Key", API_KEY);

    int c = http.sendRequest(
      "PUT",
      (uint8_t*)(data + start),
      chunkLen
    );

    http.end();

    Serial.printf("[CLOUD] Append HTTP %d\n", c);

    if (!(c == 200 || c == 201 || c == 204))
      return false;

    offset += chunkLen;
  }

  // COMPLETE
  if (!http.begin(client, url)) return false;

  http.addHeader("X-Device-Id", SENSOR_ID);
  http.addHeader("X-Assigned-Id", ASSIGNED_ID);
  http.addHeader("X-Registered-Store-Id", STORE_ID);
  http.addHeader("Content-Type", CONTENT_TYPE);
  http.addHeader("X-Upload-Action", "complete");
  http.addHeader("Content-Length", "0");
  http.addHeader("X-SHA1-Checksum", fullSha1);
  http.addHeader("X-Api-Key", API_KEY);

  int codeFinal = http.PUT("");

  http.end();

  Serial.printf("[CLOUD] Complete HTTP %d\n", codeFinal);

  return (codeFinal >= 200 && codeFinal < 300);
}


// ============================================================================
// ESP32 STATUS HELPERS
// ============================================================================
int getWifiRSSI() {
  return (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : -127;
}

float getCpuTempC() {
  return temperatureRead();
}


// HANDLE STM32 SESSION_END — FULL JSON BUILDER
// ============================================================================
void handleSummaryLine(const String& line) {
  if (!line.startsWith("{") || line.indexOf("\"event\":\"session_end\"") < 0)
    return;

  latestRaw = line;
  rawSize = line.length();

  Serial.println("\n[STM32] Received session_end:");
  Serial.println(line);

  auto getInt = [&](const char* key) -> long {
    int p = line.indexOf(key);
    if (p < 0) return 0;

    p = line.indexOf(':', p);
    if (p < 0) return 0;

    int q1 = line.indexOf(',', p + 1);
    int q2 = line.indexOf('}', p + 1);
    int q = (q1 > 0 && q1 < q2) ? q1 : q2;

    if (q < 0) q = line.length();

    return line.substring(p + 1, q).toInt();
  };

  long total_ms = getInt("\"total_ms\"");
  int final_zone = (int)getInt("\"final_zone\"");

  struct Z {
    int idx;
    int entered;
    long first_ms;
    long dwell_ms;
  } Zs[3];

  for (int i = 0; i < 3; i++) Zs[i] = { i + 1, 0, 0, 0 };

  int arrPos = line.indexOf("\"zones\":");

  if (arrPos >= 0) {
    int pos = arrPos;

    for (int k = 0; k < 3; k++) {
      int idxPos = line.indexOf("\"idx\":", pos);
      if (idxPos < 0) break;

      int entPos = line.indexOf("\"entered\":", idxPos);
      int femPos = line.indexOf("\"first_entry_ms\":", idxPos);
      int dPos = line.indexOf("\"dwell_ms\":", idxPos);

      if (entPos < 0 || femPos < 0 || dPos < 0) break;

      int endBrace = line.indexOf('}', dPos);
      int endComma = line.indexOf(',', dPos);
      int stop = (endComma > 0 && endComma < endBrace) ? endComma : endBrace;

      if (stop < 0) stop = line.length();

      Zs[k].idx = line.substring(idxPos + 6, line.indexOf(',', idxPos)).toInt();
      Zs[k].entered = line.substring(entPos + 10, line.indexOf(',', entPos)).toInt();
      Zs[k].first_ms = line.substring(femPos + 17, line.indexOf(',', femPos)).toInt();

      String raw = line.substring(dPos + 10, stop);
      raw.trim();

      String num = "";
      for (char ch : raw) {
        if ((ch >= '0' && ch <= '9') || ch == '-') num += ch;
      }

      Zs[k].dwell_ms = num.toInt();
      pos = stop + 1;
    }
  }

  struct timeval tv;
  gettimeofday(&tv, nullptr);

  time_t endSec = tv.tv_sec;
  int endMs = tv.tv_usec / 1000;

  long t_ms = (total_ms < 0) ? 0 : total_ms;

  time_t startSec = endSec - (t_ms / 1000);
  int startMs = endMs - (t_ms % 1000);

  if (startMs < 0) {
    startMs += 1000;
    startSec--;
  }

  String sessionID = makeSessionID(endSec);

  String out = "{";
  out += "\"sessionID\":\"" + sessionID + "\",";
  out += "\"sensorID\":\"" + String(SENSOR_ID) + "\",";
  out += "\"storeID\":\"" + String(STORE_ID) + "\",";
  out += "\"campaignID\":\"" + String(CAMPAIGN_ID) + "\",";
  out += "\"objectID\":\"obj" + sessionID.substring(sessionID.length() - 4) + "\",";
  out += "\"startTime\":\"" + iso8601(startSec, startMs) + "\",";
  out += "\"endTime\":\"" + iso8601(endSec, endMs) + "\",";
  out += "\"totalDuration\":" + String(((double)total_ms) / 1000.0, 2) + ",";
  out += "\"conversionEvent\":false,";

  out += "\"zones\":{";

  for (int i = 0; i < 3; i++) {
    if (i) out += ",";

    int z = Zs[i].idx;

    out += "\"zone" + String(z) + "\":{";
    out += "\"entered\":" + String(Zs[i].entered ? "true" : "false") + ",";

    if (Zs[i].entered) {
      long fem = Zs[i].first_ms;
      time_t femSec = startSec + fem / 1000;
      int femMs = startMs + (fem % 1000);

      if (femMs >= 1000) {
        femMs -= 1000;
        femSec++;
      }

      out += "\"entryTimestamp\":\"" + iso8601(femSec, femMs) + "\",";
    } else {
      out += "\"entryTimestamp\":null,";
    }

    out += "\"totalDwellTime\":" + String(((double)Zs[i].dwell_ms) / 1000.0, 2);
    out += "}";
  }

  out += "},";

  if (final_zone >= 1 && final_zone <= 3) {
    out += "\"finalState\":\"Exited Zone " + String(final_zone) + "\"";
  } else {
    out += "\"finalState\":\"Exited\",";
  }

  out += "\"wifiRSSI\":" + String(getWifiRSSI()) + ",";
  out += "\"cpuTemperature\":" + String(getCpuTempC(),1);

  out += "}";

  latestJSON = out;
  realSessionSize = latestJSON.length();

  Serial.println("\n===== FINAL JSON (Cloud) =====");
  Serial.println(latestJSON);

  const uint8_t* bytes = (const uint8_t*)latestJSON.c_str();
  const size_t totalLen = latestJSON.length();

  String fileId = md5Hex(bytes, totalLen);
  String fullSha1 = sha1Hex(bytes, totalLen);

  if (WiFi.status() == WL_CONNECTED) {
    bool ok = uploadJsonAsChunks(latestJSON, fileId, fullSha1);
    Serial.printf("[CLOUD] %s\n", ok ? "SUCCESS" : "FAILED");
  } else {
    Serial.println("[CLOUD] WiFi not connected, upload skipped.");
  }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {

  Serial.begin(115200);

  delay(500);

  Serial.println("\nBOOT");

  MyUART.begin(UART_BAUD,
               SERIAL_8N1,
               RX_PIN,
               TX_PIN);

// Prevent repeated flash writes / config corruption
  WiFi.persistent(false);

// Allow ESP32 internal reconnect handling
  WiFi.setAutoReconnect(true);

// Event-driven WiFi handling (more stable than polling)
  WiFi.onEvent(WiFiEvent);

  setupBLE();

  prefs.begin("wifi", true);

  pendingSsid = prefs.getString("ssid", "");
  pendingPass = prefs.getString("pass", "");

  prefs.end();

  if (pendingSsid.length() > 0) {

    Serial.println("Stored WiFi found");
    Serial.println("SSID: " + pendingSsid);

    retryCount = 0;

    nextAttemptMs = millis();

    pendingWifiConnect = true;
  }
  else {
    setStatus(WAITING);
  }
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {

  // START ATTEMPT (Prevent overlapping WiFi.begin() calls while already connecting)
  if (
      pendingWifiConnect &&
      millis() >= nextAttemptMs &&
      retryCount < MAX_RETRIES &&
      !wifiBusyConnecting()
     )
  {
    Serial.printf("Retry #%d\n", retryCount + 1);

    startWifiAttempt();

    retryCount++;

    pendingWifiConnect = false;
  }

  // CONNECT TIMEOUT (Retry connection only after timeout expires)
  if (!wifiConnected &&
      retryCount > 0 &&
      (millis() - connectStartMs > WIFI_CONNECT_TIMEOUT))
  {
    Serial.println("Connect timeout");

    WiFi.disconnect(false, false);

    if (retryCount < MAX_RETRIES) {

      pendingWifiConnect = true;

      nextAttemptMs = millis() + 3000;
    }
  }

  // MAX RETRIES
  if (!wifiConnected &&
      retryCount >= MAX_RETRIES)
  {
    Serial.println("Max retries reached -> WAITING");

    setStatus(WAITING);

    retryCount = 0;

    pendingWifiConnect = false;
  }

  // UART JSON
  static String buf;

  while (MyUART.available()) {

    char c = (char)MyUART.read();

    if (c == '\r')
      continue;

    if (c == '\n') {

      if (buf.length()) {

        handleSummaryLine(buf);

        buf = "";
      }
    }
    else {

      if (buf.length() < 2048)
        buf += c;
      else
        buf = "";
    }
  }

  delay(50);
}
