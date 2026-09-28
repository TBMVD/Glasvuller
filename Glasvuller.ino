#include <Arduino.h>

#define TRIG_PIN 12
#define ECHO_PIN A2
#define POMP_PIN 13
#define KNOP_PIN 2

const float VOL_AFSTAND_MM = 50;              // zelf invullen
const unsigned long MAX_POMPTIJD_MS = 30000;
const float GELUIDSSNELHEID = 0.0343;

SemaphoreHandle_t xStartSeintje;

float meetAfstandMM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  float duur = pulseIn(ECHO_PIN, HIGH, 30000);
  return (duur / 2) * GELUIDSSNELHEID * 10;
}

void TaskKnop(void pvParameters) {
  for (;;) {
    if (digitalRead(KNOP_PIN) == LOW) {
      xSemaphoreGive(xStartSeintje);
      vTaskDelay(pdMS_TO_TICKS(500));
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void TaskPomp(voidpvParameters) {
  for (;;) {
    xSemaphoreTake(xStartSeintje, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(1000));
    digitalWrite(POMP_PIN, HIGH);

    unsigned long startTijd = millis();
    while (true) {
      float afstand = meetAfstandMM();
      Serial.println(afstand);

      if (afstand > 0 && afstand <= VOL_AFSTAND_MM) break;
      if (millis() - startTijd > MAX_POMPTIJD_MS) break;

      vTaskDelay(pdMS_TO_TICKS(100));
    }

    digitalWrite(POMP_PIN, LOW);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(POMP_PIN, OUTPUT);
  pinMode(KNOP_PIN, INPUT_PULLUP);   // knop tussen pin en GND
  digitalWrite(POMP_PIN, LOW);

  xStartSeintje = xSemaphoreCreateBinary();

  xTaskCreate(TaskKnop, "Knop", 2048, NULL, 2, NULL);
  xTaskCreate(TaskPomp, "Pomp", 2048, NULL, 1, NULL);
}

void loop() {}