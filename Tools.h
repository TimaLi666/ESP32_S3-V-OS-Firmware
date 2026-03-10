void showDeviceInfo() {
  while (digitalRead(PIN_BACK) == HIGH) { // Цикл пока не нажата кнопка НАЗАД
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("--- DEVICE INFO ---"));
    
    // Модель чипа и версия
    display.printf("Chip: ESP32-S3\n");
    display.printf("Rev: %d\n", ESP.getChipRevision());
    
    // Память (в КБ)
    display.printf("Free RAM: %d KB\n", ESP.getFreeHeap() / 1024);
    
    // Flash память
    display.printf("Flash: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    
    // Напряжение (если настроено)
    display.printf("Battery: %.2fV\n", getBatteryVoltage()); 
    
    display.setCursor(0, 55);
    display.println(F("Press BACK to exit"));
    display.display();
    delay(100); 
  }
}
