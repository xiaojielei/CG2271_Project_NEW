#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ================== Pin / HW config ==================
#define DHT_PIN 4
#define DHT_TYPE DHT11
#define UART_RX_PIN 2 // ESP32 RX  <- FRDM TX (PTE22)
#define UART_TX_PIN 1 // ESP32 TX  -> FRDM RX (PTE23)

#define I2C_SDA 6
#define I2C_SCL 7
#define OLED_ADDR 0x3C
#define SCREEN_W 128
#define SCREEN_H 64

// ================== Globals ==================
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

String uartLine; // accumulates chars until '\n'
float dhtTemp = NAN, dhtHum = NAN;
int photoVal = -1, waterVal = -1;

unsigned long lastDhtReadMs = 0;         // last successful DHT sample time
unsigned long lastMcxcUpdateMs = 0;      // last time we got JSON from MCXC
const uint32_t DHT_MIN_PERIOD_MS = 1500; // DHT11 spec ~1 Hz; be gentle

// ================== Helpers ==================
static inline void oledPrintLine(uint8_t x, uint8_t y, const char *fmt, ...) {
  char buf[32];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  display.setCursor(x, y);
  display.print(buf);
}

void drawOLED() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Header
  oledPrintLine(0, 0, "Smart Plant Monitor");

  // MCXC sensor block
  oledPrintLine(0, 12, "MCXC:");
  if (photoVal >= 0)
    oledPrintLine(12, 24, "Photo: %d", photoVal);
  else
    oledPrintLine(12, 24, "Photo: --");
  if (waterVal >= 0)
    oledPrintLine(12, 36, "Water: %d", waterVal);
  else
    oledPrintLine(12, 36, "Water: --");

  // ESP32 sensor block
  oledPrintLine(0, 48, "ESP32:");
  if (!isnan(dhtTemp) && !isnan(dhtHum))
    oledPrintLine(48, 48, "%.1fC %.1f%%", dhtTemp, dhtHum);
  else
    oledPrintLine(48, 48, "--.-C --.-%%");

  // Ages (seconds since last updates) — small hint for freshness
  uint32_t now = millis();
  uint32_t ageDht = (lastDhtReadMs ? (now - lastDhtReadMs) / 1000 : 9999);
  uint32_t ageMcxc =
      (lastMcxcUpdateMs ? (now - lastMcxcUpdateMs) / 1000 : 9999);
  display.setTextSize(1);
  oledPrintLine(0, 56, "Age DHT:%lus MCXC:%lus", (unsigned long)ageDht,
                (unsigned long)ageMcxc);

  display.display();
}

void sendDhtJson() {
  // Rate-limit DHT reads to avoid hammering the sensor
  uint32_t now = millis();
  if (now - lastDhtReadMs >= DHT_MIN_PERIOD_MS || isnan(dhtTemp) ||
      isnan(dhtHum)) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      dhtTemp = t;
      dhtHum = h;
      lastDhtReadMs = now;
    }
  }

  // Reply regardless; if read failed, report an error JSON
  if (!isnan(dhtTemp) && !isnan(dhtHum)) {
    // Keep it compact, one decimal place
    char line[64];
    snprintf(line, sizeof(line), "{\"temp\":%.1f,\"humidity\":%.1f}\n", dhtTemp,
             dhtHum);
    Serial1.print(line);
    Serial.print("Sent to MCXC: ");
    Serial.print(line);
  } else {
    Serial1.println("{\"error\":\"DHT fail\"}");
    Serial.println("Sent to MCXC: {\"error\":\"DHT fail\"}");
  }
}

bool tryParseMcxcJson(const String &s) {
  // Expecting: {"photo":<int>,"water":<int>}
  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, s);
  if (err)
    return false;

  // Optional: tolerate either raw or missing fields (keep last if missing)
  if (doc.containsKey("photo")) {
    int v = doc["photo"];
    if (v >= 0)
      photoVal = v;
  }
  if (doc.containsKey("water")) {
    int v = doc["water"];
    if (v >= 0)
      waterVal = v;
  }
  lastMcxcUpdateMs = millis();
  Serial.print("MCXC update: ");
  Serial.println(s);
  return true;
}

void handleUartLine(const String &line) {
  String s = line;
  s.trim();
  if (s.length() == 0)
    return; // ignore empty

  // Command path
  if (s.equalsIgnoreCase("GET_DHT")) {
    sendDhtJson();
    return;
  }

  // JSON path (MCXC sensor update)
  if (s.startsWith("{")) {
    if (!tryParseMcxcJson(s)) {
      Serial.print("Invalid JSON from MCXC: ");
      Serial.println(s);
    }
    return;
  }

  // Unknown input (filter UART noise)
  Serial.print("Unknown input: ");
  Serial.println(s);
}

// ================== Setup / Loop ==================
void setup() {
  // USB debug
  Serial.begin(115200);
  delay(300);
  Serial.println("ESP32 Bridge + OLED + DHT starting...");

  // UART to MCXC
  Serial1.begin(9600, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  delay(50);
  // Optional: announce readiness to MCXC (harmless if ignored)
  Serial1.println("READY");

  // I2C + OLED
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 init failed");
    while (1) {
      delay(1000);
    }
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Display init OK");
  display.display();
  delay(600);

  // DHT
  dht.begin();

  // First draw
  drawOLED();
}

void loop() {
  // Heartbeat (brief blink on some boards); avoid pin assumptions
  // Just a small periodic activity:
  static uint32_t lastBeat = 0;
  if (millis() - lastBeat > 1000) {
    lastBeat = millis();
    Serial.print('.');
  }

  // Non-blocking UART line reader
  while (Serial1.available()) {
    char c = (char)Serial1.read();
    if (c == '\r')
      continue; // normalize
    if (c == '\n') {
      handleUartLine(uartLine);
      uartLine = "";
    } else {
      // Avoid runaway if MCXC somehow spams without newline
      if (uartLine.length() < 200)
        uartLine += c;
      else
        uartLine = ""; // reset if absurdly long without newline
    }
  }

  // Opportunistic DHT refresh (for OLED freshness even without GET_DHT)
  if (millis() - lastDhtReadMs >= DHT_MIN_PERIOD_MS) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      dhtTemp = t;
      dhtHum = h;
      lastDhtReadMs = millis();
    }
  }

  // Update OLED at ~10 Hz max to avoid flicker
  static uint32_t lastDraw = 0;
  if (millis() - lastDraw >= 100) {
    lastDraw = millis();
    drawOLED();
  }
}