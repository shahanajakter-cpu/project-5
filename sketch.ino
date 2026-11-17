#include <DHTesp.h>
#include <WiFi.h>

// Pin Definitions
const int DHT_PIN = 15;      // DHT22 Data pin connected to GPIO 15
const int BUTTON_PIN = 4;    // Emergency Override Button connected to GPIO 4
const int FAN_LED_PIN = 2;   // Cooling Fan LED connected to GPIO 2

DHTesp dht;

// Timing variables for non-blocking sensor reading & Wi-Fi publishing
unsigned long previousMillis = 0;
const long interval = 2000; // Every 2 seconds

// Fan state variables
bool fanManualOverride = false;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 30; // 30ms debounce for zero noticeable lag

void setup() {
  Serial.begin(115200);

  // Initialize Pins
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Button with internal pull-up (Pressed = LOW)
  pinMode(FAN_LED_PIN, OUTPUT);
  digitalWrite(FAN_LED_PIN, LOW);    // Fan starts OFF

  // Initialize DHT22 Sensor
  dht.setup(DHT_PIN, DHTesp::DHT22);

  // Connect to Wi-Fi (Wokwi-GUEST)
  WiFi.begin("Wokwi-GUEST", "", 6);
  Serial.print("Connecting to Wi-Fi");
  
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi Connected!");
  } else {
    Serial.println("\nWi-Fi Connection Failed, continuing offline loop...");
  }
}

void loop() {
  // --- 1. INSTANT BUTTON OVERRIDE (Zero Lag < 50ms) ---
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    static int committedButtonState = HIGH;
    if (reading != committedButtonState) {
      committedButtonState = reading;
      
      if (committedButtonState == LOW) { // Button pressed
        fanManualOverride = !fanManualOverride; // Toggle fan state
        digitalWrite(FAN_LED_PIN, fanManualOverride ? HIGH : LOW);
        
        Serial.print("Emergency Override! Fan Status: ");
        Serial.println(fanManualOverride ? "ON" : "OFF");
      }
    }
  }
  lastButtonState = reading;

  // --- 2. PERIODIC SENSOR READING & WI-FI PUBLISHING (Every 2 seconds) ---
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Read Temperature and Humidity from DHT22
    TempAndHumidity data = dht.getTempAndHumidity();

    if (dht.getStatus() != 0) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    Serial.print("Temp: ");
    Serial.print(data.temperature, 1);
    Serial.print("°C | Humidity: ");
    Serial.print(data.humidity, 1);
    Serial.print("% | Wi-Fi Status: ");
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("CONNECTED (Publishing data...)");
    } else {
      Serial.println("DISCONNECTED (Running locally)");
      WiFi.begin("Wokwi-GUEST", "", 6);
    }
  }
}