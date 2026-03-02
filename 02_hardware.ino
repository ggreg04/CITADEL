// ==========================================================
//   CITADEL v2.0.0 — CIT_V_2_0_0
//   File 02: Hardware — Health, Thermal, OLED, BLE
// ==========================================================

//  Health Monitor 
int getHealthState() {
  uint32_t heap = ESP.getFreeHeap();
  if (heap > 120000) return HEALTH_GREEN;
  if (heap > 80000)  return HEALTH_YELLOW;
  if (heap > 50000)  return HEALTH_ORANGE;
  return HEALTH_RED;
}

String healthName(int h) {
  switch(h) {
    case HEALTH_GREEN:  return "GREEN";
    case HEALTH_YELLOW: return "YELLOW";
    case HEALTH_ORANGE: return "ORANGE";
    case HEALTH_RED:    return "RED";
  }
  return "UNKNOWN";
}

String healthColor(int h) {
  switch(h) {
    case HEALTH_GREEN:  return "#00ff00";
    case HEALTH_YELLOW: return "#ffff00";
    case HEALTH_ORANGE: return "#ff8800";
    case HEALTH_RED:    return "#ff5555";
  }
  return "#ffffff";
}

void checkHealth() {
  if (millis() - lastHealthCheck < 3000) return;
  lastHealthCheck = millis();
  currentHealth = getHealthState();

  if (currentHealth != lastHealth) {
    addLog("[HEALTH] " + healthName(lastHealth) + "  " + healthName(currentHealth));
    lastHealth = currentHealth;
  }

  if (currentHealth == HEALTH_YELLOW) {
    static unsigned long lastWarn = 0;
    if (millis() - lastWarn > 30000) {
      lastWarn = millis();
      addLog("[!] YELLOW  Heap:" + String(ESP.getFreeHeap()/1024) +
             "KB. Consider stopping unused services.");
    }
  }

  if (currentHealth == HEALTH_ORANGE) {
    static unsigned long lastWarn = 0;
    if (millis() - lastWarn > 15000) {
      lastWarn = millis();
      addLog("[!!] ORANGE  Heap:" + String(ESP.getFreeHeap()/1024) +
             "KB. Run ATTACK STOP.");
      beaconSpamActive = false;
    }
  }

  if (currentHealth == HEALTH_RED) {
    addLog("[!!!] RED  EMERGENCY KILL");
    killAllAttacks(true);
  }
}

//  Kill All Attacks 
void killAllAttacks(bool emergency) {
  unsigned long t = millis();
  emergencyKill = true;
  String killed = "";

  if (beaconSpamActive)  { beaconSpamActive = false;  killed += "BEACON "; }
  if (deauthActive)      { deauthActive = false; deauthContinuous = false; killed += "DEAUTH "; }
  if (captureHandshake)  { captureHandshake = false;  killed += "HANDSHAKE "; }
  if (evilTwinActive)    { evilTwinActive = false; WiFi.softAP(apName.c_str(), apPass.c_str()); killed += "EVILTWIN "; }
  if (btActive && emergency) { stopBle(); killed += "BLE "; }

  panelMode = "STATUS";
  emergencyKill = false;

  addLog("[KILL] Stopped: " + (killed == "" ? "NONE" : killed));
  addLog("[KILL] Time: " + String(millis()-t) + "ms | Heap: " + String(ESP.getFreeHeap()/1024) + "KB | State: " + healthName(getHealthState()));
}

//  Thermal 
void checkThermal() {
  if (millis() - lastThermalCheck < 5000) return;
  lastThermalCheck = millis();
  float t = getTemp();
  thermalHistory[thermalHistoryIdx] = t;
  thermalHistoryIdx = (thermalHistoryIdx + 1) % 60;

  if (t > 75.0f && !thermalThrottled) {
    thermalThrottled = true;
    setClock(80);
    addLog("[THERMAL] THROTTLE ON (" + String(t,1) + "C)");
  } else if (t < 60.0f && thermalThrottled) {
    thermalThrottled = false;
    addLog("[THERMAL] THROTTLE OFF (" + String(t,1) + "C)");
  }
}

//  OLED
static const uint8_t skyline_bmp[] PROGMEM = {
  // 128x28 city skyline — 16 bytes/row, 28 rows, XBM format (LSB first)
  // y=0,1 — sky
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  // y=2,3 — center spire peak
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  // y=4-7 — flanking towers rise
  0x00,0x00,0x00,0x00,0x00,0xF0,0x0F,0xF0,0x0F,0xF0,0x0F,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xF0,0x0F,0xF0,0x0F,0xF0,0x0F,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xF0,0x0F,0xF0,0x0F,0xF0,0x0F,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xF0,0x0F,0xF0,0x0F,0xF0,0x0F,0x00,0x00,0x00,0x00,0x00,
  // y=8-11 — mid-rise buildings appear
  0x00,0x00,0x00,0xFF,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0xFF,0x00,0x00,0x00,
  0x00,0x00,0x00,0xFF,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0xFF,0x00,0x00,0x00,
  0x00,0x00,0x00,0xFF,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0xFF,0x00,0x00,0x00,
  0x00,0x00,0x00,0xFF,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0xFF,0x00,0x00,0x00,
  // y=12-15 — outer medium buildings appear
  0x00,0xFF,0x00,0xFF,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0xFF,0x00,0xFF,0x00,
  0x00,0xFF,0x00,0xFF,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0xFF,0x00,0xFF,0x00,
  0x00,0xFF,0x00,0xFF,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0xFF,0x00,0xFF,0x00,
  0x00,0xFF,0x00,0xFF,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0xFF,0x00,0xFF,0x00,
  // y=16-17 — side short buildings join
  0x00,0xFF,0xFF,0xFF,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0xFF,0xFF,0xFF,0x00,
  0x00,0xFF,0xFF,0xFF,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0x0F,0xF0,0xFF,0xFF,0xFF,0x00,
  // y=18 — inner low buildings join
  0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0x0F,0xF0,0x0F,0xF0,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,
  // y=19-27 — full ground
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
};

void drawSplashScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawXBM(0, 0, 128, 28, skyline_bmp);
  u8g2.drawStr(34, 40, "Welcome to");
  u8g2.drawStr(22, 54, "CITADEL v2.2.3");
  u8g2.sendBuffer();
}

void oledDraw(const String& l1, const String& l2, const String& l3) {
  if (oledMode == OLED_OFF) return;
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 12, l1.c_str());
  u8g2.drawStr(0, 32, l2.c_str());
  u8g2.drawStr(0, 52, l3.c_str());
  u8g2.sendBuffer();
}

void updateOled() {
  if (otaRunning) return;
  unsigned long interval = 1000;
  if (oledMode == OLED_HEALTH || oledMode == OLED_ATTACK) interval = 2000;
  if (oledMode == OLED_NETWORK) interval = 5000;
  if (millis() - lastOledUpdate < interval) return;
  lastOledUpdate = millis();
  if (oledMode == OLED_OFF) return;

  switch (oledMode) {
    case OLED_STATUS: {
      wifi_sta_list_t sl; esp_wifi_ap_get_sta_list(&sl);
      String l2 = staConnected
        ? ("STA: " + staSSID + "@" + WiFi.localIP().toString())
        : "STA: Ready to connect";
      String l3 = (sl.num == 0)
        ? ("A:" + apName + " " + WiFi.softAPIP().toString())
        : ("AP: " + String(sl.num) + " dev connected");
      oledDraw("CITADEL v2.1.0", l2.substring(0,21), l3.substring(0,21));
      break;
    }
    case OLED_TERMINAL:
    case OLED_TERMINAL_ALERTS:
      oledDraw("[T] " + oledLastCmd.substring(0,16), oledLastResp.substring(0,21), "");
      break;
    case OLED_CLOCK:
      oledDraw(getRealTime(), "UP:" + getUptime(), String(getTemp(),1) + "C " + String(currentFreq) + "MHz");
      break;
    case OLED_MSG:
      oledDraw(oledCustomMsg.substring(0,21),
               oledCustomMsg.length()>21 ? oledCustomMsg.substring(21,42) : "",
               oledCustomMsg.length()>42 ? oledCustomMsg.substring(42,63) : "");
      break;
    case OLED_SCROLL: {
      if (oledScrollText.length() == 0) break;
      String padded = oledScrollText + "     ";
      int len = padded.length();
      String vis = "";
      for (int i = 0; i < 21; i++) vis += padded[(oledScrollPos+i)%len];
      oledScrollPos = (oledScrollPos+1)%len;
      oledDraw("", vis, "");
      break;
    }
    case OLED_HEALTH:
      oledDraw("HEALTH:" + healthName(currentHealth),
               "HEAP:" + String(ESP.getFreeHeap()/1024) + "KB",
               "T:" + String(getTemp(),1) + "C");
      break;
    case OLED_ATTACK: {
      String et  = evilTwinActive ? "ET:" + evilTwinSSID.substring(0,10) : "ET:OFF";
      String cap = "CAPS:" + String(captureCount) + " DA:" + (deauthActive?"ON":"OFF");
      oledDraw(et, cap, "BCN:" + String(beaconSpamActive?"ON":"OFF"));
      break;
    }
    case OLED_NETWORK: {
      wifi_sta_list_t sl; esp_wifi_ap_get_sta_list(&sl);
      oledDraw("AP:" + WiFi.softAPIP().toString(),
               staConnected ? "ST:" + WiFi.localIP().toString() : "STA:OFF",
               "CLI:" + String(sl.num));
      break;
    }
    default: break;
  }
}

//  BLE (NimBLE-Arduino — BLE-only, no Classic BT, lower RAM, WiFi-safe)

// ── NimBLE state (private to this file) ─────────────────────────────────────
static NimBLEServer*         nimServer    = nullptr;
static NimBLEHIDDevice*      nimHid       = nullptr;
static NimBLECharacteristic* nimInput     = nullptr;  // keyboard report
static NimBLECharacteristic* nimConsumer  = nullptr;  // consumer control report
static volatile bool         nimConnected = false;

// HID keyboard state
static uint8_t nimModifiers    = 0;
static uint8_t nimKeys[6]      = {0};  // up to 6 simultaneous key codes

// ── HID Report Descriptor ────────────────────────────────────────────────────
// Report ID 1 : keyboard (8 bytes)
// Report ID 2 : consumer control (2 bytes)
static const uint8_t _hidDesc[] = {
  // Keyboard — Report ID 1
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x06,        // Usage (Keyboard)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID 1
  0x05, 0x07,        //   Usage Page (Key Codes)
  0x19, 0xE0,        //   Usage Min (Left Ctrl = 224)
  0x29, 0xE7,        //   Usage Max (Right GUI = 231)
  0x15, 0x00,        //   Logical Min (0)
  0x25, 0x01,        //   Logical Max (1)
  0x75, 0x01,        //   Report Size (1 bit)
  0x95, 0x08,        //   Report Count (8) — modifier byte
  0x81, 0x02,        //   Input (Data, Variable, Absolute)
  0x95, 0x01,        //   Report Count (1)
  0x75, 0x08,        //   Report Size (8) — reserved byte
  0x81, 0x01,        //   Input (Constant)
  0x95, 0x06,        //   Report Count (6)
  0x75, 0x08,        //   Report Size (8) — 6 key-code slots
  0x15, 0x00,        //   Logical Min (0)
  0x25, 0x73,        //   Logical Max (115)
  0x05, 0x07,        //   Usage Page (Key Codes)
  0x19, 0x00,        //   Usage Min (0)
  0x29, 0x73,        //   Usage Max (115)
  0x81, 0x00,        //   Input (Data, Array)
  0xC0,              // End Collection
  // Consumer Control — Report ID 2
  0x05, 0x0C,        // Usage Page (Consumer)
  0x09, 0x01,        // Usage (Consumer Control)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x02,        //   Report ID 2
  0x15, 0x00,        //   Logical Min (0)
  0x26, 0xFF, 0x03,  //   Logical Max (1023)
  0x19, 0x00,        //   Usage Min (0)
  0x2A, 0xFF, 0x03,  //   Usage Max (1023)
  0x75, 0x10,        //   Report Size (16 bits)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x00,        //   Input (Data, Array)
  0xC0,              // End Collection
};

// ── Connection callbacks ─────────────────────────────────────────────────────
class NimBleKbdServer : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* s)    { nimConnected = true;  }
  void onDisconnect(NimBLEServer* s) {
    nimConnected = false;
    if (btActive) NimBLEDevice::startAdvertising();
  }
};
static NimBleKbdServer _kbdCb;

// ── ASCII → HID keycode ──────────────────────────────────────────────────────
// Returns HID key code; sets *sh=true when LEFT_SHIFT modifier is needed.
static uint8_t asciiToHid(uint8_t c, bool* sh) {
  *sh = false;
  if (c >= 'a' && c <= 'z') return 0x04 + (c - 'a');
  if (c >= 'A' && c <= 'Z') { *sh = true; return 0x04 + (c - 'A'); }
  if (c >= '2' && c <= '9') return 0x1F + (c - '2');
  if (c == '1') return 0x1E;
  if (c == '0') return 0x27;
  switch (c) {
    case ' ':  return 0x2C;
    case '\n': return 0x28;
    case '\t': return 0x2B;
    case '-':  return 0x2D;
    case '=':  return 0x2E;
    case '[':  return 0x2F;
    case ']':  return 0x30;
    case '\\': return 0x31;
    case ';':  return 0x33;
    case '\'': return 0x34;
    case '`':  return 0x35;
    case ',':  return 0x36;
    case '.':  return 0x37;
    case '/':  return 0x38;
    case '!':  *sh=true; return 0x1E;
    case '@':  *sh=true; return 0x1F;
    case '#':  *sh=true; return 0x20;
    case '$':  *sh=true; return 0x21;
    case '%':  *sh=true; return 0x22;
    case '^':  *sh=true; return 0x23;
    case '&':  *sh=true; return 0x24;
    case '*':  *sh=true; return 0x25;
    case '(':  *sh=true; return 0x26;
    case ')':  *sh=true; return 0x27;
    case '_':  *sh=true; return 0x2D;
    case '+':  *sh=true; return 0x2E;
    case '{':  *sh=true; return 0x2F;
    case '}':  *sh=true; return 0x30;
    case '|':  *sh=true; return 0x31;
    case ':':  *sh=true; return 0x33;
    case '"':  *sh=true; return 0x34;
    case '~':  *sh=true; return 0x35;
    case '<':  *sh=true; return 0x36;
    case '>':  *sh=true; return 0x37;
    case '?':  *sh=true; return 0x38;
    default:   return 0;
  }
}

// Special key constant (0x80–0xDA) → HID keycode
static uint8_t specialToHid(uint8_t k) {
  switch (k) {
    case 0xB0: return 0x28; // KEY_RETURN
    case 0xB1: return 0x29; // KEY_ESC
    case 0xB2: return 0x2A; // KEY_BACKSPACE
    case 0xB3: return 0x2B; // KEY_TAB
    case 0xC2: return 0x3A; // KEY_F1
    case 0xC3: return 0x3B; case 0xC4: return 0x3C;
    case 0xC5: return 0x3D; case 0xC6: return 0x3E;
    case 0xC7: return 0x3F; case 0xC8: return 0x40;
    case 0xC9: return 0x41; case 0xCA: return 0x42;
    case 0xCB: return 0x43; case 0xCC: return 0x44;
    case 0xCD: return 0x45; // KEY_F12
    case 0xCE: return 0x46; // KEY_PRTSC
    case 0xD1: return 0x49; // KEY_INSERT
    case 0xD2: return 0x4A; // KEY_HOME
    case 0xD3: return 0x4B; // KEY_PAGE_UP
    case 0xD4: return 0x4C; // KEY_DELETE
    case 0xD5: return 0x4D; // KEY_END
    case 0xD6: return 0x4E; // KEY_PAGE_DOWN
    case 0xD7: return 0x4F; // KEY_RIGHT_ARROW
    case 0xD8: return 0x50; // KEY_LEFT_ARROW
    case 0xD9: return 0x51; // KEY_DOWN_ARROW
    case 0xDA: return 0x52; // KEY_UP_ARROW
    default:   return 0;
  }
}

// ── Report send helpers ──────────────────────────────────────────────────────
static void sendKbdReport() {
  if (!nimInput) return;
  uint8_t r[8] = {nimModifiers, 0,
                  nimKeys[0], nimKeys[1], nimKeys[2],
                  nimKeys[3], nimKeys[4], nimKeys[5]};
  nimInput->setValue(r, 8);
  nimInput->notify();
}

static void sendConsumerReport(uint8_t key) {
  if (!nimConsumer) return;
  uint16_t usage = 0;
  switch (key) {
    case KEY_MEDIA_MUTE:        usage = 0x00E2; break;
    case KEY_MEDIA_VOLUME_UP:   usage = 0x00E9; break;
    case KEY_MEDIA_VOLUME_DOWN: usage = 0x00EA; break;
    case KEY_MEDIA_PLAY_PAUSE:  usage = 0x00CD; break;
    case KEY_MEDIA_NEXT_TRACK:  usage = 0x00B5; break;
    case KEY_MEDIA_PREV_TRACK:  usage = 0x00B6; break;
  }
  if (!usage) return;
  uint8_t r[2] = {(uint8_t)(usage & 0xFF), (uint8_t)(usage >> 8)};
  nimConsumer->setValue(r, 2);
  nimConsumer->notify();
  delay(20);
  uint8_t rel[2] = {0, 0};
  nimConsumer->setValue(rel, 2);
  nimConsumer->notify();
}

// ── Public wrappers (called by 04_commands.ino / 06_server.ino) ─────────────
bool bleIsConnected() { return btActive && nimConnected; }

void blePress(uint8_t key) {
  if (!bleIsConnected()) return;
  if (key >= 0x80 && key <= 0x87) {          // modifier key
    nimModifiers |= (1 << (key - 0x80));
    sendKbdReport();
    return;
  }
  if (key >= 0xE0) return;                   // media — use bleWrite()
  bool sh = false;
  uint8_t hid = (key >= 0x80) ? specialToHid(key) : asciiToHid(key, &sh);
  if (!hid) return;
  if (sh) nimModifiers |= 0x02;
  for (int i = 0; i < 6; i++) {
    if (!nimKeys[i]) { nimKeys[i] = hid; break; }
  }
  sendKbdReport();
}

void bleReleaseAll() {
  nimModifiers = 0;
  memset(nimKeys, 0, 6);
  sendKbdReport();
}

void bleWrite(uint8_t key) {
  if (!bleIsConnected()) return;
  if (key >= 0xE0) { sendConsumerReport(key); return; }
  bool sh = false;
  uint8_t hid = (key >= 0x80) ? specialToHid(key) : asciiToHid(key, &sh);
  if (!hid) return;
  uint8_t savedMods = nimModifiers;
  if (sh) nimModifiers |= 0x02;
  nimKeys[0] = hid;
  sendKbdReport();
  delay(10);
  nimKeys[0] = 0;
  nimModifiers = savedMods;
  sendKbdReport();
}

void blePrint(const String& s) {
  for (int i = 0; i < (int)s.length(); i++) {
    bleWrite((uint8_t)s[i]);
    delay(5);
  }
}

// ── Lifecycle ────────────────────────────────────────────────────────────────
void startBle() {
  if (ESP.getFreeHeap() < 60000)
    addLog("[BT] WARN: Low heap (" + String(ESP.getFreeHeap()/1024) + "KB). May be unstable.");
  if (btActive) {
    NimBLEDevice::deinit(false);
    nimServer = nullptr; nimHid = nullptr;
    nimInput = nullptr; nimConsumer = nullptr;
    nimConnected = false;
    delay(200);
  }
  NimBLEDevice::init(btName.c_str());
  nimServer = NimBLEDevice::createServer();
  nimServer->setCallbacks(&_kbdCb, false);
  nimHid      = new NimBLEHIDDevice(nimServer);
  nimInput    = nimHid->inputReport(1);
  nimConsumer = nimHid->inputReport(2);
  nimHid->manufacturer()->setValue("CITADEL");
  nimHid->pnp(0x02, 0x05AC, 0x820A, 0x0210);
  nimHid->hidInfo(0x00, 0x01);
  nimHid->reportMap((uint8_t*)_hidDesc, sizeof(_hidDesc));
  nimHid->startServices();
  nimHid->setBatteryLevel(100);
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setAppearance(0x03C1);  // HID keyboard
  adv->addServiceUUID(nimHid->hidService()->getUUID());
  adv->setScanResponse(false);
  NimBLEDevice::startAdvertising();
  btActive = true;
  addLog("[BT] NimBLE started as \"" + btName + "\" Heap:" + String(ESP.getFreeHeap()/1024) + "KB");
}

void stopBle() {
  if (btActive) {
    NimBLEDevice::deinit(false);
    nimServer = nullptr; nimHid = nullptr;
    nimInput = nullptr; nimConsumer = nullptr;
    nimConnected = false;
  }
  btActive = false;
  btWasConnected = false;
  addLog("[BT] Stopped. Heap:" + String(ESP.getFreeHeap()/1024) + "KB");
}

int btTypingDelay(const String& speed) {
  if (speed == "SLOW")    return 120;
  if (speed == "FAST")    return 20;
  if (speed == "INSTANT") return 0;
  return 50;
}

void btApplyStyle(const String& style) {
  if (!bleIsConnected()) return;
  if (style == "BOLD") {
    blePress(KEY_LEFT_CTRL); blePress('b'); delay(80); bleReleaseAll();
  } else if (style == "ITALIC") {
    blePress(KEY_LEFT_CTRL); blePress('i'); delay(80); bleReleaseAll();
  } else if (style == "BOLD+ITALIC") {
    blePress(KEY_LEFT_CTRL); blePress('b'); delay(80); bleReleaseAll();
    blePress(KEY_LEFT_CTRL); blePress('i'); delay(80); bleReleaseAll();
  }
}

void btOpenNotepad(bool maximize, bool minimizeFirst) {
  if (!bleIsConnected()) return;
  if (minimizeFirst) {
    blePress(KEY_LEFT_GUI); blePress('d'); delay(100); bleReleaseAll();
    safeDelay(400);
  }
  blePress(KEY_LEFT_GUI); blePress('r'); delay(100); bleReleaseAll();
  safeDelay(700);
  blePrint("notepad"); bleWrite(KEY_RETURN);
  safeDelay(1400);
  if (maximize) {
    blePress(KEY_LEFT_GUI); blePress(KEY_UP_ARROW); delay(100); bleReleaseAll();
    safeDelay(200);
  }
}

void btTypeText(const String& text, int delayMs, const String& style) {
  if (!bleIsConnected()) return;
  btApplyStyle(style);
  delay(100);
  String t = text;
  if (style == "CAPS") t.toUpperCase();
  for (int i = 0; i < (int)t.length(); i++) {
    if (emergencyKill) break;
    bleWrite((uint8_t)t[i]);
    if (delayMs > 0) safeDelay(delayMs);
  }
}

void btRunPayload(int idx) {
  if (idx < 0 || idx >= payloadCount) return;
  BtPayload& p = payloads[idx];
  addLog("[BT] Payload START: " + p.name);
  for (int i = 0; i < p.stepCount; i++) {
    if (emergencyKill) { addLog("[BT] Payload KILLED: " + p.name); return; }
    String step = p.steps[i];
    if      (step.startsWith("TYPE:"))  btTypeText(step.substring(5), 50, "NORMAL");
    else if (step.startsWith("DELAY:")) safeDelay(step.substring(6).toInt());
    else if (step == "KEY:ENTER")  bleWrite(KEY_RETURN);
    else if (step == "KEY:TAB")    bleWrite(KEY_TAB);
    else if (step == "KEY:ESC")    bleWrite(KEY_ESC);
    else if (step == "KEY:WIN+R")  { blePress(KEY_LEFT_GUI); blePress('r'); delay(100); bleReleaseAll(); }
    else if (step == "KEY:WIN+L")  { blePress(KEY_LEFT_GUI); blePress('l'); delay(100); bleReleaseAll(); }
    else if (step == "KEY:WIN+D")  { blePress(KEY_LEFT_GUI); blePress('d'); delay(100); bleReleaseAll(); }
    if (p.delays[i] > 0) safeDelay(p.delays[i]);
  }
  addLog("[BT] Payload DONE: " + p.name);
}
