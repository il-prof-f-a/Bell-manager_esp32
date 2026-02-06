#ifndef TM1621_DISPLAY_H
#define TM1621_DISPLAY_H

#include <Arduino.h>
#include "config.h"

// ============================================
// Driver Display LCD TM1621 v5.0
// Basato su Tasmota xdrv_87_esp32_sonoff_tm1621.ino
// che FUNZIONA sul Sonoff POWR316D
//
// Riferimento: https://github.com/arendst/Tasmota
// ============================================

// =============================================
// COMANDI TM1621 (8 bit, senza mode prefix)
// =============================================
#define TM1621_SYS_DIS    0x00   // Disabilita oscillatore
#define TM1621_SYS_EN     0x01   // Abilita oscillatore
#define TM1621_LCD_OFF    0x02   // Spegne LCD bias
#define TM1621_LCD_ON     0x03   // Accende LCD bias
#define TM1621_TIMER_DIS  0x04   // Disabilita timer
#define TM1621_WDT_DIS    0x05   // Disabilita WDT
#define TM1621_TIMER_EN   0x06   // Abilita timer
#define TM1621_WDT_EN     0x07   // Abilita WDT
#define TM1621_TONE_OFF   0x08   // Disabilita tone
#define TM1621_TONE_ON    0x09   // Abilita tone
#define TM1621_CLR_TIMER  0x0C   // Clear timer
#define TM1621_CLR_WDT    0x0F   // Clear WDT
#define TM1621_XTAL_32K   0x14   // Crystal 32kHz
#define TM1621_RC_256K    0x18   // RC interno 256kHz
#define TM1621_EXT_256K   0x1C   // Clock esterno 256kHz
#define TM1621_BIAS_1_2_2 0x20   // 1/2 bias, 2 COM
#define TM1621_BIAS_1_2_3 0x24   // 1/2 bias, 3 COM
#define TM1621_BIAS_1_2_4 0x28   // 1/2 bias, 4 COM
#define TM1621_BIAS_1_3_2 0x21   // 1/3 bias, 2 COM
#define TM1621_BIAS_1_3_3 0x25   // 1/3 bias, 3 COM
#define TM1621_BIAS_1_3_4 0x29   // 1/3 bias, 4 COM (default Tasmota)
#define TM1621_TONE_4K    0x40   // Tone 4kHz
#define TM1621_TONE_2K    0x60   // Tone 2kHz
#define TM1621_IRQ_DIS    0x80   // Disabilita IRQ
#define TM1621_IRQ_EN     0x88   // Abilita IRQ

// =============================================
// PARAMETRI CONFIGURABILI
// =============================================

// Pulse width in microsecondi (Tasmota usa 10)
static uint8_t tm1621_pulse_us = 10;

// BIAS command (0x24 = 1/2 bias, 3 COM - funziona meglio su POWR316D)
static uint8_t tm1621_bias_cmd = TM1621_BIAS_1_2_3;

// Test mode - blocca auto-update
static bool tm1621_test_mode = false;

// =============================================
// STATO DISPLAY
// =============================================
static bool tm1621_initialized = false;
static bool tm1621_lcd_on = false;
static uint8_t tm1621_ram_mirror[32];  // Mirror RAM per debug
static uint32_t tm1621_write_count = 0;
static uint32_t tm1621_cmd_count = 0;
static uint8_t tm1621_last_addr = 0;
static uint8_t tm1621_last_count = 0;

// =============================================
// FUNZIONI DI BASSO LIVELLO (DA TASMOTA)
// =============================================

// Delay per timing
static inline void tm1621_delay() {
    delayMicroseconds(tm1621_pulse_us);
}

static inline void tm1621_delay_half() {
    delayMicroseconds(tm1621_pulse_us / 2);
}

// Stop sequence: CS HIGH, delay, DA HIGH
static void tm1621_stop() {
    digitalWrite(PIN_LCD_CS, HIGH);
    tm1621_delay_half();
    digitalWrite(PIN_LCD_DATA, HIGH);
}

// Invia bit MSB first (per comando e indirizzo)
// Usa 0x8000 e <<= perché comandi/indirizzi sono 16 bit allineati a sinistra
static void tm1621_send_msb(uint16_t data, uint8_t bits) {
    for (uint8_t i = 0; i < bits; i++) {
        digitalWrite(PIN_LCD_WR, LOW);
        if (data & 0x8000) {
            digitalWrite(PIN_LCD_DATA, HIGH);
        } else {
            digitalWrite(PIN_LCD_DATA, LOW);
        }
        tm1621_delay();
        digitalWrite(PIN_LCD_WR, HIGH);
        tm1621_delay();
        data <<= 1;
    }
}

// Invia byte LSB first (per dati RAM)
// Tasmota: TM1621SendCommon usa & 1 e >>= 1
static void tm1621_send_byte_lsb(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(PIN_LCD_WR, LOW);
        if (data & 1) {
            digitalWrite(PIN_LCD_DATA, HIGH);
        } else {
            digitalWrite(PIN_LCD_DATA, LOW);
        }
        tm1621_delay();
        digitalWrite(PIN_LCD_WR, HIGH);
        tm1621_delay();
        data >>= 1;
    }
}

// Invia comando al TM1621
// Tasmota: TM1621SendCmnd
// Formato: 0b100cccccccc00000 (12 bit trasmessi, MSB first)
void tm1621_send_command(uint8_t cmd) {
    // 0x0400 = 0b10000000000 -> mode "100" al bit 10
    // | cmd mette il comando nei bit 0-7
    // << 5 shifta tutto per allineare a sinistra in 16 bit
    uint16_t full_cmd = (0x0400 | cmd) << 5;

    digitalWrite(PIN_LCD_CS, LOW);
    tm1621_delay_half();
    tm1621_send_msb(full_cmd, 12);  // MSB first
    tm1621_stop();

    tm1621_cmd_count++;
    Serial.printf("[TM1621] CMD: 0x%02X (full: 0x%04X)\n", cmd, full_cmd);
}

// Invia indirizzo per scrittura RAM
// Tasmota: TM1621SendAddress
// Formato: 0b101aaaaaa0000000 (9 bit trasmessi, MSB first)
static void tm1621_send_address(uint8_t addr) {
    // 0x0140 = 0b101000000 -> mode "101" + 6 bit indirizzo
    // | (addr & 0x3F) mette l'indirizzo nei bit 0-5
    // << 7 shifta tutto per allineare a sinistra
    uint16_t full_addr = ((addr & 0x3F) | 0x0140) << 7;

    digitalWrite(PIN_LCD_CS, LOW);
    tm1621_delay_half();
    tm1621_send_msb(full_addr, 9);  // MSB first

    // NON fare stop qui - i dati seguono
    Serial.printf("[TM1621] ADDR: %d (full: 0x%04X)\n", addr, full_addr);
}

// Invia dati dopo l'indirizzo
// Tasmota: usa TM1621SendCommon che invia LSB first!
// I dati sono 8 bit per byte, inviati LSB first
static void tm1621_send_data(const uint8_t* data, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        tm1621_send_byte_lsb(data[i]);  // LSB first!
    }
    tm1621_stop();
    tm1621_write_count++;
}

// Scrive dati alla RAM del display (indirizzo + dati)
void tm1621_write_ram(uint8_t addr, const uint8_t* data, uint8_t count) {
    tm1621_send_address(addr);
    tm1621_send_data(data, count);

    // Salva nel mirror
    for (uint8_t i = 0; i < count && (addr + i) < 32; i++) {
        tm1621_ram_mirror[addr + i] = data[i];
    }
    tm1621_last_addr = addr;
    tm1621_last_count = count;
}

// Scrive un singolo byte
void tm1621_write_byte(uint8_t addr, uint8_t data) {
    tm1621_write_ram(addr, &data, 1);
}

// Riempie la RAM con un valore (16 byte da addr 0)
void tm1621_fill_ram(uint8_t value) {
    uint8_t data[16];
    for (int i = 0; i < 16; i++) {
        data[i] = value;
    }
    tm1621_write_ram(0, data, 16);
    Serial.printf("[TM1621] FILL 16 bytes: 0x%02X @ addr 0\n", value);
}

// Riempie con indirizzo e count configurabili (per test)
void tm1621_fill_at(uint8_t value, uint8_t addr, uint8_t count) {
    uint8_t data[32];
    for (int i = 0; i < count && i < 32; i++) {
        data[i] = value;
    }
    tm1621_write_ram(addr, data, count);
    Serial.printf("[TM1621] FILL %d bytes: 0x%02X @ addr %d\n", count, value, addr);
}

// =============================================
// INIZIALIZZAZIONE (DA TASMOTA)
// =============================================

void tm1621_init() {
    Serial.println("[TM1621] ========== INIT v5.0 (Tasmota) ==========");
    Serial.printf("[TM1621] Pins: CS=%d, WR=%d, DATA=%d, RD=%d\n",
                  PIN_LCD_CS, PIN_LCD_WR, PIN_LCD_DATA, PIN_LCD_RD);
    Serial.printf("[TM1621] Pulse: %d us, BIAS: 0x%02X\n",
                  tm1621_pulse_us, tm1621_bias_cmd);

    // Configura GPIO come output
    pinMode(PIN_LCD_CS, OUTPUT);
    pinMode(PIN_LCD_WR, OUTPUT);
    pinMode(PIN_LCD_DATA, OUTPUT);
    pinMode(PIN_LCD_RD, OUTPUT);

    // Sequenza init Tasmota
    digitalWrite(PIN_LCD_CS, LOW);
    delayMicroseconds(80);
    digitalWrite(PIN_LCD_RD, LOW);
    delayMicroseconds(15);
    digitalWrite(PIN_LCD_WR, LOW);
    delayMicroseconds(25);
    digitalWrite(PIN_LCD_DATA, LOW);
    delayMicroseconds(tm1621_pulse_us);
    digitalWrite(PIN_LCD_DATA, HIGH);

    // Invia comandi di inizializzazione
    tm1621_send_command(TM1621_SYS_EN);
    tm1621_send_command(TM1621_LCD_ON);
    tm1621_send_command(tm1621_bias_cmd);
    tm1621_send_command(TM1621_TIMER_DIS);
    tm1621_send_command(TM1621_WDT_DIS);
    tm1621_send_command(TM1621_TONE_OFF);
    tm1621_send_command(TM1621_IRQ_DIS);

    // Pulisci display (scrivi 0 a tutti i segmenti)
    tm1621_fill_ram(0x00);

    tm1621_initialized = true;
    tm1621_lcd_on = true;

    Serial.println("[TM1621] ========== INIT COMPLETE ==========");
}

void tm1621_reinit() {
    tm1621_initialized = false;
    tm1621_lcd_on = false;
    delay(100);
    tm1621_init();
}

// =============================================
// CONTROLLI LCD
// =============================================

void tm1621_lcd_on_cmd() {
    tm1621_send_command(TM1621_LCD_ON);
    tm1621_lcd_on = true;
    Serial.println("[TM1621] LCD ON");
}

void tm1621_lcd_off_cmd() {
    tm1621_send_command(TM1621_LCD_OFF);
    tm1621_lcd_on = false;
    Serial.println("[TM1621] LCD OFF");
}

void tm1621_set_bias(uint8_t bias) {
    tm1621_bias_cmd = bias;
    tm1621_send_command(bias);
    Serial.printf("[TM1621] BIAS: 0x%02X\n", bias);
}

void tm1621_set_pulse_width(uint8_t us) {
    tm1621_pulse_us = us;
    Serial.printf("[TM1621] Pulse: %d us\n", us);
}

// =============================================
// TEST MODE
// =============================================

void tm1621_set_test_mode(bool enable) {
    tm1621_test_mode = enable;
    Serial.printf("[TM1621] Test mode: %s\n", enable ? "ON" : "OFF");
}

bool tm1621_is_test_mode() {
    return tm1621_test_mode;
}

// =============================================
// FUNZIONI TEST/DEBUG
// =============================================

void tm1621_test_all_on() {
    tm1621_fill_ram(0xFF);
    Serial.println("[TM1621] All segments ON");
}

void tm1621_test_all_off() {
    tm1621_fill_ram(0x00);
    Serial.println("[TM1621] All segments OFF");
}

void tm1621_test_pattern(uint8_t pattern) {
    tm1621_fill_ram(pattern);
    Serial.printf("[TM1621] Pattern: 0x%02X\n", pattern);
}

void tm1621_test_pins_sequence() {
    Serial.println("[TM1621] === PIN TEST ===");
    uint8_t pins[] = {PIN_LCD_CS, PIN_LCD_WR, PIN_LCD_DATA, PIN_LCD_RD};
    const char* names[] = {"CS", "WR", "DATA", "RD"};

    for (int p = 0; p < 4; p++) {
        Serial.printf("[TM1621] Testing %s (GPIO%d)\n", names[p], pins[p]);
        for (int i = 0; i < 3; i++) {
            digitalWrite(pins[p], LOW);
            delay(200);
            digitalWrite(pins[p], HIGH);
            delay(200);
        }
    }
    Serial.println("[TM1621] === PIN TEST DONE ===");
}

void tm1621_test_pin(uint8_t pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
    Serial.printf("[TM1621] GPIO%d = %s\n", pin, state ? "HIGH" : "LOW");
}

// =============================================
// GETTERS PER STATO
// =============================================

bool tm1621_is_initialized() { return tm1621_initialized; }
bool tm1621_is_lcd_on() { return tm1621_lcd_on; }
uint32_t tm1621_get_write_count() { return tm1621_write_count; }
uint32_t tm1621_get_cmd_count() { return tm1621_cmd_count; }
uint8_t tm1621_get_bias() { return tm1621_bias_cmd; }
uint8_t tm1621_get_pulse_width() { return tm1621_pulse_us; }

String tm1621_get_ram_hex() {
    char buf[128];
    snprintf(buf, sizeof(buf),
             "%02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X",
             tm1621_ram_mirror[0], tm1621_ram_mirror[1], tm1621_ram_mirror[2], tm1621_ram_mirror[3],
             tm1621_ram_mirror[4], tm1621_ram_mirror[5], tm1621_ram_mirror[6], tm1621_ram_mirror[7],
             tm1621_ram_mirror[8], tm1621_ram_mirror[9], tm1621_ram_mirror[10], tm1621_ram_mirror[11],
             tm1621_ram_mirror[12], tm1621_ram_mirror[13], tm1621_ram_mirror[14], tm1621_ram_mirror[15]);
    return String(buf);
}

void tm1621_print_status() {
    Serial.println("[TM1621] === STATUS ===");
    Serial.printf("  Initialized: %s\n", tm1621_initialized ? "YES" : "NO");
    Serial.printf("  LCD ON: %s\n", tm1621_lcd_on ? "YES" : "NO");
    Serial.printf("  Test mode: %s\n", tm1621_test_mode ? "YES" : "NO");
    Serial.printf("  BIAS: 0x%02X\n", tm1621_bias_cmd);
    Serial.printf("  Pulse: %d us\n", tm1621_pulse_us);
    Serial.printf("  Writes: %lu\n", tm1621_write_count);
    Serial.printf("  Commands: %lu\n", tm1621_cmd_count);
}

// =============================================
// FUNZIONI PLACEHOLDER PER COMPATIBILITA'
// =============================================

void initDisplay() { tm1621_init(); }
void reinitDisplay() { tm1621_reinit(); }
void displayOn() { tm1621_lcd_on_cmd(); }
void displayOff() { tm1621_lcd_off_cmd(); }
bool isDisplayInitialized() { return tm1621_is_initialized(); }
bool isDisplayOn() { return tm1621_is_lcd_on(); }
uint32_t getDisplayUpdateCount() { return tm1621_get_write_count(); }
String getDisplayContent() { return "----"; }
String getDisplayBufferHex() { return tm1621_get_ram_hex(); }
String getDisplayParams() {
    char buf[64];
    snprintf(buf, sizeof(buf), "bias=0x%02X,pulse=%dus",
             tm1621_bias_cmd, tm1621_pulse_us);
    return String(buf);
}
void printDisplayStatus() { tm1621_print_status(); }

void clearDisplay() {
    if (!tm1621_test_mode) tm1621_fill_ram(0x00);
}
void displayAllOn() {
    if (!tm1621_test_mode) tm1621_fill_ram(0xFF);
}
void displayTime(uint8_t hour, uint8_t minute) {
    if (tm1621_test_mode) return;
    // TODO: implementare dopo mapping segmenti
}
void displayLoading() {
    if (tm1621_test_mode) return;
    // TODO
}
void displayBell() {
    if (tm1621_test_mode) return;
    // TODO
}
void displayConnecting() {
    if (tm1621_test_mode) return;
    // TODO
}
void displayNumber(uint8_t row, int value, uint8_t decimals) {
    if (tm1621_test_mode) return;
    // TODO
}

// =============================================
// SCRIVE NIBBLE SINGOLO (per test debug page)
// =============================================
void tm1621_write_nibble(uint8_t addr, uint8_t nibble) {
    // Scrive un singolo byte (che contiene il nibble)
    tm1621_write_byte(addr, nibble);
    Serial.printf("[TM1621] Nibble: addr=%d val=0x%X\n", addr, nibble);
}

#endif // TM1621_DISPLAY_H
