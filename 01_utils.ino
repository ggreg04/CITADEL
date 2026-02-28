// ==========================================================
//   CITADEL v2.0.0 — CIT_V_2_0_0
//   File 01: Utilities — Logging, Parsing, Auth, Flash, Clock
// ==========================================================

//  Safe Delay 
void safeDelay(int ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    if (emergencyKill) return;
    yield();
  }
}

//  Clock 
void setClock(int mhz) {
  if (currentFreq == mhz) return;
  setCpuFrequencyMhz(mhz);
  currentFreq = mhz;
}

void burstClock() {
  if (!thermalThrottled) setClock(240);
}

//  Sensors 
float getTemp() {
  return temperatureRead();
}

String getUptime() {
  unsigned long s = (millis() - bootTime) / 1000;
  unsigned long m = s / 60; s %= 60;
  unsigned long h = m / 60; m %= 60;
  char buf[12];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", h, m, s);
  return String(buf);
}

String getRealTime() {
  if (!ntpSynced) return getUptime();
  time_t now = time(nullptr) + (ntpOffset * 3600);
  struct tm* t = localtime(&now);
  char buf[12];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
  return String(buf);
}

//  Logging 
void addLog(String entry) {
  terminalLog += entry + "\n";
  if (terminalLog.length() > 8000)
    terminalLog = terminalLog.substring(terminalLog.length() - 6000);
  if (oledMode == OLED_TERMINAL || oledMode == OLED_TERMINAL_ALERTS)
    oledLastResp = entry.substring(0, 21);
}

void addAttackLog(String entry) {
  String ts = "[" + getRealTime() + "] ";
  attackLog += ts + entry + "\n";
  if (attackLog.length() > 4000)
    attackLog = attackLog.substring(attackLog.length() - 3000);
  addLog("[ATTACK] " + entry);
}

void addLoginLog(String ip, String user, bool success) {
  String ts  = "[" + getRealTime() + "] ";
  String entry = ts + ip + " USER:" + user + " " + (success ? "OK" : "FAIL");
  loginLog += entry + "\n";
  if (loginLog.length() > 2000)
    loginLog = loginLog.substring(loginLog.length() - 1500);
}

//  HTML Encode / Strip 
String htmlEncode(const String& s) {
  String r = s;
  r.replace("&", "&amp;"); r.replace("<", "&lt;"); r.replace(">", "&gt;");
  return r;
}

String stripHtml(String s) {
  while (s.indexOf('<') >= 0) {
    int a = s.indexOf('<'), b = s.indexOf('>');
    if (b > a) s = s.substring(0, a) + s.substring(b + 1);
    else break;
  }
  s.replace("&amp;", "&"); s.replace("&lt;", "<"); s.replace("&gt;", ">");
  return s;
}

//  Quoted Arg Parser 
String parseQuotedArg(const String& input, int argIndex) {
  int count = 0, i = 0;
  while (i < (int)input.length()) {
    while (i < (int)input.length() && input[i] == ' ') i++;
    if (i >= (int)input.length()) break;
    String token = "";
    if (input[i] == '"') {
      i++;
      while (i < (int)input.length() && input[i] != '"') { token += input[i]; i++; }
      if (i < (int)input.length()) i++;
    } else {
      while (i < (int)input.length() && input[i] != ' ') { token += input[i]; i++; }
    }
    if (count == argIndex) return token;
    count++;
  }
  return "";
}

//  Auth Utilities 
String generateToken() {
  String t = "";
  for (int i = 0; i < 32; i++) t += (char)('a' + random(26));
  return t;
}

bool isIpLocked(String ip) {
  for (int i = 0; i < loginAttemptCount; i++)
    if (loginAttempts[i].ip == ip && loginAttempts[i].lockUntil > millis()) return true;
  return false;
}

void recordFailedLogin(String ip) {
  for (int i = 0; i < loginAttemptCount; i++) {
    if (loginAttempts[i].ip == ip) {
      loginAttempts[i].fails++;
      if (loginAttempts[i].fails >= 3)
        loginAttempts[i].lockUntil = millis() + 300000UL;
      return;
    }
  }
  if (loginAttemptCount < 8) {
    loginAttempts[loginAttemptCount] = {ip, 1, 0};
    loginAttemptCount++;
  }
}

void clearFailedLogin(String ip) {
  for (int i = 0; i < loginAttemptCount; i++)
    if (loginAttempts[i].ip == ip) { loginAttempts[i].fails = 0; loginAttempts[i].lockUntil = 0; }
}

bool isWhitelisted(String ip) {
  if (!whitelistEnabled || whitelistCount == 0) return true;
  for (int i = 0; i < whitelistCount; i++)
    if (authWhitelist[i] == ip) return true;
  return false;
}

bool isAuthenticated() {
  if (!authEnabled) return true;
  if (sessionToken == "") return false;
  if (authTimeout > 0 && !authAutoRenew &&
      millis() - sessionStart > (unsigned long)authTimeout * 60000UL) {
    sessionToken = "";
    return false;
  }
  String cookie = server.header("Cookie");
  bool valid = cookie.indexOf("CSID=" + sessionToken) >= 0;
  if (valid && authAutoRenew) sessionStart = millis();
  return valid;
}

//  Flash Storage 
void savePayloads() {
  prefs.putInt("pl_count", payloadCount);
  for (int i = 0; i < payloadCount; i++) {
    String key = "pl_" + String(i);
    String val = payloads[i].name + "|" + String(payloads[i].stepCount);
    for (int j = 0; j < payloads[i].stepCount; j++)
      val += "|" + payloads[i].steps[j] + "|" + String(payloads[i].delays[j]);
    prefs.putString(key.c_str(), val);
  }
}

void loadPayloads() {
  payloadCount = prefs.getInt("pl_count", 0);
  for (int i = 0; i < payloadCount && i < MAX_PAYLOADS; i++) {
    String key = "pl_" + String(i);
    String val = prefs.getString(key.c_str(), "");
    if (val == "") continue;
    int pos = 0, next = val.indexOf('|', pos);
    payloads[i].name = val.substring(pos, next); pos = next + 1;
    next = val.indexOf('|', pos);
    int steps = val.substring(pos, next).toInt(); pos = next + 1;
    payloads[i].stepCount = steps;
    for (int j = 0; j < steps && j < 32; j++) {
      next = val.indexOf('|', pos);
      payloads[i].steps[j] = val.substring(pos, next); pos = next + 1;
      next = val.indexOf('|', pos); if (next < 0) next = val.length();
      payloads[i].delays[j] = val.substring(pos, next).toInt(); pos = next + 1;
    }
  }
}

void saveAliases() {
  prefs.putInt("alias_count", aliasCount);
  for (int i = 0; i < aliasCount; i++) {
    String key = "alias_" + String(i);
    prefs.putString(key.c_str(), aliases[i].alias + "|" + aliases[i].command);
  }
}

void loadAliases() {
  aliasCount = prefs.getInt("alias_count", 0);
  for (int i = 0; i < aliasCount && i < MAX_ALIASES; i++) {
    String key   = "alias_" + String(i);
    String val   = prefs.getString(key.c_str(), "");
    int    sep   = val.indexOf('|');
    if (sep >= 0) { aliases[i].alias = val.substring(0, sep); aliases[i].command = val.substring(sep + 1); }
  }
}

void saveAccounts() {
  prefs.putInt("acc_count", accountCount);
  for (int i = 0; i < accountCount; i++) {
    String key = "acc_" + String(i);
    prefs.putString(key.c_str(),
      accounts[i].username + "|" + accounts[i].password + "|" +
      accounts[i].role + "|" + String(accounts[i].locked ? 1 : 0));
  }
}

void loadAccounts() {
  accountCount = prefs.getInt("acc_count", 0);
  for (int i = 0; i < accountCount && i < MAX_ACCOUNTS; i++) {
    String key = "acc_" + String(i);
    String val = prefs.getString(key.c_str(), "");
    int p1 = val.indexOf('|'), p2 = val.indexOf('|', p1+1), p3 = val.indexOf('|', p2+1);
    if (p1 >= 0 && p2 >= 0 && p3 >= 0) {
      accounts[i].username = val.substring(0, p1);
      accounts[i].password = val.substring(p1+1, p2);
      accounts[i].role     = val.substring(p2+1, p3);
      accounts[i].locked   = val.substring(p3+1).toInt();
    }
  }
}
