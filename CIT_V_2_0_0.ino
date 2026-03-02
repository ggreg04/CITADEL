// ==========================================================
//   CITADEL v2.2.7 — CIT_V_2_2_7
//   Bootloop fix — include ordering
//   2026-03-02 00:00 UTC
//   Checksum: CLDTX8
//   File 00: Master .ino — Includes, Types, Globals, Forward Declarations
// ==========================================================

// Must be defined before #includes so Arduino auto-prototyping sees the type
enum HealthState { HEALTH_GREEN, HEALTH_YELLOW, HEALTH_ORANGE, HEALTH_RED };

#include <Wire.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <U8g2lib.h>
#include <WebServer.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

// ── BLE HID key codes (replaces BleKeyboard.h) ──────────────────────────────
// Modifier keys — pressed via blePress(); map to HID modifier byte bits 0-7
#define KEY_LEFT_CTRL      0x80
#define KEY_LEFT_SHIFT     0x81
#define KEY_LEFT_ALT       0x82
#define KEY_LEFT_GUI       0x83
#define KEY_RIGHT_CTRL     0x84
#define KEY_RIGHT_SHIFT    0x85
#define KEY_RIGHT_ALT      0x86
#define KEY_RIGHT_GUI      0x87
// Special keyboard keys
#define KEY_RETURN         0xB0
#define KEY_ESC            0xB1
#define KEY_BACKSPACE      0xB2
#define KEY_TAB            0xB3
#define KEY_F1             0xC2
#define KEY_F2             0xC3
#define KEY_F3             0xC4
#define KEY_F4             0xC5
#define KEY_F5             0xC6
#define KEY_F6             0xC7
#define KEY_F7             0xC8
#define KEY_F8             0xC9
#define KEY_F9             0xCA
#define KEY_F10            0xCB
#define KEY_F11            0xCC
#define KEY_F12            0xCD
#define KEY_PRTSC          0xCE
#define KEY_INSERT         0xD1
#define KEY_HOME           0xD2
#define KEY_PAGE_UP        0xD3
#define KEY_DELETE         0xD4
#define KEY_END            0xD5
#define KEY_PAGE_DOWN      0xD6
#define KEY_RIGHT_ARROW    0xD7
#define KEY_LEFT_ARROW     0xD8
#define KEY_DOWN_ARROW     0xD9
#define KEY_UP_ARROW       0xDA
// Consumer Control keys — sent via separate HID report in bleWrite()
#define KEY_MEDIA_MUTE         0xE0
#define KEY_MEDIA_VOLUME_UP    0xE1
#define KEY_MEDIA_VOLUME_DOWN  0xE2
#define KEY_MEDIA_PLAY_PAUSE   0xE3
#define KEY_MEDIA_NEXT_TRACK   0xE4
#define KEY_MEDIA_PREV_TRACK   0xE5
// ────────────────────────────────────────────────────────────────────────────
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <Update.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"


//  Network 
Preferences prefs;
WebServer   server(80);

String apName       = "CITADEL";
String apPass       = "12345678";
String staSSID      = "";
String staPass      = "";
bool   staConnected = false;
bool   mdnsRunning  = false;
bool   sniffing     = false;

//  BLE
String btName         = "CITADEL";
bool   btActive       = false;
bool   btWasConnected = false;
unsigned long lastKeepAlive = 0;

//  Auth 
String authUser      = "";
String authPass      = "";
bool   authEnabled   = false;
int    authTimeout   = 30;
bool   authAutoRenew = true;
String sessionToken  = "";
unsigned long sessionStart = 0;

struct Account {
  String username;
  String password;
  String role;
  bool   locked;
};
#define MAX_ACCOUNTS 8
Account accounts[MAX_ACCOUNTS];
int     accountCount = 0;
String  currentUser  = "";

struct LoginAttempt {
  String ip;
  int    fails;
  unsigned long lockUntil;
};
LoginAttempt loginAttempts[8];
int loginAttemptCount = 0;

String authWhitelist[8];
int    whitelistCount   = 0;
bool   whitelistEnabled = false;
String loginLog         = "";

//  OTA 
bool   otaRunning    = false;
bool   otaEnabled    = false;
String otaPassword   = "citadel";
unsigned long otaEnabledTime = 0;

//  System 
int           currentFreq      = 240;
bool          thermalThrottled = false;
bool          emergencyKill    = false;
bool          oledFlipped      = false;
unsigned long bootTime         = 0;
bool          ghOtaChecked     = false;
unsigned long lastThermalCheck = 0;
unsigned long lastHealthCheck  = 0;
unsigned long lastWifiCheck    = 0;
String        crashReason      = "";
String        sysBanner        = "";
String        terminalLog      = "";
String        panelMode        = "STATUS";

float thermalHistory[60];
int   thermalHistoryIdx = 0;

//  NTP 
bool   ntpSynced = false;
int    ntpOffset = 0;

//  Health
int currentHealth = HEALTH_GREEN;
int lastHealth    = HEALTH_GREEN;

//  OLED 
enum OledMode {
  OLED_STATUS, OLED_TERMINAL, OLED_TERMINAL_ALERTS,
  OLED_CLOCK, OLED_MSG, OLED_SCROLL,
  OLED_HEALTH, OLED_ATTACK, OLED_NETWORK, OLED_OFF
};
OledMode oledMode      = OLED_STATUS;
String   oledCustomMsg = "";
String   oledScrollText = "";
int      oledScrollPos  = 0;
unsigned long lastOledUpdate = 0;
String   oledLastCmd   = "";
String   oledLastResp  = "";

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

//  Attack 
bool   evilTwinActive   = false;
bool   deauthActive     = false;
bool   deauthContinuous = false;
bool   beaconSpamActive = false;
bool   captureHandshake = false;
String evilTwinSSID     = "";
String capturedCreds    = "";
int    captureCount     = 0;
String attackLog        = "";
int    deauthPacketRate = 10;
String captiveTheme     = "auto";
bool   handshakeGot     = false;
String handshakeTarget  = "";
String handshakeHex     = "";

uint8_t deauthFrame[26] = {
  0xC0, 0x00, 0x3A, 0x01,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x07, 0x00
};

//  BT Payloads 
struct BtPayload {
  String name;
  String steps[32];
  int    delays[32];
  int    stepCount;
};
#define MAX_PAYLOADS 16
BtPayload payloads[MAX_PAYLOADS];
int       payloadCount     = 0;
bool      recordingPayload = false;
int       recordingIndex   = -1;

//  Aliases 
struct CmdAlias { String alias; String command; };
#define MAX_ALIASES 16
CmdAlias aliases[MAX_ALIASES];
int      aliasCount = 0;

//  Forward Declarations 
void     addLog(String entry);
void     addAttackLog(String entry);
void     killAllAttacks(bool emergency);
String   getUptime();
float    getTemp();
String   getRealTime();
int      getHealthState();
String   healthName(int h);
String   healthColor(int h);
void     stopBle();
void     startBle();
bool     bleIsConnected();
void     blePress(uint8_t key);
void     bleReleaseAll();
void     bleWrite(uint8_t key);
void     blePrint(const String& s);
String   runCommand(String raw);
void checkGithubOta();
void ghSaveToken(const String& token);