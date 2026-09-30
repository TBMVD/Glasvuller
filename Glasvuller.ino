#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <condition_variable>

#define TRIG_PIN 12
#define ECHO_PIN A2
#define POMP_PIN A5
#define KNOP_PIN 15

const float VOL_AFSTAND_MM = 50;  // zelf invullen
const unsigned long MAX_POMPTIJD_MS = 30000;
const float GELUIDSSNELHEID = 0.0343;

SemaphoreHandle_t xPompStartSeintje;
SemaphoreHandle_t xPompParaatSeintje;

LiquidCrystal_I2C lcd(0x27, 20, 4);

float meetAfstandMM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  float duur = pulseIn(ECHO_PIN, HIGH, 30000);
  return (duur / 2) * GELUIDSSNELHEID * 10;
}

void zetOpLcd(const char* l1, const char* l2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
}

void TaskKnop(void *pvParameters) {
  for (;;) {
    static bool isPressed = false;

    if (digitalRead(KNOP_PIN) == LOW && !isPressed) {
      isPressed = true;
      xSemaphoreTake(xPompParaatSeintje, portMAX_DELAY);
      xSemaphoreGive(xPompStartSeintje);
      Serial.println("Semaphore given");
    }
    else if (digitalRead(KNOP_PIN) == HIGH && isPressed) {
    isPressed = false;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void TaskPomp(void *pvParameters) {
  for (;;) {
    
    xSemaphoreTake(xPompStartSeintje, portMAX_DELAY);

    Serial.println("Semaphore taken");

    vTaskDelay(pdMS_TO_TICKS(1000));
    digitalWrite(POMP_PIN, HIGH);

    zetOpLcd("Bezig met vullen...", "");

    unsigned long startTijd = millis();
    while (true) {
      float afstand = meetAfstandMM();
      Serial.println(afstand);

      if (afstand > 0 && afstand <= VOL_AFSTAND_MM) break;
      if (millis() - startTijd > MAX_POMPTIJD_MS) break;

      vTaskDelay(pdMS_TO_TICKS(100));
    }
    zetOpLcd("Glas gevuld!", "Pak uw glas.");

    digitalWrite(POMP_PIN, LOW);

    vTaskDelay(pdMS_TO_TICKS(1000));
    zetOpLcd("Glasvuller 3000", "Druk op de knop");
    Serial.println("Done");
    xSemaphoreGive(xPompParaatSeintje);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(POMP_PIN, OUTPUT);
  pinMode(KNOP_PIN, INPUT_PULLUP);  // knop tussen pin en GND
  digitalWrite(POMP_PIN, LOW);

  xPompStartSeintje = xSemaphoreCreateBinary();
  xPompParaatSeintje = xSemaphoreCreateBinary();
  
  xSemaphoreGive(xPompParaatSeintje);

  lcd.init();
  lcd.backlight();
  zetOpLcd("Glasvuller 3000", "Druk op de knop");

  xTaskCreate(TaskKnop, "Knop", 2048, NULL, 2, NULL);
  xTaskCreate(TaskPomp, "Pomp", 2048, NULL, 1, NULL);
}

void loop() {}