#ifndef POWER_MGMT_H
#define POWER_MGMT_H

#include <Adafruit_SSD1306.h>
void drawDVDSaver(); 

// Настройки времени
#define SLEEP_TIMEOUT 300000  // 5 минут
#define OFF_TIMEOUT   900000  // 15 минут

// Сообщаем файлу, что эти функции и переменные существуют в других местах
extern void drawMenuBackground(); 
extern void resetPowerTimers();
extern Adafruit_SSD1306 display;
extern int bgMode;
extern void drawMenu();
extern float dvdX, dvdY, dvdDX, dvdDY; // Координаты из основного файла

unsigned long lastActivity = 0;   
unsigned long lastProcess = 0;    

// Сброс таймеров (переносим ВВЕРХ, чтобы функция ниже её видела)
void resetPowerTimers() {
  lastActivity = millis();
  lastProcess = millis();
}

// Функция ухода в сон
void enterDeepSleep(String reason) {
  display.clearDisplay();
  display.setCursor(20, 20);
  display.println("SYSTEM HALT:");
  display.setCursor(20, 35);
  display.println(reason);
  display.display();
  delay(2000);
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_SET, 0); 
  esp_deep_sleep_start();
}

void drawDVDSaver() {
  // 1. ПОЛНАЯ ОЧИСТКА (делает фон черным)
  display.clearDisplay();
  
  // 3. Обновление координат логотипа
  dvdX += dvdDX;
  dvdY += dvdDY;

  /// --- ТОЧНЫЕ ГРАНИЦЫ ОТСКОКА ---
  // Ширина текста V-OS (size 2) ~48px. 128 - 48 = 80.
  if (dvdX <= 0 || dvdX >= 80) { 
    dvdDX = -dvdDX; 
  }
  
  // Высота текста V-OS (size 2) ~14px. 64 - 14 = 50.
  if (dvdY <= 0 || dvdY >= 50) { 
    dvdDY = -dvdDY; 
  }

  // 4. Рисуем логотип V-OS белым по черному
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor((int)dvdX, (int)dvdY);
  display.print("V-OS");
  
  // 5. Вывод на физический экран
  display.display();
  delay(10); // Добавляем микро-паузу для стабильности картинки
  }

// Проверка таймеров
// Изменяем void на bool
bool checkPowerTimers() {
  unsigned long current = millis();

  if (current - lastActivity > SLEEP_TIMEOUT && current - lastProcess > SLEEP_TIMEOUT) {
    drawDVDSaver(); 
    
    if (digitalRead(PIN_SET) == LOW || digitalRead(PIN_UP) == LOW || 
        digitalRead(PIN_DWN) == LOW || digitalRead(PIN_BACK) == LOW) {
      
      resetPowerTimers();
      display.clearDisplay();
      display.setTextSize(1); 
      drawMenu(); 
    }
    return true; // СООБЩАЕМ, ЧТО ЗАСТАВКА АКТИВНА
  }

  if (current - lastActivity > OFF_TIMEOUT) {
    enterDeepSleep("AUTO OFF");
  }
  return false; // СООБЩАЕМ, ЧТО МЫ НЕ В СОНЕ
}

#endif
