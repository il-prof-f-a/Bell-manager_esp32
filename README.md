# Bell-manager_esp32
Bell manager for work or school environment managed by web interface with ESP32 based device

Si prenda spunto da:
https://bangertech.de/sonoff-pow-elite/
https://github.com/arendst/Tasmota/issues/15856

#include <Arduino.h>
// Inserisci qui la libreria del display che usi (SPI / I2C / grafico / testo)
// Es: #include <Adafruit_SSD1306.h> oppure la libreria che identifica il controller del display

// --- Configurazione PIN (da modificare con i pin reali) ---
const int PIN_RELAY = 15;      // pin che pilota il relè (via transistor)
const int PIN_LED_STATUS = 2;  // LED di stato
const int PIN_BTN = 0;         // pulsante (es. POWER)
  
// Pin del display (esempi generici)
const int PIN_LCD_DC = 21;
const int PIN_LCD_CS = 22;
const int PIN_LCD_CLK = 18;
const int PIN_LCD_MOSI = 23;
// etc...

// --- Variabili di stato ---
bool relayState = false;

// — Per il debounce del pulsante —
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
bool lastBtnState = LOW;
bool btnState = LOW;

// --- Setup ---
void setup() {
  Serial.begin(115200);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);  // relè inizialmente OFF

  pinMode(PIN_LED_STATUS, OUTPUT);
  digitalWrite(PIN_LED_STATUS, LOW);

  pinMode(PIN_BTN, INPUT_PULLUP);  // se il pulsante è attivo su GND

  // Inizializza display
  // display.begin();
  // display.clear();
  // display.setTextSize(1);
  // display.setTextColor(WHITE);

  // Mostra qualcosa all’avvio
  // display.setCursor(0, 0);
  // display.println("Init...");
  // display.display();
}

// --- Loop principale ---
void loop() {
  // 1) Gestione pulsante / toggle relè
  int reading = digitalRead(PIN_BTN);
  if (reading != lastBtnState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != btnState) {
      btnState = reading;
      if (btnState == LOW) {
        // pulsante premuto (se è pull-up)
        relayState = !relayState;
        digitalWrite(PIN_RELAY, relayState ? HIGH : LOW);
      }
    }
  }
  lastBtnState = reading;

  // 2) LED di stato
  if (relayState) {
    digitalWrite(PIN_LED_STATUS, HIGH);
  } else {
    digitalWrite(PIN_LED_STATUS, LOW);
  }

  // 3) Lettura sensore / calcolo misure (corrente, tensione, energia)
  // Dovresti avere funzioni che leggono ADC, converti con fattori, ecc.
  float voltage = 230.0;   // placeholder
  float current = 1.23;    // placeholder
  float power = voltage * current;
  float energyWh = power * (millis()/1000.0/3600.0);

  // 4) Aggiornamento display (testo)
  // display.clear();
  // display.setCursor(0, 0);
  // display.printf("V: %.1f V\n", voltage);
  // display.printf("I: %.3f A\n", current);
  // display.printf("P: %.1f W\n", power);
  // display.printf("E: %.3f kWh\n", energyWh / 1000.0);
  // display.display();

  // 5) Delay / scheduling (oppure task / timer)
  delay(500);
}
