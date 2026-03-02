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
      oledDraw("CITADEL v2.2.4", l2.substring(0,21), l3.substring(0,21));
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

//  BLE 
void startBle() {
  if (ESP.getFreeHeap() < 60000)
    addLog("[BT] WARN: Low heap (" + String(ESP.getFreeHeap()/1024) + "KB). May be unstable.");
  if (bleKbd != nullptr) { delete bleKbd; bleKbd = nullptr; delay(200); }
  bleKbd = new BleKeyboard(btName.c_str(), "CITADEL", 100);
  bleKbd->begin();
  btActive = true;
  addLog("[BT] Started as \"" + btName + "\" Heap:" + String(ESP.getFreeHeap()/1024) + "KB");
}

void stopBle() {
  if (bleKbd != nullptr) { delete bleKbd; bleKbd = nullptr; delay(100); }
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
  if (!bleKbd || !bleKbd->isConnected()) return;
  if (style == "BOLD") {
    bleKbd->press(KEY_LEFT_CTRL); bleKbd->press('b'); delay(80); bleKbd->releaseAll();
  } else if (style == "ITALIC") {
    bleKbd->press(KEY_LEFT_CTRL); bleKbd->press('i'); delay(80); bleKbd->releaseAll();
  } else if (style == "BOLD+ITALIC") {
    bleKbd->press(KEY_LEFT_CTRL); bleKbd->press('b'); delay(80); bleKbd->releaseAll();
    bleKbd->press(KEY_LEFT_CTRL); bleKbd->press('i'); delay(80); bleKbd->releaseAll();
  }
}

void btOpenNotepad(bool maximize, bool minimizeFirst) {
  if (!bleKbd || !bleKbd->isConnected()) return;
  if (minimizeFirst) {
    bleKbd->press(KEY_LEFT_GUI); bleKbd->press('d'); delay(100); bleKbd->releaseAll();
    safeDelay(400);
  }
  bleKbd->press(KEY_LEFT_GUI); bleKbd->press('r'); delay(100); bleKbd->releaseAll();
  safeDelay(700);
  bleKbd->print("notepad"); bleKbd->write(KEY_RETURN);
  safeDelay(1400);
  if (maximize) {
    bleKbd->press(KEY_LEFT_GUI); bleKbd->press(KEY_UP_ARROW); delay(100); bleKbd->releaseAll();
    safeDelay(200);
  }
}

void btTypeText(const String& text, int delayMs, const String& style) {
  if (!bleKbd || !bleKbd->isConnected()) return;
  btApplyStyle(style);
  delay(100);
  String t = text;
  if (style == "CAPS") t.toUpperCase();
  for (int i = 0; i < (int)t.length(); i++) {
    if (emergencyKill) break;
    bleKbd->print(t.substring(i, i+1));
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
    else if (step == "KEY:ENTER")  bleKbd->write(KEY_RETURN);
    else if (step == "KEY:TAB")    bleKbd->write(KEY_TAB);
    else if (step == "KEY:ESC")    bleKbd->write(KEY_ESC);
    else if (step == "KEY:WIN+R")  { bleKbd->press(KEY_LEFT_GUI); bleKbd->press('r'); delay(100); bleKbd->releaseAll(); }
    else if (step == "KEY:WIN+L")  { bleKbd->press(KEY_LEFT_GUI); bleKbd->press('l'); delay(100); bleKbd->releaseAll(); }
    else if (step == "KEY:WIN+D")  { bleKbd->press(KEY_LEFT_GUI); bleKbd->press('d'); delay(100); bleKbd->releaseAll(); }
    if (p.delays[i] > 0) safeDelay(p.delays[i]);
  }
  addLog("[BT] Payload DONE: " + p.name);
}
