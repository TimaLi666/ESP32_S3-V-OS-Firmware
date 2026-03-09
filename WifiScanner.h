#ifndef WIFI_SCAN_H
#define WIFI_SCAN_H

extern void updateDisplay(String msg);

#include <WiFi.h>

// Структура для хранения данных одной точки доступа
struct AccessPoint {
  String ssid;
  uint8_t bssid[6];
  int channel;
  int rssi;
};

// Массив для хранения найденных сетей (максимум 20 для экономии памяти)
AccessPoint foundNetworks[20];
int networksCount = 0;

// Функция сканирования
void runScanAP() {
  networksCount = 0;
  display.clearDisplay();
  display.setCursor(20, 30);
  display.print("SCANNING WI-FI...");
  display.display();

  int n = WiFi.scanNetworks(); // Запускаем стандартный скан
  
  if (n == 0) {
    updateDisplay("NO NETWORKS\nFOUND");
  } else {
    networksCount = (n > 20) ? 20 : n; // Берем первые 20 сетей
    for (int i = 0; i < networksCount; i++) {
      foundNetworks[i].ssid = WiFi.SSID(i);
      foundNetworks[i].channel = WiFi.channel(i);
      foundNetworks[i].rssi = WiFi.RSSI(i);
      memcpy(foundNetworks[i].bssid, WiFi.BSSID(i), 6);
    }
    updateDisplay("FOUND: " + String(networksCount) + "\nAP's");
  }
  delay(1500);
}

// Функция выбора цели из списка (визуальное меню)
void selectTargetMenu() {
  int localSelected = 0;
  while(digitalRead(PIN_BACK) == HIGH) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("SELECT TARGET:");
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

    for (int i = 0; i < 4; i++) {
      int idx = i; // Тут можно добавить скролл, как мы делали раньше
      if (idx >= networksCount) break;
      
      int y = 15 + (i * 12);
      if (idx == localSelected) {
        display.fillRect(0, y-1, 128, 11, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
      } else display.setTextColor(SSD1306_WHITE);

      display.setCursor(5, y);
      display.print(foundNetworks[idx].ssid.substring(0, 15));
    }
    
    display.display();

    // Навигация внутри выбора цели
    if (digitalRead(PIN_UP) == LOW) { localSelected--; delay(150); }
    if (digitalRead(PIN_DWN) == LOW) { localSelected++; delay(150); }
    if (localSelected < 0) localSelected = networksCount - 1;
    if (localSelected >= networksCount) localSelected = 0;

    // Подтверждение выбора
    if (digitalRead(PIN_SET) == LOW) {
      // ЗАПИСЫВАЕМ ДАННЫЕ В ТАРГЕТ ДЛЯ СНАЙПЕРА (из WifiHacking.h)
      extern uint8_t target_bssid[6];
      extern int target_channel;
      extern String target_ssid;

      memcpy(target_bssid, foundNetworks[localSelected].bssid, 6);
      target_channel = foundNetworks[localSelected].channel;
      target_ssid = foundNetworks[localSelected].ssid;

      updateDisplay("TARGET SET:\n" + target_ssid);
      delay(1500);
      return;
    }
  }
}

#endif
