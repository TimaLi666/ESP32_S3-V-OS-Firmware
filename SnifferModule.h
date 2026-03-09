#ifndef SNIFFER_MODULE_H
#define SNIFFER_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"

// Внешние переменные и функции из основного файла
extern Adafruit_SSD1306 display;
extern int stationsCount;
extern String foundStations[10];
extern unsigned long packetsCount;
extern int eapol_found; // Для Handshake
extern void updateDisplay(String msg);

void drawStatusBar() {
  unsigned long sec = millis() / 1000;
  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK); 
  display.setCursor(98, 55);  
  display.printf("%02d:%02d", (sec / 60) % 60, sec % 60);
}

void runSniffer() {
  packetsCount = 0;
  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb([](void* buf, wifi_promiscuous_pkt_type_t type) {
    packetsCount++;
  });

  int ch = 1;
  while (digitalRead(PIN_BACK) == HIGH) {
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    ch = (ch % 13) + 1;
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("SNIFF CH:%d", ch);
    display.setCursor(0, 25);
    display.setTextSize(2);
    display.printf("PKTS:%lu", packetsCount);
    display.setTextSize(1);
    display.setCursor(0, 55);
    display.println("BACK to EXIT");
    display.display();
    delay(200);
  }
  esp_wifi_set_promiscuous(false);
}

void stations_cb(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* frame = pkt->payload;

  if (frame[0] == 0x40 && stationsCount < 10) {  // Probe Request
    char macStr[20];
    sprintf(macStr, "%02X:%02X:%02X:%02X", frame[10], frame[11], frame[12], frame[13]);
    String mac = String(macStr);

    bool exists = false;
    for (int i = 0; i < stationsCount; i++)
      if (foundStations[i] == mac) exists = true;

    if (!exists) {
      foundStations[stationsCount] = mac;
      stationsCount++;

      // МИГАЕМ ПРИ НАХОДКЕ
      digitalWrite(LED_PIN, HIGH);
      delay(50);  // Короткая вспышка
      digitalWrite(LED_PIN, LOW);

      Serial.println("[NEW DEVICE] " + mac);
    }
  }
}

void runScanStations() {
  stationsCount = 0; 
  for (int i = 0; i < 10; i++) foundStations[i] = ""; 

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&stations_cb);

  int ch = 1;
  unsigned long lastHop = 0;
  unsigned long lastScroll = millis(); 
  int scrollPage = 0;

  while (digitalRead(PIN_BACK) == HIGH) {
    unsigned long currentMillis = millis();

    // 1. ПЕРЕКЛЮЧЕНИЕ КАНАЛОВ
    if (currentMillis - lastHop > 300) {
      ch = (ch % 13) + 1;
      esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
      lastHop = currentMillis;
    }

    // 2. ТАЙМЕР ПЕРЕКЛЮЧЕНИЯ СТРАНИЦ
    if (currentMillis - lastScroll > 3000) { 
      if (stationsCount > 4) {
          scrollPage = (scrollPage == 0) ? 1 : 0;
      } else {
          scrollPage = 0; 
      }
      lastScroll = currentMillis; 
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // ЗАГОЛОВОК (Добавил номер страницы P1/P2)
    display.setCursor(0, 0);
    display.printf("DEVICES [%d/10] P%d", stationsCount, scrollPage + 1);
    display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

    if (stationsCount == 0) {
      display.setCursor(30, 30);
      display.println("Searching...");
    } else {
      // --- ИСПРАВЛЕННАЯ ЛОГИКА ОТРИСОВКИ СТРАНИЦ ---
      int startIdx = scrollPage * 4; // Будет либо 0, либо 4
      int endIdx = startIdx + 4;     // Будет либо 4, либо 8
      if (endIdx > stationsCount) endIdx = stationsCount;

      for (int i = startIdx; i < endIdx; i++) {
        int row = i - startIdx; // Позиция на экране всегда 0, 1, 2, 3
        display.setCursor(0, 12 + (row * 10));
        display.print(i + 1);
        display.print(". ");
        display.println(foundStations[i]); 
      }
      
      // Маленький индикатор, если есть еще страницы
      if (stationsCount > 4) {
        display.setCursor(115, 42);
        display.print(scrollPage == 0 ? "v" : "^"); 
      }
    }

    // НИЖНЯЯ ПАНЕЛЬ
    display.fillRect(0, 53, 128, 11, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(2, 55);
    display.print("< EXIT");

    drawStatusBar();
    
    display.display();
    yield();
  }
  esp_wifi_set_promiscuous(false);
}


#endif