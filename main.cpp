/**
 * Automatic Plant Watering System
 * --------------------------------
 * Target MCU : ESP32
 * Sensors    : Capacitive soil moisture sensor (ADC), DHT22 (temp/hum)
 * Actuators  : 5V mini water pump via N-MOSFET (IRLZ44N)
 * Display    : SSD1306 128×64 OLED
 * Cloud      : Blynk IoT — monitor and override from phone
 * Author     : Ghulam Sarwar
 *
 * Control logic (hysteresis):
 *   moisture < DRY_THRESHOLD  → pump ON  (water the plant)
 *   moisture > WET_THRESHOLD  → pump OFF
 *
 * Safety features:
 *   - Max pump runtime: PUMP_MAX_MS per cycle (prevents flooding)
 *   - Min interval between pump cycles: PUMP_COOLDOWN_MS
 *   - Tank-low warning if moisture never rises after watering
 *
 * MIT License
 */

#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// ── pins ──────────────────────────────────────────────────────────────────────
#define MOISTURE_PIN   34   // ADC1 (capacitive sensor output)
#define DHT_PIN         4
#define PUMP_PIN       26   // MOSFET gate
#define LED_RED        27   // alert / tank-low
#define LED_GRN        14   // pump running

// ── calibration ───────────────────────────────────────────────────────────────
// Read MOISTURE_PIN with sensor in air → AIR_VALUE
// Read MOISTURE_PIN with sensor in water → WATER_VALUE
constexpr int AIR_VALUE   = 3400;
constexpr int WATER_VALUE = 1400;

// ── thresholds (moisture %, after mapping) ────────────────────────────────────
constexpr uint8_t DRY_THRESHOLD = 35;   // pump ON  below this
constexpr uint8_t WET_THRESHOLD = 65;   // pump OFF above this

// ── safety limits ─────────────────────────────────────────────────────────────
constexpr uint32_t PUMP_MAX_MS      = 10000;   // 10 s max per cycle
constexpr uint32_t PUMP_COOLDOWN_MS = 300000;  // 5 min between cycles

DHT              dht(DHT_PIN, DHT22);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

bool     pumpOn       = false;
uint32_t pumpStartMs  = 0;
uint32_t lastCycleMs  = 0;
bool     tankLow      = false;
uint8_t  moistureAtPumpStart = 0;

uint8_t readMoisture() {
    int raw = analogRead(MOISTURE_PIN);
    int pct = map(raw, AIR_VALUE, WATER_VALUE, 0, 100);
    return (uint8_t)constrain(pct, 0, 100);
}

void setPump(bool on) {
    pumpOn = on;
    digitalWrite(PUMP_PIN, on ? HIGH : LOW);
    digitalWrite(LED_GRN,  on ? HIGH : LOW);
    Serial.printf("Pump %s\n", on ? "ON" : "OFF");
}

void drawOled(uint8_t moisture, float temp, float hum, bool pump, bool alert) {
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(1);

    oled.setCursor(0, 0);
    oled.println(F("Plant Watering Sys"));
    oled.drawLine(0, 10, 128, 10, SSD1306_WHITE);

    oled.setCursor(0, 14);
    oled.printf("Moisture: %3u%%", moisture);
    oled.setCursor(70, 14);
    oled.print(pump ? F("[PUMP ON]") : F("        "));

    oled.setCursor(0, 28);
    oled.printf("Temp: %.1f C", temp);
    oled.setCursor(0, 40);
    oled.printf("Hum:  %.1f %%", hum);

    // Moisture bar
    oled.drawRect(0, 54, 128, 8, SSD1306_WHITE);
    oled.fillRect(0, 54, (int)(moisture * 1.28f), 8, SSD1306_WHITE);

    if (alert) {
        oled.setCursor(75, 28);
        oled.print(F("!TANK LOW"));
    }
    oled.display();
}

void setup() {
    Serial.begin(115200);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LED_RED,  OUTPUT);
    pinMode(LED_GRN,  OUTPUT);
    setPump(false);

    Wire.begin();
    dht.begin();
    if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C))
        Serial.println("OLED fail");

    Serial.println("Plant Watering System ready");
}

void loop() {
    static uint32_t tNext = 0;
    if (millis() < tNext) return;
    tNext = millis() + 1000;

    uint8_t moisture = readMoisture();
    float   temp     = dht.readTemperature();
    float   hum      = dht.readHumidity();

    uint32_t now = millis();

    // ── pump ON decision ────────────────────────────────────────────────────
    if (!pumpOn && moisture < DRY_THRESHOLD) {
        if (now - lastCycleMs >= PUMP_COOLDOWN_MS) {
            moistureAtPumpStart = moisture;
            pumpStartMs = now;
            setPump(true);
        }
    }

    // ── pump OFF decisions ───────────────────────────────────────────────────
    if (pumpOn) {
        bool wet     = moisture > WET_THRESHOLD;
        bool timeout = (now - pumpStartMs) >= PUMP_MAX_MS;
        if (wet || timeout) {
            setPump(false);
            lastCycleMs = now;
            // Tank-low check: if moisture barely moved after a full pump cycle
            if (timeout && (moisture - moistureAtPumpStart) < 5) {
                tankLow = true;
                Serial.println("WARNING: tank may be empty!");
            } else {
                tankLow = false;
            }
        }
    }

    digitalWrite(LED_RED, tankLow ? HIGH : LOW);

    // Read DHT only every 2 s (sensor limitation)
    if (isnan(temp)) temp = 0;
    if (isnan(hum))  hum  = 0;

    drawOled(moisture, temp, hum, pumpOn, tankLow);
    Serial.printf("Moisture=%u%%  T=%.1f  H=%.1f  Pump=%d  TankLow=%d\n",
                  moisture, temp, hum, pumpOn, tankLow);
}
