// ============================================
// TEST DISPLAY TM1621 - Standalone v2
// Copia ESATTA di ESPEasy P148 + init COMPLETA di tutti i GPIO
//
// Target: Sonoff POWR316D (ESP32)
// ============================================

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

// === WiFi - MODIFICA QUI ===
const char* WIFI_SSID = "ASUS_RiceWLan";
const char* WIFI_PASS = "pippoplutopaperinominnie";

// === TUTTI i GPIO del POWR316D (come nel firmware principale) ===
#define PIN_BUTTON      0   // Pulsante (active LOW, BOOT pin)
#define PIN_LED_WIFI    5   // LED WiFi (active LOW)
#define PIN_RELAY       13  // Relay 16A (active HIGH)
#define PIN_LCD_DATA    14  // TM1621 Data
#define PIN_CSE_RX      16  // CSE7759B RX (non usato ma presente)
#define PIN_LED_RELAY   18  // LED relay (active LOW)
#define PIN_LCD_CS      25  // TM1621 Chip Select
#define PIN_LCD_RD      26  // TM1621 Read
#define PIN_LCD_WR      27  // TM1621 Write (clock)

// === Comandi TM1621 (identici ESPEasy P148) ===
#define TM1621_PULSE_WIDTH  10  // microseconds

#define TM1621_SYS_EN      0x01
#define TM1621_LCD_OFF      0x02
#define TM1621_LCD_ON       0x03
#define TM1621_TIMER_DIS    0x04
#define TM1621_WDT_DIS      0x05
#define TM1621_TONE_OFF     0x08
#define TM1621_BIAS         0x29  // 1/3 bias, 4 commons
#define TM1621_IRQ_DIS      0x80

const uint8_t tm1621_commands[] = {
    TM1621_SYS_EN, TM1621_LCD_ON, TM1621_BIAS,
    TM1621_TIMER_DIS, TM1621_WDT_DIS, TM1621_TONE_OFF, TM1621_IRQ_DIS
};

// === Stato display ===
struct {
    char row[2][12];
    bool celsius;
    bool fahrenheit;
    bool humidity;
    bool voltage;
    bool kwh;
} Tm1621 = {};

// === Font IDENTICHE a ESPEasy P148 ===
const uint8_t tm1621_digit_row[2][11] = {
    { 0x5F, 0x50, 0x3D, 0x79, 0x72, 0x6B, 0x6F, 0x51, 0x7F, 0x7B, 0x20 },
    { 0xF5, 0x05, 0xB6, 0x97, 0x47, 0xD3, 0xF3, 0x85, 0xF7, 0xD7, 0x02 }
};

const uint8_t tm1621_char_row[2][27] = {
    { 0x77, 0x6E, 0x0F, 0x7C, 0x2F, 0x27, 0x4f, 0x76, 0x40, 0x5C, 0x00, 0x0E, 0x00, 0x64, 0x6C, 0x37, 0x73, 0x24, 0x6B, 0x2E, 0x5E, 0x4C, 0x00, 0x00, 0x7A, 0x00, 0x35 },
    { 0xE7, 0x73, 0xF0, 0x37, 0xF2, 0xE2, 0xF1, 0x67, 0x01, 0x35, 0x00, 0x70, 0x00, 0x23, 0x33, 0xE6, 0xC7, 0x22, 0xD3, 0x72, 0x75, 0x31, 0x00, 0x00, 0x57, 0x00, 0xA6 }
};

// =============================================
// TM1621GetFontCharacter - COPIA ESATTA ESPEasy P148
// =============================================
uint8_t TM1621GetFontCharacter(char character, bool firstrow) {
    const char c = tolower(character);
    const uint32_t row = firstrow ? 0 : 1;

    if (isdigit(c) || (c == '-')) {
        if (c == '-') {
            return tm1621_digit_row[row][10];
        } else {
            return tm1621_digit_row[row][c - '0'];
        }
    }
    if (c == '?') { return tm1621_char_row[row][26]; }
    if (('a' <= c) && (c <= 'z')) {
        return tm1621_char_row[row][c - 'a'];
    }
    return 0;
}

// =============================================
// Funzioni basso livello - COPIA ESATTA ESPEasy P148
// =============================================

void TM1621WriteBit(bool value) {
    digitalWrite(PIN_LCD_WR, 0);
    digitalWrite(PIN_LCD_DATA, value ? 1 : 0);
    delayMicroseconds(TM1621_PULSE_WIDTH);
    digitalWrite(PIN_LCD_WR, 1);
    delayMicroseconds(TM1621_PULSE_WIDTH);
}

void TM1621StartSequence() {
    digitalWrite(PIN_LCD_CS, 0);
    delayMicroseconds(TM1621_PULSE_WIDTH / 2);
}

void TM1621StopSequence() {
    digitalWrite(PIN_LCD_CS, 1);
    delayMicroseconds(TM1621_PULSE_WIDTH / 2);
    digitalWrite(PIN_LCD_DATA, 1);
}

void TM1621SendCmnd(uint16_t command) {
    uint16_t full_command = (0x0400 | command) << 5;
    TM1621StartSequence();
    for (uint32_t i = 0; i < 12; i++) {
        TM1621WriteBit(full_command & 0x8000);
        full_command <<= 1;
    }
    TM1621StopSequence();
}

void TM1621SendAddress(uint16_t address) {
    uint16_t full_address = (address | 0x0140) << 7;
    TM1621StartSequence();
    for (uint32_t i = 0; i < 9; i++) {
        TM1621WriteBit(full_address & 0x8000);
        full_address <<= 1;
    }
}

void TM1621SendCommon(uint8_t common) {
    for (uint32_t i = 0; i < 8; i++) {
        TM1621WriteBit(common & 1);
        common >>= 1;
    }
}

void TM1621WritePixelBuffer(const uint8_t *buf, size_t size, uint16_t address) {
    TM1621SendAddress(address);
    for (uint32_t i = 0; i < size; i++) {
        TM1621SendCommon(buf[i]);
    }
    TM1621StopSequence();
}

uint32_t bufferIndex(bool firstrow, uint32_t col) {
    return firstrow ? col : 7 - col;
}

// =============================================
// TM1621SendRows - COPIA ESATTA ESPEasy P148
// =============================================
bool myValidFloat(const char* str, float &value) {
    char *endptr;
    float v = strtof(str, &endptr);
    if (endptr == str || *endptr != '\0') return false;
    value = v;
    return true;
}

void TM1621SendRows() {
    uint8_t buffer[8] = { 0 };

    for (uint32_t j = 0; j < 2; j++) {
        const bool firstrow = (0 == j);
        float value = 0.0f;

        if (myValidFloat(Tm1621.row[j], value)) {
            bool hadDot = false;
            for (size_t i = 0; i < 4 && !hadDot; ++i) {
                if (Tm1621.row[j][i] == '.') { hadDot = true; }
            }
            char row[4] = {};
            if (value > 9999.0f) { value = 9999.0f; }
            if (value < -999.0f) { value = -999.0f; }
            bool dot = false;
            if ((-99.9f < value) && (value < 999.9f)) {
                if (hadDot) { dot = true; value *= 10.0f; }
            }
            String value_str(static_cast<int>(value));
            size_t slen = value_str.length();
            size_t pos = 0;
            for (size_t i = 4 - slen; i < 4 && pos < slen; ++i, ++pos) {
                row[i] = value_str[pos];
            }
            for (uint32_t i = 0; i < 4; i++) {
                buffer[bufferIndex(firstrow, i)] = TM1621GetFontCharacter(row[i], firstrow);
            }
            if (dot) {
                if (firstrow) { buffer[2] |= 0x80; }
                else { buffer[5] |= 0x08; }
            }
        } else {
            for (uint32_t i = 0; i < 4; i++) {
                buffer[bufferIndex(firstrow, i)] = TM1621GetFontCharacter(Tm1621.row[j][i], firstrow);
            }
        }
    }

    if (Tm1621.fahrenheit) { buffer[1] |= 0x80; }
    if (Tm1621.celsius)    { buffer[3] |= 0x80; }
    if (Tm1621.kwh)        { buffer[4] |= 0x08; }
    if (Tm1621.humidity)   { buffer[6] |= 0x08; }
    if (Tm1621.voltage)    { buffer[7] |= 0x08; }

    Serial.printf("  Buffer: %02X %02X %02X %02X | %02X %02X %02X %02X\n",
                  buffer[0], buffer[1], buffer[2], buffer[3],
                  buffer[4], buffer[5], buffer[6], buffer[7]);

    TM1621WritePixelBuffer(buffer, 8, 0x10);
}

// =============================================
// TM1621Init - COPIA ESATTA ESPEasy P148
// =============================================
void TM1621Init() {
    digitalWrite(PIN_LCD_CS, 0);
    delayMicroseconds(80);
    digitalWrite(PIN_LCD_RD, 0);
    delayMicroseconds(15);
    digitalWrite(PIN_LCD_WR, 0);
    delayMicroseconds(25);
    digitalWrite(PIN_LCD_DATA, 0);
    delayMicroseconds(TM1621_PULSE_WIDTH);
    digitalWrite(PIN_LCD_DATA, 1);

    for (uint32_t command = 0; command < sizeof(tm1621_commands); command++) {
        TM1621SendCmnd(tm1621_commands[command]);
    }

    TM1621SendAddress(0x00);
    for (uint32_t segment = 0; segment < 16; segment++) {
        TM1621SendCommon(0);
    }
    TM1621StopSequence();
}

// =============================================
// Funzioni helper
// =============================================

void writeStrings(const char* str1, const char* str2) {
    strncpy(Tm1621.row[0], str1, sizeof(Tm1621.row[0]) - 1);
    Tm1621.row[0][sizeof(Tm1621.row[0]) - 1] = '\0';
    strncpy(Tm1621.row[1], str2, sizeof(Tm1621.row[1]) - 1);
    Tm1621.row[1][sizeof(Tm1621.row[1]) - 1] = '\0';
    TM1621SendRows();
}

void writeRaw(uint8_t val) {
    uint8_t buffer[8];
    memset(buffer, val, 8);
    TM1621WritePixelBuffer(buffer, 8, 0x10);
    Serial.printf("  Raw: all 0x%02X\n", val);
}

void clearUnits() {
    Tm1621.celsius = false;
    Tm1621.fahrenheit = false;
    Tm1621.humidity = false;
    Tm1621.voltage = false;
    Tm1621.kwh = false;
}

// =============================================
// DIAGNOSTICA GPIO
// Verifica che ogni pin funzioni realmente
// =============================================
void testGPIO() {
    Serial.println("\n[DIAG] === TEST GPIO ===");
    Serial.println("[DIAG] Leggo stato attuale di ogni pin...");

    int pins[] = { PIN_BUTTON, PIN_LED_WIFI, PIN_RELAY, PIN_LCD_DATA,
                   PIN_CSE_RX, PIN_LED_RELAY, PIN_LCD_CS, PIN_LCD_RD, PIN_LCD_WR };
    const char* names[] = { "GPIO0  BTN", "GPIO5  LED_WIFI", "GPIO13 RELAY",
                            "GPIO14 LCD_DATA", "GPIO16 CSE_RX", "GPIO18 LED_RELAY",
                            "GPIO25 LCD_CS", "GPIO26 LCD_RD", "GPIO27 LCD_WR" };

    for (int i = 0; i < 9; i++) {
        Serial.printf("  %s = %d\n", names[i], digitalRead(pins[i]));
    }

    // Test toggle dei 4 pin display
    Serial.println("[DIAG] Toggle pin display LOW-HIGH-LOW (500ms ciascuno)...");
    int lcd_pins[] = { PIN_LCD_DATA, PIN_LCD_CS, PIN_LCD_RD, PIN_LCD_WR };
    const char* lcd_names[] = { "DATA(14)", "CS(25)", "RD(26)", "WR(27)" };

    for (int i = 0; i < 4; i++) {
        Serial.printf("  Toggle %s: LOW...", lcd_names[i]);
        digitalWrite(lcd_pins[i], LOW);
        delay(500);
        Serial.printf(" read=%d, HIGH...", digitalRead(lcd_pins[i]));
        digitalWrite(lcd_pins[i], HIGH);
        delay(500);
        Serial.printf(" read=%d OK\n", digitalRead(lcd_pins[i]));
    }

    Serial.println("[DIAG] === TEST GPIO COMPLETATO ===\n");
}

// =============================================
// SEQUENZA TEST: 12 schermate
// =============================================
#define NUM_TESTS 12

void runTest(int testNum) {
    clearUnits();
    Serial.printf("\n=== TEST %d/%d ===\n", testNum + 1, NUM_TESTS);

    switch (testNum) {
        case 0:
            Serial.println("ALL ON - tutti i segmenti (0xFF)");
            writeRaw(0xFF);
            return;
        case 1:
            Serial.println("ALL OFF - nessun segmento (0x00)");
            writeRaw(0x00);
            return;
        case 2:
            // QUESTO mostrava la "mezza H" nel firmware principale
            Serial.println("*** Row1='223' Row2='----' (prima mostrava mezza H)");
            writeStrings("223", "----");
            break;
        case 3:
            // QUESTO mostrava la "C" nel firmware principale
            Serial.println("*** Row1='23.8' Row2='----' (prima mostrava C)");
            writeStrings("23.8", "----");
            break;
        case 4:
            Serial.println("Row1='8888' Row2='8888'");
            writeStrings("8888", "8888");
            break;
        case 5:
            Serial.println("Row1='1234' Row2='5678'");
            writeStrings("1234", "5678");
            break;
        case 6:
            Serial.println("Row1='23.8' Row2='45.6'");
            writeStrings("23.8", "45.6");
            break;
        case 7:
            Serial.println("Row1='0' Row2='0'");
            writeStrings("0", "0");
            break;
        case 8:
            Serial.println("Row1='----' Row2='----'");
            writeStrings("----", "----");
            break;
        case 9:
            Serial.println("Row1='HELP' Row2='End'");
            writeStrings("HELP", "End");
            break;
        case 10:
            Serial.println("Row1='22.0'+V/A Row2='150'+kWh");
            Tm1621.voltage = true;
            Tm1621.kwh = true;
            writeStrings("22.0", "150");
            break;
        case 11:
            Serial.println("Row1='25.3'+C Row2='60'+%RH");
            Tm1621.celsius = true;
            Tm1621.humidity = true;
            writeStrings("25.3", "60");
            break;
    }
}

// =============================================
// SETUP & LOOP
// =============================================

int currentTest = 0;
unsigned long lastSwitch = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("=============================================");
    Serial.println("  TM1621 TEST v2 - Full GPIO init");
    Serial.println("  Sonoff POWR316D");
    Serial.println("=============================================");

    // ========================================
    // INIT TUTTI I GPIO - IDENTICO al firmware principale
    // (gpio_control.h :: initGPIO())
    // ========================================
    Serial.println("[GPIO] Init TUTTI i pin (come firmware principale)...");

    // Pulsante (input con pull-up, active LOW)
    pinMode(PIN_BUTTON, INPUT_PULLUP);

    // LED WiFi (output, active LOW)
    pinMode(PIN_LED_WIFI, OUTPUT);
    digitalWrite(PIN_LED_WIFI, HIGH);  // Spento

    // Relay (output, active HIGH)
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);  // Spento

    // LED Relay (output, active LOW)
    pinMode(PIN_LED_RELAY, OUTPUT);
    digitalWrite(PIN_LED_RELAY, HIGH);  // Spento

    Serial.println("[GPIO] Button=INPUT_PULLUP, LED_WIFI=HIGH, RELAY=LOW, LED_RELAY=HIGH");

    // ========================================
    // INIT PIN DISPLAY TM1621
    // (come ESPEasy P148 constructor)
    // ========================================
    pinMode(PIN_LCD_DATA, OUTPUT);
    digitalWrite(PIN_LCD_DATA, 1);
    pinMode(PIN_LCD_CS, OUTPUT);
    digitalWrite(PIN_LCD_CS, 1);
    pinMode(PIN_LCD_RD, OUTPUT);
    digitalWrite(PIN_LCD_RD, 1);
    pinMode(PIN_LCD_WR, OUTPUT);
    digitalWrite(PIN_LCD_WR, 1);

    Serial.println("[GPIO] LCD pins: DATA=1, CS=1, RD=1, WR=1");
    Serial.printf("  DA=GPIO%d, CS=GPIO%d, RD=GPIO%d, WR=GPIO%d\n",
                  PIN_LCD_DATA, PIN_LCD_CS, PIN_LCD_RD, PIN_LCD_WR);
    Serial.printf("  BIAS=0x%02X, PULSE=%d us\n", TM1621_BIAS, TM1621_PULSE_WIDTH);

    // ========================================
    // DIAGNOSTICA GPIO
    // ========================================
    testGPIO();

    // ========================================
    // INIT TM1621
    // ========================================
    delay(200);  // Power stabilization

    Serial.println("[INIT] Inizializzazione TM1621...");
    TM1621Init();
    Serial.println("[INIT] Completata.");

    // Mostra "----" iniziale
    snprintf(Tm1621.row[0], sizeof(Tm1621.row[0]), "----");
    snprintf(Tm1621.row[1], sizeof(Tm1621.row[1]), "----");
    TM1621SendRows();
    Serial.println("[INIT] Scritto '----' su entrambe le righe");

    // ========================================
    // WIFI + OTA
    // ========================================
    Serial.printf("[WIFI] Connessione a '%s'...\n", WIFI_SSID);
    writeStrings("Conn", "----");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 15000) {
        delay(500);
        Serial.print(".");
        digitalWrite(PIN_LED_WIFI, !digitalRead(PIN_LED_WIFI));  // Blink
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WIFI] Connesso! IP: %s\n", WiFi.localIP().toString().c_str());
        digitalWrite(PIN_LED_WIFI, LOW);  // LED acceso (active low)

        ArduinoOTA.setHostname("Bell-Manager-Test");
        ArduinoOTA.setPassword("bellmanager");
        ArduinoOTA.onStart([]() {
            Serial.println("[OTA] Inizio aggiornamento...");
        });
        ArduinoOTA.onEnd([]() {
            Serial.println("\n[OTA] Completato! Riavvio...");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("[OTA] %u%%\r", progress / (total / 100));
        });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("[OTA] Errore: %u\n", error);
        });
        ArduinoOTA.begin();
        Serial.println("[OTA] OTA attivo - hostname: Bell-Manager-Test");
    } else {
        Serial.println("[WIFI] Connessione fallita, OTA non disponibile");
        digitalWrite(PIN_LED_WIFI, HIGH);  // LED spento
    }

    // ========================================
    // AVVIO TEST
    // ========================================
    delay(1000);
    runTest(0);
    lastSwitch = millis();
}

void loop() {
    ArduinoOTA.handle();

    unsigned long now = millis();

    if (now - lastSwitch >= 3000) {
        lastSwitch = now;
        currentTest = (currentTest + 1) % NUM_TESTS;
        runTest(currentTest);
    }
}
