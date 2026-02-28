// ==========================================================
//   CITADEL v2.0.0 — CIT_V_2_0_0
//   File 05: UI — Login Page + Terminal HTML
// ==========================================================

//  Login Page 
String buildLoginPage(bool firstBoot, bool failed, bool locked) {
  String msg = locked ? "TOO MANY ATTEMPTS. TRY AGAIN LATER." :
               failed ? "ACCESS DENIED" : "";
  String h =
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1'>"
    "<link rel='manifest' href='/manifest.json'>"
    "<title>CITADEL</title>"
    "<style>"
    "*{box-sizing:border-box;margin:0;padding:0}"
    "body{background:#000080;font-family:'Courier New',monospace;"
    "display:flex;align-items:center;justify-content:center;min-height:100vh;}"
    ".win{background:#c0c0c0;border-top:2px solid #fff;border-left:2px solid #fff;"
    "border-right:2px solid #808080;border-bottom:2px solid #808080;width:320px;}"
    ".title{background:#000080;color:#fff;font-weight:bold;padding:3px 6px;"
    "font-size:13px;display:flex;align-items:center;gap:6px;}"
    ".title::before{content:'\\25A0';}"
    ".body{padding:20px;}"
    "p{font-size:13px;color:#000;margin-bottom:16px;}"
    "label{font-size:12px;color:#000;display:block;margin-bottom:2px;}"
    ".field{width:100%;background:#fff;border-top:2px solid #808080;"
    "border-left:2px solid #808080;border-right:2px solid #fff;"
    "border-bottom:2px solid #fff;padding:4px 6px;"
    "font-family:'Courier New',monospace;font-size:13px;margin-bottom:12px;outline:none;}"
    ".row{display:flex;justify-content:center;margin-top:4px;}"
    ".btn{background:#c0c0c0;border-top:2px solid #fff;border-left:2px solid #fff;"
    "border-right:2px solid #808080;border-bottom:2px solid #808080;"
    "padding:5px 24px;font-family:'Courier New',monospace;font-size:13px;"
    "cursor:pointer;color:#000;}"
    ".btn:active{border-top:2px solid #808080;border-left:2px solid #808080;"
    "border-right:2px solid #fff;border-bottom:2px solid #fff;}"
    ".err{color:#c00;font-size:12px;text-align:center;margin-top:8px;}"
    "@keyframes shake{0%,100%{transform:translateX(0)}20%,60%{transform:translateX(-8px)}40%,80%{transform:translateX(8px)}}"
    ".shake{animation:shake 0.4s ease;}"
    "</style></head><body>";
  h += "<div class='win";
  h += (failed||locked?" shake":"");
  h += "' id='w'><div class='title'>CITADEL &mdash; ";
  h += (firstBoot?"FIRST TIME SETUP":"SECURE TERMINAL");
  h += "</div><div class='body'><p>";
  h += (firstBoot?"Welcome. Set your credentials. Saved permanently.":"Enter credentials to continue.");
  h += "</p>";
  if (msg!="") h+="<div class='err'>&#9632; "+msg+"</div><br>";
  h += "<form method='POST' action='";
  h += (firstBoot?"/setup":"/login");
  h += "'>"
       "<label>Username:</label><input class='field' name='u' type='text' autocomplete='off'><br>"
       "<label>Password:</label><input class='field' name='p' type='password'>";
  if (firstBoot) h+="<br><label>Confirm:</label><input class='field' name='p2' type='password'>";
  h += "<div class='row'><button class='btn' type='submit'>";
  h += (firstBoot?"INITIALIZE":"LOGIN");
  h += "</button></div></form></div></div><script>if(";
  h += (failed||locked?"true":"false");
  h += "){"
       "var w=document.getElementById('w');"
       "setTimeout(function(){w.classList.remove('shake');},500);}"
       "</script></body></html>";
  return h;
}

String buildTerminal() {
  return R"rawhtml(<!DOCTYPE html>
<html lang="en"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="mobile-web-app-capable" content="yes">
<meta name="theme-color" content="#000080">
<link rel="manifest" href="/manifest.json">
<title>CITADEL</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
:root{--blue:#000080;--white:#fff;--grey:#c0c0c0;--dgrey:#808080;
  --cyan:#00ffff;--yellow:#ffff00;--red:#ff5555;--orange:#ff8800;--green:#00ff00;}
html,body{height:100%;overflow:hidden;background:var(--blue);
  font-family:'Courier New',monospace;color:var(--white);}
#app{display:flex;flex-direction:column;height:100vh;padding:6px;gap:4px;}
.win{background:var(--blue);border-top:2px solid var(--white);
  border-left:2px solid var(--white);border-right:2px solid var(--dgrey);
  border-bottom:2px solid var(--dgrey);display:flex;flex-direction:column;}
.titlebar{background:var(--blue);color:var(--white);font-weight:bold;
  font-size:13px;padding:3px 8px;display:flex;align-items:center;
  justify-content:space-between;flex-shrink:0;}
.titlebar .left{display:flex;align-items:center;gap:6px;}
.titlebar .left::before{content:'\25A0';}
.pill{font-size:11px;display:flex;align-items:center;gap:4px;}
.pill .dot{width:8px;height:8px;border-radius:50%;background:#aaa;}
.pill.online .dot{background:var(--green);}
.pill.partial .dot{background:var(--yellow);}
.pill.offline .dot{background:var(--red);}
#main{display:flex;flex:1;gap:4px;min-height:0;}
#terminal-wrap{flex:1;display:flex;flex-direction:column;min-width:0;}
#output{flex:1;overflow-y:auto;padding:6px;font-size:13px;line-height:1.5;
  word-break:break-word;background:#000070;
  border-top:2px solid var(--dgrey);border-left:2px solid var(--dgrey);
  border-right:2px solid var(--white);border-bottom:2px solid var(--white);}
#output .ts{color:var(--dgrey);font-size:11px;}
#output .cmd{color:var(--white);cursor:pointer;}
#output .resp{color:#aad4ff;}
#output .cy{color:var(--cyan);}
#output .yw{color:var(--yellow);}
#output .er{color:var(--red);}
#sidebar{width:210px;flex-shrink:0;display:flex;flex-direction:column;gap:4px;}
.swin{font-size:12px;}
.swin .titlebar{font-size:12px;padding:2px 6px;}
.sbody{padding:6px;background:#000060;font-size:12px;line-height:1.9;
  border-top:2px solid var(--dgrey);border-left:2px solid var(--dgrey);
  border-right:2px solid var(--white);border-bottom:2px solid var(--white);
  overflow-y:auto;flex:1;}
.srow{display:flex;justify-content:space-between;}
.sk{color:var(--dgrey);}
.sv{color:var(--cyan);}
.sv.ok{color:var(--green);}
.sv.warn{color:var(--yellow);}
.sv.err{color:var(--red);}
.sv.orange{color:var(--orange);}
.srefresh{font-size:10px;color:var(--dgrey);text-align:right;margin-top:4px;}
#statusbar{background:#000060;color:var(--white);font-size:12px;
  padding:2px 8px;flex-shrink:0;white-space:nowrap;overflow:hidden;
  border-top:1px solid var(--dgrey);}
#inputrow{display:flex;align-items:center;gap:4px;flex-shrink:0;padding-top:2px;}
#prompt{color:var(--yellow);font-size:13px;white-space:nowrap;flex-shrink:0;}
#cmd{flex:1;background:#000060;color:var(--white);
  border-top:2px solid var(--dgrey);border-left:2px solid var(--dgrey);
  border-right:2px solid var(--white);border-bottom:2px solid var(--white);
  padding:5px 8px;font-family:'Courier New',monospace;font-size:13px;
  outline:none;caret-color:var(--white);}
#sendbtn{background:var(--grey);border-top:2px solid var(--white);
  border-left:2px solid var(--white);border-right:2px solid var(--dgrey);
  border-bottom:2px solid var(--dgrey);padding:5px 14px;
  font-family:'Courier New',monospace;font-size:13px;cursor:pointer;color:#000;}
#sendbtn:active{border-top:2px solid var(--dgrey);border-left:2px solid var(--dgrey);
  border-right:2px solid var(--white);border-bottom:2px solid var(--white);}
#buttons{display:flex;flex-wrap:wrap;gap:3px;padding-top:2px;}
.btn{background:var(--grey);border-top:2px solid var(--white);
  border-left:2px solid var(--white);border-right:2px solid var(--dgrey);
  border-bottom:2px solid var(--dgrey);padding:4px 8px;
  font-family:'Courier New',monospace;font-size:12px;cursor:pointer;color:#000;}
.btn:active{border-top:2px solid var(--dgrey);border-left:2px solid var(--dgrey);
  border-right:2px solid var(--white);border-bottom:2px solid var(--white);}
.fkey{background:transparent;border:none;color:var(--white);
  font-family:'Courier New',monospace;font-size:12px;cursor:pointer;
  padding:4px 6px;text-decoration:underline;}
.fkey:hover{color:var(--cyan);}
.sep{color:var(--dgrey);font-size:12px;align-self:center;}
.modal-overlay{display:none;position:fixed;top:0;left:0;width:100%;height:100%;
  background:rgba(0,0,128,0.85);z-index:100;align-items:center;justify-content:center;}
.modal-overlay.show{display:flex;}
.modal{background:#c0c0c0;border-top:2px solid #fff;border-left:2px solid #fff;
  border-right:2px solid #808080;border-bottom:2px solid #808080;width:300px;}
.modal .title{background:#000080;color:#fff;font-weight:bold;padding:3px 6px;font-size:13px;}
.modal .body{padding:16px;}
.modal .body p{font-size:13px;margin-bottom:12px;}
.modal .field{width:100%;background:#fff;border-top:2px solid #808080;
  border-left:2px solid #808080;border-right:2px solid #fff;
  border-bottom:2px solid #fff;padding:4px 6px;
  font-family:'Courier New',monospace;font-size:13px;outline:none;}
.modal .row{display:flex;justify-content:center;margin-top:12px;}
.modal .mbtn{background:#c0c0c0;border-top:2px solid #fff;border-left:2px solid #fff;
  border-right:2px solid #808080;border-bottom:2px solid #808080;
  padding:5px 24px;font-family:'Courier New',monospace;font-size:13px;cursor:pointer;}
@media(max-width:600px){
  #sidebar{display:none;}
  #app{padding:3px;gap:2px;}
  #output{font-size:12px;}
  #cmd,#sendbtn{font-size:12px;}
  .btn,.fkey{font-size:11px;}
}
</style></head>
<body>
<div id="app">
  <div class="win" style="flex-shrink:0">
    <div class="titlebar">
      <div class="left">CITADEL v2.0.0</div>
      <div class="pill" id="pill"><div class="dot"></div><span id="pilltext">CONNECTING</span></div>
    </div>
  </div>
  <div id="main">
    <div id="terminal-wrap" class="win">
      <div class="titlebar"><div class="left">TERMINAL OUTPUT</div></div>
      <div id="output"></div>
    </div>
    <div id="sidebar">
      <div class="win swin" style="flex:1;display:flex;flex-direction:column;">
        <div class="titlebar"><div class="left" id="panel-title">SYSTEM STATUS</div></div>
        <div class="sbody" id="panel-body">
          <div class="srow"><span class="sk">CPU</span><span class="sv" id="s_cpu">---</span></div>
          <div class="srow"><span class="sk">TEMP</span><span class="sv" id="s_temp">---</span></div>
          <div class="srow"><span class="sk">HEAP</span><span class="sv" id="s_heap">---</span></div>
          <div class="srow"><span class="sk">HEALTH</span><span class="sv" id="s_health">---</span></div>
          <div class="srow"><span class="sk">THRTL</span><span class="sv" id="s_thr">---</span></div>
          <div class="srow" style="margin-top:4px"><span class="sk">AP</span><span class="sv" id="s_ap">---</span></div>
          <div class="srow"><span class="sk">STA</span><span class="sv" id="s_sta">---</span></div>
          <div class="srow" style="margin-top:4px"><span class="sk">BT</span><span class="sv" id="s_bt">---</span></div>
          <div class="srow"><span class="sk">OLED</span><span class="sv" id="s_oled">---</span></div>
          <div class="srow" style="margin-top:4px"><span class="sk">EVILTWIN</span><span class="sv" id="s_et">---</span></div>
          <div class="srow"><span class="sk">CAPS</span><span class="sv" id="s_caps">---</span></div>
          <div class="srow"><span class="sk">DEAUTH</span><span class="sv" id="s_da">---</span></div>
          <div class="srow" style="margin-top:4px"><span class="sk">SESSION</span><span class="sv" id="s_sess">---</span></div>
          <div class="srow"><span class="sk">UPTIME</span><span class="sv" id="s_up">---</span></div>
          <div class="srefresh">&#8635; live</div>
        </div>
      </div>
    </div>
  </div>
  <div id="statusbar">Loading...</div>
  <div id="inputrow">
    <span id="prompt">C:\CITADEL&gt;</span>
    <input id="cmd" type="text" autocomplete="off" autocorrect="off"
           autocapitalize="off" spellcheck="false" autofocus>
    <button id="sendbtn" onclick="sendCmd()">ENTER</button>
  </div>
  <div id="buttons">
    <button class="btn" onclick="sendCmd('BT ON')">BT ON</button>
    <button class="btn" onclick="sendCmd('BT OFF')">BT OFF</button>
    <button class="btn" onclick="sendCmd('BT LOCK')">LOCK</button>
    <button class="btn" onclick="sendCmd('ATTACK STOP')">KILL</button>
    <span class="sep">|</span>
    <button class="fkey" onclick="sendCmd('SYS TOP')">[F1]TOP</button>
    <button class="fkey" onclick="sendCmd('WIFI STATUS')">[F2]NET</button>
    <button class="fkey" onclick="sendCmd('ATTACK SCAN')">[F3]SCAN</button>
    <button class="fkey" onclick="sendCmd('ATTACK STATUS')">[F4]ATK</button>
    <button class="fkey" onclick="sendCmd('HELP')">[F5]HELP</button>
    <button class="fkey" onclick="sendCmd('SYS REBOOT')">[F6]BOOT</button>
    <button class="fkey" onclick="sendCmd('SYS STEALTH')">[F7]HIDE</button>
    <button class="fkey" onclick="sendCmd('SYS VERSION')">[F8]VER</button>
  </div>
</div>

<!-- Session expired modal -->
<div class="modal-overlay" id="relogin-modal">
  <div class="modal">
    <div class="title">&#9632; SESSION EXPIRED</div>
    <div class="body">
      <p>Your session has timed out.<br>Enter password to continue.</p>
      <input class="field" type="password" id="relogin-pass" placeholder="Password">
      <div class="row"><button class="mbtn" onclick="doRelogin()">RECONNECT</button></div>
    </div>
  </div>
</div>

<script>
var cmdHistory=[], histIdx=-1;
var sessionAlive=true;

function ts(){
  var d=new Date();
  return '['+String(d.getHours()).padStart(2,'0')+':'+
    String(d.getMinutes()).padStart(2,'0')+':'+
    String(d.getSeconds()).padStart(2,'0')+']';
}

function escHtml(s){
  return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}

function appendOutput(cmd,resp){
  var o=document.getElementById('output');
  var d=document.createElement('div');
  d.innerHTML='<span class="ts">'+ts()+'</span> '+
    '<span class="cmd" title="Click to retype" onclick="retype(this)">'+escHtml(cmd)+'</span><br>'+
    '<span class="resp">'+resp+'</span><br>';
  o.appendChild(d);
  o.scrollTop=o.scrollHeight;
  return d;
}

function retype(el){
  document.getElementById('cmd').value=el.textContent;
  document.getElementById('cmd').focus();
}

function sendCmd(c){
  if(!sessionAlive){showReloginModal();return;}
  var inp=document.getElementById('cmd');
  var cmd=c||inp.value.trim();
  if(!cmd)return;
  if(!c){inp.value='';if(cmdHistory[0]!==cmd)cmdHistory.unshift(cmd);if(cmdHistory.length>50)cmdHistory.pop();histIdx=-1;}
  var line=appendOutput(cmd,'<span style="color:#555">...</span>');
  fetch('/e?c='+encodeURIComponent(cmd))
    .then(function(r){
      if(r.status===403){showReloginModal();return null;}
      return r.text();
    })
    .then(function(t){
      if(t===null)return;
      line.querySelector('.resp').innerHTML=t||'<span style="color:#555">OK</span>';
      document.getElementById('output').scrollTop=document.getElementById('output').scrollHeight;
    })
    .catch(function(){
      line.querySelector('.resp').innerHTML='<span class="er">CONNECTION ERROR</span>';
    });
  inp.focus();
}

document.getElementById('cmd').addEventListener('keydown',function(e){
  if(e.key==='Enter'){sendCmd();return;}
  if(e.key==='ArrowUp'){e.preventDefault();if(histIdx<cmdHistory.length-1){histIdx++;this.value=cmdHistory[histIdx];}return;}
  if(e.key==='ArrowDown'){e.preventDefault();if(histIdx>0){histIdx--;this.value=cmdHistory[histIdx];}else{histIdx=-1;this.value='';}return;}
  if(e.key==='Tab'){
    e.preventDefault();
    var v=this.value.toUpperCase().trim();
    var cmds=['WIFI CONNECT','WIFI DISCONNECT','WIFI FORGET','WIFI STATUS','WIFI SCAN','WIFI IP',
      'WIFI CLIENTS','WIFI SNIFF ON','WIFI SNIFF OFF','WIFI AP NAME','WIFI AP PASS','WIFI AP ON','WIFI AP OFF',
      'BT ON','BT OFF','BT STATUS','BT PAIR','BT TYPE','BT NOTE','BT NOTE FULL','BT NOTE CLEAN',
      'BT NOTE SAVE','BT SAY','BT LOCK','BT RICKROLL','BT MATRIX','BT MUTE','BT VOL UP','BT VOL DOWN',
      'BT NAME','BT SHELL','BT URL','BT DOWNLOAD','BT ADMIN','BT SEARCH','BT SCREENSHOT','BT MINALL',
      'BT PAYLOAD LIST','BT PAYLOAD NEW','BT PAYLOAD RUN','BT PAYLOAD DELETE','BT PAYLOAD VIEW','BT PAYLOAD SAVE','BT PAYLOAD STEP',
      'SYS TOP','SYS HEALTH','SYS FREQ','SYS REBOOT','SYS CLEAR','SYS VERSION','SYS THERMAL','SYS UPTIME',
      'SYS MEMORY','SYS STORAGE','SYS PREFS','SYS PREFS RESET','SYS LOG','SYS CRASH','SYS BANNER',
      'SYS TIME','SYS TIME SYNC','SYS TIME ZONE','SYS STRESS',
      'SYS PANEL STATUS','SYS PANEL WATCH','SYS PANEL LOG','SYS PANEL CLIENTS',
      'SYS PANEL ATTACK','SYS PANEL PAYLOADS','SYS PANEL PREFS','SYS PANEL NETWORK','SYS PANEL THERMAL','SYS PANEL OFF',
      'SYS OLED STATUS','SYS OLED TERMINAL','SYS OLED TERMINAL ALERTS','SYS OLED TERMINAL OFF',
      'SYS OLED CLOCK','SYS OLED HEALTH','SYS OLED ATTACK','SYS OLED NETWORK',
      'SYS OLED MSG','SYS OLED SCROLL','SYS OLED OFF','SYS OLED ON','SYS OLED CLEAR',
      'SYS OLED BRIGHTNESS','SYS OLED FLIP','SYS OLED MODE','SYS STEALTH','SYS REVEAL',
      'AUTH ON','AUTH OFF','AUTH USER','AUTH PASSWORD','AUTH STATUS','AUTH TIMEOUT','AUTH RESET',
      'AUTH AUTORENEW ON','AUTH AUTORENEW OFF','AUTH LOGOUT','AUTH LOG',
      'AUTH LOCKOUT STATUS','AUTH LOCKOUT CLEAR',
      'AUTH WHITELIST ON','AUTH WHITELIST OFF','AUTH WHITELIST ADD','AUTH WHITELIST LIST',
      'AUTH ACCOUNT NEW','AUTH ACCOUNT LIST','AUTH ACCOUNT DELETE','AUTH ACCOUNT LOCK','AUTH ACCOUNT UNLOCK',
      'OTA STATUS','OTA ENABLE','OTA DISABLE','OTA PASSWORD','OTA RESTART',
      'ATTACK SCAN','ATTACK EVILTWIN','ATTACK EVILTWIN OFF','ATTACK CREDS','ATTACK CREDS CLEAR',
      'ATTACK DEAUTH','ATTACK DEAUTH OFF','ATTACK BEACON','ATTACK BEACON OFF',
      'ATTACK HANDSHAKE','ATTACK HANDSHAKE OFF','ATTACK HANDSHAKE SHOW',
      'ATTACK THEME','ATTACK CLIENTS','ATTACK LOG','ATTACK STATUS','ATTACK STOP',
      'ALIAS','ALIAS LIST','ALIAS DELETE','HISTORY',
      'HELP','HELP WIFI','HELP BT','HELP BT SAY','HELP BT NOTE','HELP BT PAYLOAD','HELP BT MEMORY',
      'HELP ATTACK','HELP SYS','HELP SYS OLED','HELP AUTH','HELP OTA'];
    if(v==='')return;
    var m=cmds.filter(function(c){return c.startsWith(v);});
    if(m.length===1){this.value=m[0]+' ';}
    else if(m.length>1){
      var o=document.getElementById('output');
      var d=document.createElement('div');
      d.innerHTML='<span class="ts">'+ts()+'</span> <span class="cy">'+m.join('&nbsp;&nbsp;')+'</span><br>';
      o.appendChild(d);o.scrollTop=o.scrollHeight;
    }
    return;
  }
});

function updatePanel(d){
  var healthColors={GREEN:'ok',YELLOW:'warn',ORANGE:'orange',RED:'err'};
  document.getElementById('s_cpu').textContent=d.cpu+'MHz';
  var te=document.getElementById('s_temp');
  te.textContent=d.temp+'C';
  te.className='sv'+(parseFloat(d.temp)>=75?' err':parseFloat(d.temp)>=70?' warn':'');
  document.getElementById('s_heap').textContent=d.heap+'KB';
  var sh=document.getElementById('s_health');
  sh.textContent=d.health;
  sh.className='sv '+(healthColors[d.health]||'');
  var tr=document.getElementById('s_thr');
  tr.textContent=d.throttle;
  tr.className='sv'+(d.throttle==='YES'?' err':'');
  document.getElementById('s_ap').textContent=d.ap;
  var ss=document.getElementById('s_sta');
  ss.textContent=d.sta; ss.className='sv'+(d.sta_ok?' ok':'');
  var sb=document.getElementById('s_bt');
  sb.textContent=d.bt; sb.className='sv'+(d.bt_paired?' ok':d.bt_on?' warn':'');
  document.getElementById('s_oled').textContent=d.oled;
  var set=document.getElementById('s_et');
  set.textContent=d.eviltwin; set.className='sv'+(d.eviltwin!='OFF'?' warn':'');
  document.getElementById('s_caps').textContent=d.captures;
  var sda=document.getElementById('s_da');
  sda.textContent=d.deauth; sda.className='sv'+(d.deauth!='OFF'?' warn':'');
  document.getElementById('s_sess').textContent=d.session;
  document.getElementById('s_up').textContent=d.uptime;

  var p=document.getElementById('pill'), pt=document.getElementById('pilltext');
  p.className='pill';
  if(d.sta_ok&&d.bt_paired){p.classList.add('online');pt.textContent='ONLINE';}
  else if(d.sta_ok||d.bt_on){p.classList.add('partial');pt.textContent='PARTIAL';}
  else{p.classList.add('offline');pt.textContent='OFFLINE';}

  document.getElementById('statusbar').textContent=
    'CPU:'+d.cpu+'MHz TEMP:'+d.temp+'C HEAP:'+d.heap+'KB HEALTH:'+d.health+' BT:'+d.bt+' UP:'+d.uptime;

  if(d.session==='EXPIRED'){sessionAlive=false;showReloginModal();}
  else{sessionAlive=true;}
}

function updateStatus(){
  fetch('/status')
    .then(function(r){return r.json();})
    .then(updatePanel)
    .catch(function(){});
}

function showReloginModal(){
  document.getElementById('relogin-modal').classList.add('show');
  document.getElementById('relogin-pass').focus();
}

function doRelogin(){
  var p=document.getElementById('relogin-pass').value;
  fetch('/relogin',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'p='+encodeURIComponent(p)})
    .then(function(r){return r.json();})
    .then(function(d){
      if(d.ok){
        document.getElementById('relogin-modal').classList.remove('show');
        document.getElementById('relogin-pass').value='';
        sessionAlive=true;
        appendOutput('SYSTEM','Session renewed.');
      } else {
        document.getElementById('relogin-pass').value='';
        appendOutput('SYSTEM','<span class="er">Wrong password.</span>');
      }
    });
}

document.getElementById('relogin-pass').addEventListener('keydown',function(e){
  if(e.key==='Enter') doRelogin();
});

updateStatus();
setInterval(updateStatus,3000);
</script>
</body></html>)rawhtml";
}
