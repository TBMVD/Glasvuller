#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <condition_variable>
#include <atomic>

#define TRIG_PIN 12
#define ECHO_PIN A2
#define POMP_PIN A5
#define KNOP_PIN 15

const float VOL_AFSTAND_MM = 50;
const unsigned long MAX_POMPTIJD_MS = 30000;
const float GELUIDSSNELHEID = 0.0343;

SemaphoreHandle_t xPompStartSeintje;
std::atomic<bool> isPompActief = false;

LiquidCrystal_I2C lcd(0x27, 20, 4);

// Lees de afstand af van de ultrasoon sensor en geef de waarde in millimeters
float meetAfstandMM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  float duur = pulseIn(ECHO_PIN, HIGH, 30000);
  return (duur / 2) * GELUIDSSNELHEID * 10;
}

// Methode om tekst op de LCD scherm te zetten
void zetOpLcd(const char* l1, const char* l2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
}

// Hou bij wanneer de knop ingedrukt en losgelaten wordt en geef een startseintje wanneer de pomp inactief is.
void TaskKnop(void* pvParameters) {
  for (;;) {
    static bool isIngedrukt = false;

    if (digitalRead(KNOP_PIN) == LOW && !isIngedrukt) {
      isIngedrukt = true;
      if (!isPompActief) {
        xSemaphoreGive(xPompStartSeintje);
        Serial.println("Pomp Semaphore gegeven");
      }
    } else if (digitalRead(KNOP_PIN) == HIGH && isIngedrukt) {
      isIngedrukt = false;
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// Wacht op het startseintje en laat de pomp draaien tot de beker vol is.
void TaskPomp(void* pvParameters) {
  for (;;) {
    xSemaphoreTake(xPompStartSeintje, portMAX_DELAY);
    Serial.println("Pomp Semaphore Gepakt");
    isPompActief = true;

    vTaskDelay(pdMS_TO_TICKS(1000));
    digitalWrite(POMP_PIN, HIGH);

    zetOpLcd("Bezig met vullen...", "");

    unsigned long startTijd = millis();
    while (true) {
      float afstand = meetAfstandMM();
      Serial.println(afstand);

      if (afstand > 0 && afstand <= VOL_AFSTAND_MM) {
        zetOpLcd("Glas gevuld!", "Pak uw glas.");
        break;
      }
      if (millis() - startTijd > MAX_POMPTIJD_MS) {
        zetOpLcd("Pomp stopgezet", "");
        break;
      }

      vTaskDelay(pdMS_TO_TICKS(100));
    }

    digitalWrite(POMP_PIN, LOW);

    vTaskDelay(pdMS_TO_TICKS(1000));
    zetOpLcd("Glasvuller 3000", "Druk op de knop");
    Serial.println("Glas gevuld");
    isPompActief = false;
  }
}


void setup() {
  Serial.begin(9600);

  // Setup pinnen
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(POMP_PIN, OUTPUT);
  pinMode(KNOP_PIN, INPUT_PULLUP);  // knop tussen pin en GND
  digitalWrite(POMP_PIN, LOW);

  xPompStartSeintje = xSemaphoreCreateBinary();

  // Setup LCD scherm
  lcd.init();
  lcd.backlight();
  zetOpLcd("Glasvuller 3000", "Druk op de knop");

  // Maak de taken aan
  xTaskCreate(TaskKnop, "Knop", 2048, NULL, 2, NULL);
  xTaskCreate(TaskPomp, "Pomp", 2048, NULL, 1, NULL);
}

void loop() {}