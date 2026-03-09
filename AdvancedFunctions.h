#ifndef ADV_FUNC_H
#define ADV_FUNC_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLE2902.h>
#include <BLEAdvertising.h>

extern bool isDimmed; // Говорим файлу, что переменная создана в другом месте
extern void updateDisplay(String msg);
extern void runScanAP();        // Из WifiScanner.h
extern void toggleGhostMode();  // Если она в другом месте
extern void resetPowerTimers(); // Из PowerManagement.h
extern void drawMenu();         // Из основного файла
extern void runAutoSniper(); // Из WifiHacking.h

// Переменные состояния
bool ghostModeActive = false;
String lastSerialMsg = "No data yet...";
bool captureRunning = false;
int eapolCount = 0; // Количество пойманных пакетов рукопожатия


// --- 1. GHOST MODE (Скрытый узел) ---
void toggleGhostMode() {
  lastProcess = millis();
  ghostModeActive = !ghostModeActive;
  if (ghostModeActive) {
    uint8_t newMac[6];
    for (int i = 0; i < 6; i++) newMac[i] = random(0, 256);
    newMac[0] = (newMac[0] & 0xFE) | 0x02; // Установка бита локального адреса
    esp_wifi_set_mac(WIFI_IF_STA, newMac);
  } else {
    // Возврат заводского MAC (требует перезагрузки для полной чистоты)
    ESP.restart(); 
  }
}

// --- 2. BLE SCANNER (Bluetooth) ---
void runBLEScan() {
  lastProcess = millis();
  display.clearDisplay();
  display.setCursor(20, 20);
  display.println("Initializing BLE...");
  display.display();

  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);
  BLEScanResults foundDevices = pBLEScan->start(5, false);
  int count = foundDevices.getCount();

  while (digitalRead(PIN_BACK) == HIGH) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("BLE FOUND: %d", count);
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

    for (int i = 0; i < count && i < 4; i++) {
      BLEAdvertisedDevice d = foundDevices.getDevice(i);
      display.setCursor(0, 15 + (i * 12));
      String name = d.getName().c_str();
      if (name == "") name = d.getAddress().toString().c_str();
      display.print(String(i+1) + "." + name.substring(0, 14));
    }
    
    display.setCursor(0, 55);
    display.print("[BACK] to Exit");
    display.display();
    yield();
  }
  BLEDevice::deinit(false);
}

// --- 3. SERIAL MESSENGER ---
void runSerialMessenger() {
  while (digitalRead(PIN_BACK) == HIGH) {
    if (Serial.available() > 0) {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();

      // --- ОБРАБОТКА КОМАНД С ПК ---
      if (cmd == "SCAN") { runScanAP(); Serial.println("V-OS: Scanning started..."); }
      else if (cmd == "GHOST") { toggleGhostMode(); Serial.println("V-OS: Ghost Mode Toggled"); }
      else if (cmd == "REBOOT") ESP.restart();
      else {
        lastSerialMsg = "PC: " + cmd; // Просто выводим текст, если это не команда
        Serial.println("V-OS: Received -> " + cmd);
      }
      resetPowerTimers();
    }

    // Отрисовка интерфейса терминала
    display.clearDisplay();
    display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
    display.fillRect(0, 0, 128, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(20, 1);
    display.print("V-OS TERMINAL");

    display.setTextColor(SSD1306_WHITE);
    display.setCursor(5, 15);
    display.println(lastSerialMsg != "" ? lastSerialMsg : ">>> _");
    
    display.setCursor(5, 55);
    display.print("[BACK] EXIT");
    display.display();
    
    yield();
  }
}

void toggleBrightness() {
  isDimmed = !isDimmed;
  display.dim(isDimmed); // true — тусклый режим, false — яркий
}

void runApplePopup() {
  uint8_t apple_data[] = {
    0x11, 0x07, 0x11, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  
  BLEDevice::init("");
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  
  while(digitalRead(PIN_BACK) == HIGH) {
    display.clearDisplay();
    display.setCursor(10, 20); display.println("APPLE/ANDROID");
    display.setCursor(10, 35); display.println("POPUP ATTACK...");
    display.setCursor(0, 55);  display.println("[BACK] TO STOP");
    display.display();

    // Меняем тип устройства для каждого "пакета"
    apple_data[6] = random(0, 20); // AirPods, Beats, etc.
    
    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
    oAdvertisementData.setManufacturerData(std::string((char*)apple_data, 17));
    pAdvertising->setAdvertisementData(oAdvertisementData);
    pAdvertising->start();
    
    delay(100); // Интенсивность спама
    pAdvertising->stop();
    lastProcess = millis(); // Сбрасываем таймер сна
  }
  BLEDevice::deinit(false);
}

void runHandshakeCapture() {
  eapolCount = 0;
  captureRunning = true;
  
  // Настройка Wi-Fi в режим монитора
  esp_wifi_set_promiscuous(true);
  
  updateDisplay("CAPTURING...\nCh: " + String(WiFi.channel()) + "\nEAPOL: 0");

  while(digitalRead(PIN_BACK) == HIGH) {
    // В реальном Marauder здесь работает колбэк-функция, 
    // которая ловит пакеты типа WIFI_PKT_MGMT и фильтрует EAPOL.
    
    display.clearDisplay();
    display.setCursor(0, 0); display.print("SNIFFING HANDSHAKE");
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
    
    display.setCursor(0, 20);
    display.printf("Channel: %d", WiFi.channel());
    display.setCursor(0, 35);
    display.printf("EAPOL Packets: %d", eapolCount);
    
    if (eapolCount >= 4) {
      display.setCursor(0, 50);
      display.print("SUCCESS! [BACK]");
    } else {
      display.setCursor(0, 50);
      display.print("Waiting for client...");
    }
    
    display.display();
    lastProcess = millis(); // Не даем уснуть
    delay(200);
  }
  
  esp_wifi_set_promiscuous(false);
  captureRunning = false;
}

#endif
