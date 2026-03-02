// ==========================================================
//   CITADEL v2.1.1 — CIT_V_2_1_1
//   Multi-file restructure — identical functionality
//   2025-02-28 00:00 UTC
//   Checksum: CLDTX6
//   File 00: Master .ino — Includes, Types, Globals, Forward Declarations
// ==========================================================

// Must be defined before #includes so Arduino auto-prototyping sees the type
enum HealthState { HEALTH_GREEN, HEALTH_YELLOW, HEALTH_ORANGE, HEALTH_RED };

#include <Wire.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <U8g2lib.h>
#include <WebServer.h>
#include <BleKeyboard.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <Update.h>
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
BleKeyboard* bleKbd   = nullptr;

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
String   runCommand(String raw);
void checkGithubOta();
void ghSaveToken(const String& token);