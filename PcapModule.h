#ifndef PCAP_MODULE_H
#define PCAP_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <Adafruit_SSD1306.h>

// Внешние объекты из основного файла
extern Adafruit_SSD1306 display;

// Структура заголовка пакета PCAP (16 байт)
struct pcap_pkthdr {
    uint32_t ts_sec;   
    uint32_t ts_usec;  
    uint32_t caplen;   
    uint32_t len;      
};

// Функция отправки Глобального заголовка PCAP (24 байта)
void sendGlobalHeader() {
    uint32_t magic_number = 0xa1b2c3d4;
    uint16_t version_major = 2;
    uint16_t version_minor = 4;
    int32_t  thiszone = 0;
    uint32_t sigfigs = 0;
    uint32_t snaplen = 65535;
    uint32_t network = 105; // IEEE 802.11 (Wifi)

    Serial.write((uint8_t*)&magic_number, 4);
    Serial.write((uint8_t*)&version_major, 2);
    Serial.write((uint8_t*)&version_minor, 2);
    Serial.write((uint8_t*)&thiszone, 4);
    Serial.write((uint8_t*)&sigfigs, 4);
    Serial.write((uint8_t*)&snaplen, 4);
    Serial.write((uint8_t*)&network, 4);
}

// ОСНОВНАЯ ФУНКЦИЯ СНИФФЕРА
void runPcapSniffer() {
    display.clearDisplay();
    display.setCursor(10, 20);
    display.println("PCAP STREAMING...");
    display.setCursor(10, 40);
    display.println("USE PYTHON SCRIPT");
    display.display();

    WiFi.mode(WIFI_STA);
    esp_wifi_set_promiscuous(true);
    
    // 1. Сначала отправляем заголовок файла
    sendGlobalHeader(); 

    // 2. Настраиваем перехват и отправку каждого пакета
    esp_wifi_set_promiscuous_rx_cb([](void* buf, wifi_promiscuous_pkt_type_t type) {
        wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
        
        // В ESP32 длина пакета находится в подструктуре rx_ctrl
        uint32_t packet_len = pkt->rx_ctrl.sig_len; 

        pcap_pkthdr h;
        h.ts_sec = millis() / 1000;
        h.ts_usec = (millis() % 1000) * 1000;
        h.caplen = packet_len;
        h.len = packet_len;

        // Отправляем заголовок пакета и данные
        Serial.write((uint8_t*)&h, 16);
        Serial.write(pkt->payload, packet_len);
    });

    int ch = 1;
    unsigned long lastHop = 0;

    // Цикл работы, пока не нажата кнопка BACK (пин 18)
    while (digitalRead(18) == HIGH) {
        // Переключаем каналы, чтобы ловить всё
        if (millis() - lastHop > 200) {
            ch = (ch % 13) + 1;
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            lastHop = millis();
        }
        yield(); 
    }

    esp_wifi_set_promiscuous(false);
}

#endif
