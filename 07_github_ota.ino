// ==========================================================
//   CITADEL v2.1.0 — CIT_V_2_1_22
//   File: 07_github_ota.ino
//   GitHub OTA — boot version check, fetch, self-flash
// ==========================================================

#include "lwip/dns.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>

static const char* GH_OWNER    = "ggreg04";
static const char* GH_REPO     = "CITADEL";
static const char* CURRENT_VER = "v2.1.0";
static const char* GH_TOKEN_KEY = "gh_token";

static String ghLoadToken() {
  return prefs.getString(GH_TOKEN_KEY, "");
}

void ghSaveToken(const String& token) {
  prefs.putString(GH_TOKEN_KEY, token);
  addLog("[GH-OTA] Token saved to flash.");
}

static String jsonExtract(const String& json, const String& key) {
  String search = "\"" + key + "\":\"";
  int start = json.indexOf(search);
  if (start < 0) return "";
  start += search.length();
  int end = json.indexOf("\"", start);
  if (end < 0) return "";
  return json.substring(start, end);
}

static String jsonExtractDownloadUrl(const String& json) {
  String key = "\"browser_download_url\":\"";
  int start = json.indexOf(key);
  if (start < 0) return "";
  start += key.length();
  int end = json.indexOf("\"", start);
  if (end < 0) return "";
  return json.substring(start, end);
}

static bool isNewer(const String& remote, const String& current) {
  if (remote == current) return false;
  String r = remote.startsWith("v") ? remote.substring(1) : remote;
  String c = current.startsWith("v") ? current.substring(1) : current;
  int rMaj = r.substring(0, r.indexOf('.')).toInt();
  int cMaj = c.substring(0, c.indexOf('.')).toInt();
  if (rMaj != cMaj) return rMaj > cMaj;
  r = r.substring(r.indexOf('.') + 1);
  c = c.substring(c.indexOf('.') + 1);
  int rMin = r.substring(0, r.indexOf('.')).toInt();
  int cMin = c.substring(0, c.indexOf('.')).toInt();
  if (rMin != cMin) return rMin > cMin;
  int rPat = r.substring(r.indexOf('.') + 1).toInt();
  int cPat = c.substring(c.indexOf('.') + 1).toInt();
  return rPat > cPat;
}

String ghGetLatestTag() {
  WiFiClient client;  // Plain client, no SSL verification
  HTTPClient http;
  String url = "https://api.github.com/repos/";
  url += GH_OWNER;
  url += "/";
  url += GH_REPO;
  url += "/releases/latest";
  http.begin(client, url);

  String token = ghLoadToken();
  if (token.length() > 0) {
    http.addHeader("Authorization", "Bearer " + token);
  }
  http.addHeader("User-Agent", "CITADEL-ESP32");
  http.addHeader("Accept", "application/vnd.github+json");

  int code = http.GET();
  if (code != 200) {
    addLog("[GH-OTA] API error: " + String(code));
    http.end();
    return "";
  }

  String body = http.getString();
  http.end();
  return jsonExtract(body, "tag_name");
}

String ghGetBinUrl() {
  String token = ghLoadToken();
  WiFiClient client;  // Plain client, no SSL verification

  HTTPClient http;
  String url = "https://api.github.com/repos/";
  url += GH_OWNER;
  url += "/";
  url += GH_REPO;
  url += "/releases/latest";
  http.begin(client, url);

  if (token.length() > 0) {
    http.addHeader("Authorization", "Bearer " + token);
  }
  http.addHeader("User-Agent", "CITADEL-ESP32");
  http.addHeader("Accept", "application/vnd.github+json");

  int code = http.GET();
  if (code != 200) {
    http.end();
    return "";
  }

  String body = http.getString();
  http.end();
  return jsonExtractDownloadUrl(body);
}

bool ghFlashBin(const String& binUrl) {
  if (binUrl == "") {
    addLog("[GH-OTA] No .bin URL found.");
    return false;
  }

  String token = ghLoadToken();
  WiFiClient client;  // Plain client, no SSL verification

  HTTPClient http;
  http.begin(client, binUrl);

  if (token.length() > 0) {
    http.addHeader("Authorization", "Bearer " + token);
  }
  http.addHeader("User-Agent", "CITADEL-ESP32");
  http.addHeader("Accept", "application/octet-stream");

  int code = http.GET();
  if (code != 200) {
    addLog("[GH-OTA] Download error: " + String(code));
    http.end();
    return false;
  }

  int contentLen = http.getSize();
  if (contentLen <= 0) {
    addLog("[GH-OTA] Invalid content length.");
    http.end();
    return false;
  }

  if (!Update.begin(contentLen)) {
    addLog("[GH-OTA] Not enough flash space.");
    http.end();
    return false;
  }

  addLog("[GH-OTA] Flashing " + String(contentLen / 1024) + "KB...");
  oledDraw("GH-OTA", "FLASHING...", String(contentLen / 1024) + "KB");

  WiFiClient* stream = http.getStreamPtr();
  static uint8_t buf[512];
  size_t written = 0;

  while (http.connected() && written < (size_t)contentLen) {
    size_t avail = stream->available();
    if (avail) {
      size_t rd = stream->readBytes(buf, min(avail, sizeof(buf)));
      Update.write(buf, rd);
      written += rd;
    }
    yield();
  }

  http.end();

  if (!Update.end(true)) {
    addLog("[GH-OTA] Finalize failed.");
    oledDraw("GH-OTA", "FLASH FAILED", "");
    return false;
  }

  addLog("[GH-OTA] Flash complete. Rebooting...");
  oledDraw("GH-OTA", "FLASH OK", "REBOOTING...");
  return true;
}

void checkGithubOta() {
  if (!staConnected) return;

  addLog("[GH-OTA] Checking for update...");
  oledDraw("GH-OTA","CHECKING...","");

  String latestTag = ghGetLatestTag();
  if (latestTag == "") {
    addLog("[GH-OTA] Could not reach GitHub.");
    return;
  }

  addLog("[GH-OTA] Latest: "+latestTag+"  Current: "+String(CURRENT_VER));
  if (!isNewer(latestTag, String(CURRENT_VER))) {
    addLog("[GH-OTA] Firmware up to date.");
    return;
  }

  addLog("[GH-OTA] Update found: "+latestTag+" — fetching...");
  oledDraw("GH-OTA","UPDATE FOUND",latestTag);
  delay(1500);

  String binUrl = ghGetBinUrl();
  if (ghFlashBin(binUrl)) {
    delay(2000);
    ESP.restart();
  }
}