#ifndef TM1621_DISPLAY_H
#define TM1621_DISPLAY_H

#include <Arduino.h>
#include "config.h"

// ============================================
// Driver Display LCD TM1621 v8.0
// Basato su ESPEasy P148_data_struct.cpp
// https://github.com/letscontrolit/ESPEasy
//
// POWR316D: DA=GPIO14, CS=GPIO25, RD=GPIO26, WR=GPIO27
// ============================================

// Comandi TM1621
#define TM1621_SYS_EN     0x01
#define TM1621_LCD_OFF    0x02
#define TM1621_LCD_ON     0x03
#define TM1621_TIMER_DIS  0x04
#define TM1621_WDT_DIS    0x05
#define TM1621_TONE_OFF   0x08
#define TM1621_BIAS       0x29   // 1/3 bias 4 commons
#define TM1621_IRQ_DIS    0x80

#define TM1621_PULSE_WIDTH 10  // microseconds

// Comandi init (ESPEasy P148 exact)
static const uint8_t tm1621_commands[] = {
    TM1621_SYS_EN, TM1621_LCD_ON, TM1621_BIAS,
    TM1621_TIMER_DIS, TM1621_WDT_DIS, TM1621_TONE_OFF, TM1621_IRQ_DIS
};

// =============================================
// FONT TABLES - ESPEasy P148 ESATTO
// Due tabelle separate pre-calcolate per Row 1 e Row 2
// =============================================

// HEX bit values per segment
// Row 1              Row 2
//        1                  80
//      -----              -----
//     |     |            |     |
//    2|     |10        40|     |4
//     |     |            |     |
//      --20-              --2--
//     |     |            |     |
//    4|     |40        20|     |1
//     |     |            |     |
//      -----              -----
//        8                  10

//                                                     0     1     2     3     4     5     6     7     8     9     -
static const uint8_t tm1621_digit_row[2][11] = { { 0x5F, 0x50, 0x3D, 0x79, 0x72, 0x6B, 0x6F, 0x51, 0x7F, 0x7B, 0x20 },
                                                 { 0xF5, 0x05, 0xB6, 0x97, 0x47, 0xD3, 0xF3, 0x85, 0xF7, 0xD7, 0x02 } };

//                                                     'A'   'b'   'C'   'd'   'E'   'F'   'G'   'H'   'i'   'J'   'K'   'L'   'M'   'n'   'o'   'P'   'q'   'r'   'S'   't'   'U'   'v'   'W'   'X'   'Y'   'Z'   '?'
static const uint8_t tm1621_char_row[2][27] = { { 0x77, 0x6E, 0x0F, 0x7C, 0x2F, 0x27, 0x4f, 0x76, 0x40, 0x5C, 0x00, 0x0E, 0x00, 0x64, 0x6C, 0x37, 0x73, 0x24, 0x6B, 0x2E, 0x5E, 0x4C, 0x00, 0x00, 0x7A, 0x00, 0x35 },
                                                { 0xE7, 0x73, 0xF0, 0x37, 0xF2, 0xE2, 0xF1, 0x67, 0x01, 0x35, 0x00, 0x70, 0x00, 0x23, 0x33, 0xE6, 0xC7, 0x22, 0xD3, 0x72, 0x75, 0x31, 0x00, 0x00, 0x57, 0x00, 0xA6 } };

// =============================================
// STRUCT STATO
// =============================================
static struct {
    char     row[2][12];
    bool     celsius;
    bool     fahrenheit;
    bool     humidity;
    bool     voltage;
    bool     kwh;
    bool     present;
    bool     initialized;
    bool     lcd_on;
    bool     test_mode;
    uint32_t write_count;
    uint32_t cmd_count;
    uint8_t  last_buffer[8];
} Tm1621 = {};

// =============================================
// TM1621GetFontCharacter - ESPEasy P148 ESATTO
// =============================================
static uint8_t TM1621GetFontCharacter(char character, bool firstrow) {
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

    if ('a' <= c && c <= 'z') {
        return tm1621_char_row[row][c - 'a'];
    }

    // Spazio o carattere sconosciuto
    return 0u;
}

// =============================================
// bufferIndex - ESPEasy P148 ESATTO
// =============================================
static uint32_t bufferIndex(bool firstrow, uint32_t col) {
    return firstrow ? col : 7 - col;
}

// =============================================
// FUNZIONI BASSO LIVELLO - ESPEasy P148 ESATTO
// =============================================

// ESPEasy: TM1621WriteBit
static void TM1621WriteBit(bool value) {
    digitalWrite(PIN_LCD_WR, 0);
    digitalWrite(PIN_LCD_DATA, value ? 1 : 0);
    delayMicroseconds(TM1621_PULSE_WIDTH);
    digitalWrite(PIN_LCD_WR, 1);
    delayMicroseconds(TM1621_PULSE_WIDTH);
}

// ESPEasy: TM1621StartSequence
static void TM1621StartSequence(void) {
    digitalWrite(PIN_LCD_CS, 0);
    delayMicroseconds(TM1621_PULSE_WIDTH / 2);
}

// ESPEasy: TM1621StopSequence
static void TM1621StopSequence(void) {
    digitalWrite(PIN_LCD_CS, 1);
    delayMicroseconds(TM1621_PULSE_WIDTH / 2);
    digitalWrite(PIN_LCD_DATA, 1);
}

// ESPEasy: TM1621SendCmnd
static void TM1621SendCmnd(uint16_t command) {
    uint16_t full_command = (0x0400 | command) << 5; // 0b100cccccccc00000

    TM1621StartSequence();

    for (uint32_t i = 0; i < 12; i++) {
        TM1621WriteBit(full_command & 0x8000);
        full_command <<= 1;
    }
    TM1621StopSequence();
    Tm1621.cmd_count++;
}

// ESPEasy: TM1621SendAddress
static void TM1621SendAddress(uint16_t address) {
    uint16_t full_address = (address | 0x0140) << 7; // 0b101aaaaaa0000000

    TM1621StartSequence();

    for (uint32_t i = 0; i < 9; i++) {
        TM1621WriteBit(full_address & 0x8000);
        full_address <<= 1;
    }
}

// ESPEasy: TM1621SendCommon (LSB first)
static void TM1621SendCommon(uint8_t common) {
    for (uint32_t i = 0; i < 8; i++) {
        TM1621WriteBit(common & 1);
        common >>= 1;
    }
}

// ESPEasy: TM1621WritePixelBuffer
static void TM1621WritePixelBuffer(const uint8_t *buf, size_t size, uint16_t address) {
    TM1621SendAddress(address);

    for (uint32_t i = 0; i < size; i++) {
        TM1621SendCommon(buf[i]);
    }
    TM1621StopSequence();
}

// =============================================
// TM1621SendRows - ESPEasy P148 ESATTO
// =============================================

// Helper: prova a parsare una stringa come float
static bool tm1621_validFloat(const char* str, float& value) {
    if (!str || str[0] == '\0') return false;
    char* endptr;
    value = strtof(str, &endptr);
    return endptr != str && (*endptr == '\0' || *endptr == ' ');
}

static void TM1621SendRows(void) {
    uint8_t buffer[8] = { 0 };

    for (uint32_t j = 0; j < 2; j++) {
        const bool firstrow = (0 == j);
        float value = 0.0f;

        if (tm1621_validFloat(Tm1621.row[j], value)) {
            // Numero: gestisci come ESPEasy P148
            bool hadDot = false;
            for (size_t i = 0; i < 4 && !hadDot; ++i) {
                if (Tm1621.row[j][i] == '.') { hadDot = true; }
            }

            char row[4] = {};

            if (value > 9999.0f) { value = 9999.0f; }
            if (value < -999.0f) { value = -999.0f; }
            bool dot = false;

            if ((-99.9f < value) && (value < 999.9f)) {
                if (hadDot) {
                    dot = true;
                    value *= 10.0f;
                }
            }

            // Converti a intero e poi a stringa
            int intValue = (int)value;
            char value_str[12];
            snprintf(value_str, sizeof(value_str), "%d", intValue);
            size_t slen = strlen(value_str);
            size_t pos = 0;

            for (size_t i = (4 > slen ? 4 - slen : 0); i < 4 && pos < slen; ++i, ++pos) {
                row[i] = value_str[pos];
            }

            for (uint32_t i = 0; i < 4; i++) {
                buffer[bufferIndex(firstrow, i)] = TM1621GetFontCharacter(row[i], firstrow);
            }

            if (dot) {
                if (firstrow) {
                    buffer[2] |= 0x80; // Row 1 decimal point
                } else {
                    buffer[5] |= 0x08; // Row 2 decimal point
                }
            }
        } else {
            // Testo: renderizza carattere per carattere
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

    // Salva mirror per debug
    memcpy(Tm1621.last_buffer, buffer, 8);

    TM1621WritePixelBuffer(buffer, 8, 0x10); // Sonoff uses upper 16 segments

    Tm1621.write_count++;
}

// =============================================
// TM1621Init - ESPEasy P148 ESATTO
// =============================================

void tm1621_init(void) {
    Serial.println("[TM1621] ========== INIT v8.0 (ESPEasy P148) ==========");
    Serial.printf("[TM1621] Pins: DA=%d, CS=%d, RD=%d, WR=%d\n",
                  PIN_LCD_DATA, PIN_LCD_CS, PIN_LCD_RD, PIN_LCD_WR);
    Serial.printf("[TM1621] Pulse: %d us, BIAS: 0x%02X\n", TM1621_PULSE_WIDTH, TM1621_BIAS);

    // Pin setup
    pinMode(PIN_LCD_DATA, OUTPUT);
    digitalWrite(PIN_LCD_DATA, 1);
    pinMode(PIN_LCD_CS, OUTPUT);
    digitalWrite(PIN_LCD_CS, 1);
    pinMode(PIN_LCD_RD, OUTPUT);
    digitalWrite(PIN_LCD_RD, 1);
    pinMode(PIN_LCD_WR, OUTPUT);
    digitalWrite(PIN_LCD_WR, 1);

    // Init sequence (ESPEasy P148 TM1621Init esatto)
    digitalWrite(PIN_LCD_CS, 0);
    delayMicroseconds(80);
    digitalWrite(PIN_LCD_RD, 0);
    delayMicroseconds(15);
    digitalWrite(PIN_LCD_WR, 0);
    delayMicroseconds(25);
    digitalWrite(PIN_LCD_DATA, 0);
    delayMicroseconds(TM1621_PULSE_WIDTH);
    digitalWrite(PIN_LCD_DATA, 1);

    constexpr uint32_t nr_commands = sizeof(tm1621_commands) / sizeof(tm1621_commands[0]);
    for (uint32_t command = 0; command < nr_commands; command++) {
        TM1621SendCmnd(tm1621_commands[command]);
    }

    // Clear entire display buffer
    TM1621SendAddress(0x00);
    for (uint32_t segment = 0; segment < 16; segment++) {
        TM1621SendCommon(0);
    }
    TM1621StopSequence();

    snprintf(Tm1621.row[0], sizeof(Tm1621.row[0]), "----");
    snprintf(Tm1621.row[1], sizeof(Tm1621.row[1]), "----");
    TM1621SendRows();

    Tm1621.initialized = true;
    Tm1621.lcd_on = true;
    Tm1621.present = true;

    Serial.println("[TM1621] ========== INIT COMPLETE ==========");
    Serial.printf("[TM1621] Buffer: %02X %02X %02X %02X %02X %02X %02X %02X\n",
                  Tm1621.last_buffer[0], Tm1621.last_buffer[1],
                  Tm1621.last_buffer[2], Tm1621.last_buffer[3],
                  Tm1621.last_buffer[4], Tm1621.last_buffer[5],
                  Tm1621.last_buffer[6], Tm1621.last_buffer[7]);
}

void tm1621_reinit(void) {
    Tm1621.initialized = false;
    Tm1621.lcd_on = false;
    delay(100);
    tm1621_init();
}

// =============================================
// FUNZIONI ALTO LIVELLO
// =============================================

void tm1621_write_string(bool firstrow, const char* str) {
    strncpy(Tm1621.row[firstrow ? 0 : 1], str, sizeof(Tm1621.row[0]) - 1);
    Tm1621.row[firstrow ? 0 : 1][sizeof(Tm1621.row[0]) - 1] = '\0';
    TM1621SendRows();
}

void tm1621_write_strings(const char* str1, const char* str2) {
    strncpy(Tm1621.row[0], str1, sizeof(Tm1621.row[0]) - 1);
    Tm1621.row[0][sizeof(Tm1621.row[0]) - 1] = '\0';
    strncpy(Tm1621.row[1], str2, sizeof(Tm1621.row[1]) - 1);
    Tm1621.row[1][sizeof(Tm1621.row[1]) - 1] = '\0';
    TM1621SendRows();
}

void tm1621_write_float(bool firstrow, float value) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%.1f", value);
    tm1621_write_string(firstrow, buf);
}

void tm1621_write_floats(float value1, float value2) {
    char buf1[12], buf2[12];
    snprintf(buf1, sizeof(buf1), "%.1f", value1);
    snprintf(buf2, sizeof(buf2), "%.1f", value2);
    tm1621_write_strings(buf1, buf2);
}

// Write raw 8 bytes
void tm1621_write_raw(const uint8_t* data, uint8_t len) {
    if (len > 8) len = 8;
    memcpy(Tm1621.last_buffer, data, len);
    TM1621WritePixelBuffer(data, len, 0x10);
    Tm1621.write_count++;
}

// Write raw from uint64_t (ESPEasy P148 writeRawData)
void tm1621_write_raw(uint64_t rawdata) {
    uint8_t buffer[8] = { 0 };

    for (uint32_t j = 0; j < 2; j++) {
        for (uint32_t i = 0; i < 4; i++) {
            buffer[bufferIndex((0 == j), i)] = ((rawdata >> 56) & 0xFF);
            rawdata <<= 8;
        }
    }

    memcpy(Tm1621.last_buffer, buffer, 8);
    TM1621WritePixelBuffer(buffer, 8, 0x10);
    Tm1621.write_count++;
}

// =============================================
// UNITA' DI MISURA
// =============================================
enum Tm1621UnitOfMeasure {
    TM1621_UNIT_NONE = 0,
    TM1621_UNIT_CELSIUS,
    TM1621_UNIT_FAHRENHEIT,
    TM1621_UNIT_HUMIDITY,
    TM1621_UNIT_VOLT_AMP,
    TM1621_UNIT_KWH_WATT
};

void tm1621_set_unit(Tm1621UnitOfMeasure unit, bool firstrow) {
    switch (unit) {
        case TM1621_UNIT_NONE:
            if (firstrow) { Tm1621.celsius = false; Tm1621.fahrenheit = false; Tm1621.voltage = false; Tm1621.kwh = false; }
            else { Tm1621.humidity = false; }
            break;
        case TM1621_UNIT_CELSIUS:
            if (firstrow) { Tm1621.celsius = true; Tm1621.fahrenheit = false; Tm1621.voltage = false; Tm1621.kwh = false; }
            break;
        case TM1621_UNIT_FAHRENHEIT:
            if (firstrow) { Tm1621.fahrenheit = true; Tm1621.celsius = false; Tm1621.voltage = false; Tm1621.kwh = false; }
            break;
        case TM1621_UNIT_KWH_WATT:
            Tm1621.kwh = true;
            if (firstrow) { Tm1621.celsius = false; Tm1621.fahrenheit = false; Tm1621.voltage = false; }
            else { Tm1621.humidity = false; }
            break;
        case TM1621_UNIT_HUMIDITY:
            if (!firstrow) { Tm1621.humidity = true; Tm1621.voltage = false; Tm1621.kwh = false; }
            break;
        case TM1621_UNIT_VOLT_AMP:
            Tm1621.voltage = true; Tm1621.kwh = false;
            if (firstrow) { Tm1621.celsius = false; Tm1621.fahrenheit = false; }
            else { Tm1621.humidity = false; }
            break;
    }
}

void tm1621_clear_all_units() {
    Tm1621.celsius = false;
    Tm1621.fahrenheit = false;
    Tm1621.humidity = false;
    Tm1621.voltage = false;
    Tm1621.kwh = false;
}

// =============================================
// TEST MODE
// =============================================

void tm1621_set_test_mode(bool enable) {
    Tm1621.test_mode = enable;
    Serial.printf("[TM1621] Test mode: %s\n", enable ? "ON" : "OFF");
}

bool tm1621_is_test_mode() {
    return Tm1621.test_mode;
}

// =============================================
// CONTROLLI LCD
// =============================================

void tm1621_lcd_on_cmd() {
    TM1621SendCmnd(TM1621_LCD_ON);
    Tm1621.lcd_on = true;
}

void tm1621_lcd_off_cmd() {
    TM1621SendCmnd(TM1621_LCD_OFF);
    Tm1621.lcd_on = false;
}

// =============================================
// TEST/DEBUG
// =============================================

void tm1621_test_all_on() {
    uint8_t buffer[8];
    memset(buffer, 0xFF, 8);
    tm1621_write_raw(buffer, 8);
}

void tm1621_test_all_off() {
    uint8_t buffer[8];
    memset(buffer, 0x00, 8);
    tm1621_write_raw(buffer, 8);
}

// =============================================
// GETTERS
// =============================================

bool tm1621_is_initialized() { return Tm1621.initialized; }
bool tm1621_is_lcd_on() { return Tm1621.lcd_on; }
uint32_t tm1621_get_write_count() { return Tm1621.write_count; }
uint32_t tm1621_get_cmd_count() { return Tm1621.cmd_count; }

String tm1621_get_row(int row) {
    if (row < 0 || row > 1) return "";
    return String(Tm1621.row[row]);
}

String tm1621_get_buffer_hex() {
    char buf[32];
    snprintf(buf, sizeof(buf), "%02X%02X%02X%02X %02X%02X%02X%02X",
             Tm1621.last_buffer[0], Tm1621.last_buffer[1],
             Tm1621.last_buffer[2], Tm1621.last_buffer[3],
             Tm1621.last_buffer[4], Tm1621.last_buffer[5],
             Tm1621.last_buffer[6], Tm1621.last_buffer[7]);
    return String(buf);
}

String tm1621_get_units_str() {
    String s;
    if (Tm1621.celsius) s += "C ";
    if (Tm1621.fahrenheit) s += "F ";
    if (Tm1621.humidity) s += "%RH ";
    if (Tm1621.voltage) s += "V/A ";
    if (Tm1621.kwh) s += "kWh/W ";
    if (s.length() == 0) s = "none";
    return s;
}

void tm1621_print_status() {
    Serial.println("[TM1621] === STATUS ===");
    Serial.printf("  Driver: ESPEasy P148 v8.0\n");
    Serial.printf("  Initialized: %s\n", Tm1621.initialized ? "YES" : "NO");
    Serial.printf("  LCD ON: %s\n", Tm1621.lcd_on ? "YES" : "NO");
    Serial.printf("  Test mode: %s\n", Tm1621.test_mode ? "YES" : "NO");
    Serial.printf("  Pulse: %d us\n", TM1621_PULSE_WIDTH);
    Serial.printf("  Row 0: '%s'\n", Tm1621.row[0]);
    Serial.printf("  Row 1: '%s'\n", Tm1621.row[1]);
    Serial.printf("  Units: %s\n", tm1621_get_units_str().c_str());
    Serial.printf("  Buffer: %02X %02X %02X %02X %02X %02X %02X %02X\n",
                  Tm1621.last_buffer[0], Tm1621.last_buffer[1],
                  Tm1621.last_buffer[2], Tm1621.last_buffer[3],
                  Tm1621.last_buffer[4], Tm1621.last_buffer[5],
                  Tm1621.last_buffer[6], Tm1621.last_buffer[7]);
    Serial.printf("  Writes: %lu, Cmds: %lu\n", Tm1621.write_count, Tm1621.cmd_count);
}

// Forza re-invio delle righe correnti
void tm1621_refresh() {
    TM1621SendRows();
}

// =============================================
// FUNZIONI COMPATIBILITA' CON MAIN SKETCH
// =============================================

void initDisplay() { tm1621_init(); }
void reinitDisplay() { tm1621_reinit(); }
void displayOn() { tm1621_lcd_on_cmd(); }
void displayOff() { tm1621_lcd_off_cmd(); }
bool isDisplayInitialized() { return Tm1621.initialized; }
bool isDisplayOn() { return Tm1621.lcd_on; }
uint32_t getDisplayUpdateCount() { return Tm1621.write_count; }
String getDisplayContent() { return String(Tm1621.row[0]) + "|" + String(Tm1621.row[1]); }
String getDisplayBufferHex() { return tm1621_get_buffer_hex(); }
String getDisplayParams() {
    return String("ESPEasy P148 v8, BIAS=0x29, pulse=10us");
}
void printDisplayStatus() { tm1621_print_status(); }

void clearDisplay() {
    if (Tm1621.test_mode) return;
    tm1621_clear_all_units();
    tm1621_write_strings("    ", "    ");
}

void displayAllOn() {
    if (Tm1621.test_mode) return;
    tm1621_test_all_on();
}

void displayTime(uint8_t hour, uint8_t minute) {
    if (Tm1621.test_mode) return;
    tm1621_clear_all_units();
    char buf1[12], buf2[12];
    snprintf(buf1, sizeof(buf1), "%2d", hour);
    snprintf(buf2, sizeof(buf2), "%2d", minute);
    tm1621_write_strings(buf1, buf2);
}

void displayLoading() {
    if (Tm1621.test_mode) return;
    tm1621_clear_all_units();
    tm1621_write_strings("----", "----");
}

void displayBell() {
    if (Tm1621.test_mode) return;
    tm1621_clear_all_units();
    tm1621_write_strings("bELL", "    ");
}

void displayConnecting() {
    if (Tm1621.test_mode) return;
    tm1621_clear_all_units();
    tm1621_write_strings("Conn", "    ");
}

void displayNumber(uint8_t row, int value, uint8_t decimals) {
    if (Tm1621.test_mode) return;
    bool firstrow = (row == 0);
    if (decimals > 0) {
        tm1621_write_float(firstrow, value / pow(10, decimals));
    } else {
        char buf[12];
        snprintf(buf, sizeof(buf), "%d", value);
        tm1621_write_string(firstrow, buf);
    }
}

#endif // TM1621_DISPLAY_H
