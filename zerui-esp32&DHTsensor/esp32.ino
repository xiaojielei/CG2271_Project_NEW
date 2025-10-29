#include <Arduino.h>
#include <DHT.h>

#define DHT_PIN 4
#define DHT_TYPE DHT11
const int NEW_TX_PIN = 1;   // TX -> MCU RX (PTB16)
const int NEW_RX_PIN = 2;   // RX -> MCU TX (PTB17)

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);                                 // USB debug
  Serial1.begin(9600, SERIAL_8N1, NEW_RX_PIN, NEW_TX_PIN); // UART to FRDM
  dht.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("ESP32 UART-DHT Bridge Starting...");
}

void loop() {
  // Heartbeat LED
  digitalWrite(LED_BUILTIN, HIGH);
  delay(50);
  digitalWrite(LED_BUILTIN, LOW);
  delay(950);

  // Check for message from FRDM
  if (Serial1.available()) {
    String cmd = Serial1.readStringUntil('\n');
    cmd.trim();
    Serial.print("Received command: ");
    Serial.println(cmd);

    if (cmd.equalsIgnoreCase("GET_DHT")) {
      float t = dht.readTemperature();
      float h = dht.readHumidity();
      if (isnan(t) || isnan(h)) {
        Serial.println("DHT read fail");
        Serial1.println("{\"error\":\"DHT read fail\"}");
      } else {
        String json = "{\"temp\":" + String(t, 1) + ",\"humidity\":" + String(h, 1) + "}";
        Serial1.println(json);          // send to FRDM
        Serial.println(json);           // show on USB monitor
      }
    }
  }
}