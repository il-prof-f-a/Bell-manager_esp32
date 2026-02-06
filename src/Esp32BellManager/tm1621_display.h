#ifndef TM1621_DISPLAY_H
#define TM1621_DISPLAY_H

#include <Arduino.h>
#include "config.h"

// ============================================
// Driver Display LCD TM1621 v3.0
// Per Sonoff POW Elite 16A (POWR316D)
//
// Riferimenti:
// - Tasmota: xdrv_87_esp32_sonoff_tm1621.ino
// - ESPHome: tm1621 display component
// - ESPEasy: P148 POWR3xxD/THR3xxD plugin
// - Datasheet TM1621 (Titan Micro)
// ============================================

// =============================================
// PARAMETRI CONFIGURABILI (per debug/tuning)
// =============================================

// Modalità comando: 0=emsyscode (LSB first, pre-calcolati, 2us), 1=Tasmota (MSB first, runtime, 10us)
static uint8_t tm1621_cmd_mode = 0;  // DEFAULT: emsyscode

// Timing (microsecondi) - Valori comuni: 2, 5, 10, 20
static uint8_t tm1621_pulse_width = 2;  // Default emsyscode: 2us
static uint8_t tm1621_cs_delay = 5;

// Indirizzo memoria display - Tasmota usa 0x00 per POWR316D
static uint8_t tm1621_mem_address = 0x00;

// Ordine bit dati: true=LSB first (standard), false=MSB first
static bool tm1621_lsb_first = true;

// Comando BIAS: 0x29=1/3 bias 4 commons (standard POWR316D)
// Alternative: 0x28, 0x2A, 0x2B, 0x25, 0x27
static uint8_t tm1621_bias_cmd = 0x29;

// Sequenza reset: 0=Tasmota, 1=ESPHome, 2=Minimal, 3=Extended
static uint8_t tm1621_reset_mode = 0;

// Comandi extra durante init
static bool tm1621_send_sys_dis_first = false;  // Alcuni inviano SYS_DIS prima di SYS_EN

// =============================================
// COMANDI TM1621 (da datasheet)
// =============================================
#define TM1621_SYS_DIS      0x00  // System disable (oscillator off)
#define TM1621_SYS_EN       0x01  // System enable (oscillator on)
#define TM1621_LCD_OFF      0x02  // LCD bias generator off
#define TM1621_LCD_ON       0x03  // LCD bias generator on
#define TM1621_TIMER_DIS    0x04  // Timer disable
#define TM1621_WDT_DIS      0x05  // Watchdog disable
#define TM1621_TIMER_EN     0x06  // Timer enable
#define TM1621_WDT_EN       0x07  // Watchdog enable
#define TM1621_TONE_OFF     0x08  // Tone off
#define TM1621_TONE_ON      0x09  // Tone on
#define TM1621_CLR_TIMER    0x0C  // Clear timer
#define TM1621_CLR_WDT      0x0F  // Clear watchdog

// BIAS/COM configurations
#define TM1621_BIAS_2_2     0x20  // 1/2 bias, 2 commons
#define TM1621_BIAS_2_3     0x24  // 1/2 bias, 3 commons
#define TM1621_BIAS_2_4     0x28  // 1/2 bias, 4 commons
#define TM1621_BIAS_3_2     0x21  // 1/3 bias, 2 commons
#define TM1621_BIAS_3_3     0x25  // 1/3 bias, 3 commons
#define TM1621_BIAS_3_4     0x29  // 1/3 bias, 4 commons (POWR316D default)

#define TM1621_IRQ_DIS      0x80  // IRQ output disable
#define TM1621_IRQ_EN       0x88  // IRQ output enable

// =============================================
// STATO DISPLAY (per diagnostica)
// =============================================
static bool tm1621_initialized = false;
static bool tm1621_display_on = false;
static String tm1621_last_content = "----";
static uint32_t tm1621_update_count = 0;
static uint32_t tm1621_error_count = 0;
static uint32_t tm1621_last_init_time = 0;

// Modalità TEST: blocca aggiornamenti automatici del display
static bool tm1621_test_mode = false;

// =============================================
// SEGMENT MAPPING - POWR316D
// Tasmota: Row 0 (top), Row 1 (bottom) hanno mapping diversi!
// =============================================

// Row 0 (riga superiore) - 0-9, minus(10), blank(11), E(12)
static uint8_t TM1621_DIGIT_ROW0[] = {
    0x5F,  // 0
    0x50,  // 1
    0x3D,  // 2
    0x79,  // 3
    0x72,  // 4
    0x6B,  // 5
    0x6F,  // 6
    0x51,  // 7
    0x7F,  // 8
    0x7B,  // 9
    0x20,  // - (minus)
    0x00,  // blank
    0x2F   // E
};

// Row 1 (riga inferiore) - mapping diverso!
static uint8_t TM1621_DIGIT_ROW1[] = {
    0xF5,  // 0
    0x05,  // 1
    0xB6,  // 2
    0x97,  // 3
    0x47,  // 4
    0xD3,  // 5
    0xF3,  // 6
    0x85,  // 7
    0xF7,  // 8
    0xD7,  // 9
    0x02,  // - (minus)
    0x00,  // blank
    0xB3   // E
};

// Punto decimale
static uint8_t tm1621_dp_row0 = 0x80;
static uint8_t tm1621_dp_row1 = 0x08;

// Buffer display (16 bytes per sicurezza)
static uint8_t tm1621_buffer[16];

// =============================================
// FUNZIONI CONFIGURAZIONE RUNTIME
// =============================================

void tm1621_set_pulse_width(uint8_t us) {
    tm1621_pulse_width = us;
    Serial.printf("[TM1621] Pulse width: %d us\n", us);
}

void tm1621_set_mem_address(uint8_t addr) {
    tm1621_mem_address = addr;
    Serial.printf("[TM1621] Memory address: 0x%02X\n", addr);
}

void tm1621_set_lsb_first(bool lsb) {
    tm1621_lsb_first = lsb;
    Serial.printf("[TM1621] Bit order: %s first\n", lsb ? "LSB" : "MSB");
}

void tm1621_set_bias(uint8_t bias) {
    tm1621_bias_cmd = bias;
    Serial.printf("[TM1621] BIAS command: 0x%02X\n", bias);
}

void tm1621_set_reset_mode(uint8_t mode) {
    tm1621_reset_mode = mode;
    Serial.printf("[TM1621] Reset mode: %d\n", mode);
}

void tm1621_set_cmd_mode(uint8_t mode) {
    tm1621_cmd_mode = mode;
    Serial.printf("[TM1621] Cmd mode: %s (%d)\n", mode == 0 ? "emsyscode" : "Tasmota", mode);
}

void tm1621_set_test_mode(bool enable) {
    tm1621_test_mode = enable;
    Serial.printf("[TM1621] Test mode: %s\n", enable ? "ON (auto-update disabled)" : "OFF");
}

bool tm1621_is_test_mode() {
    return tm1621_test_mode;
}

// =============================================
// FUNZIONI DI BASSO LIVELLO
// =============================================

void tm1621_delay() {
    delayMicroseconds(tm1621_pulse_width);
}

void tm1621_stop() {
    digitalWrite(PIN_LCD_CS, HIGH);
    delayMicroseconds(tm1621_pulse_width / 2);  // Tasmota usa PULSE_WIDTH / 2
    digitalWrite(PIN_LCD_DATA, HIGH);
}

// Invia N bit LSB first (per modalità emsyscode)
// data contiene i bit da inviare, bit0 viene inviato per primo
void tm1621_send_bits_lsb(uint16_t data, uint8_t bits) {
    for (uint8_t i = 0; i < bits; i++) {
        digitalWrite(PIN_LCD_WR, LOW);
        digitalWrite(PIN_LCD_DATA, (data & 1) ? HIGH : LOW);
        delayMicroseconds(tm1621_pulse_width);
        digitalWrite(PIN_LCD_WR, HIGH);
        delayMicroseconds(tm1621_pulse_width);
        data >>= 1;
    }
}

// Invia comando (12 bit: 100 + 8 bit comando + 1 bit don't care)
// Formato TM1621: bit11-9=100 (prefix), bit8-1=comando, bit0=X
// Supporta modalità emsyscode (LSB first) e Tasmota (MSB first)
void tm1621_send_command(uint8_t cmd) {
    digitalWrite(PIN_LCD_CS, LOW);
    delayMicroseconds(tm1621_pulse_width / 2);  // Tasmota usa PULSE_WIDTH / 2

    if (tm1621_cmd_mode == 0) {
        // emsyscode: LSB first, pre-calcolato
        // 12 bit LSB first: bit0=X(0), bit1-8=cmd, bit9-10=0, bit11=1
        uint16_t lsb_cmd = (cmd << 1) | 0x800;
        tm1621_send_bits_lsb(lsb_cmd, 12);
    } else {
        // Tasmota: MSB first (ESATTO come codice Tasmota)
        uint16_t full = (0x0400 | cmd) << 5;  // 0b100cccccccc00000
        for (uint32_t i = 0; i < 12; i++) {
            digitalWrite(PIN_LCD_WR, LOW);
            if (full & 0x8000) {
                digitalWrite(PIN_LCD_DATA, HIGH);
            } else {
                digitalWrite(PIN_LCD_DATA, LOW);
            }
            delayMicroseconds(tm1621_pulse_width);
            digitalWrite(PIN_LCD_WR, HIGH);
            delayMicroseconds(tm1621_pulse_width);
            full <<= 1;
        }
    }

    tm1621_stop();
}

// Invia indirizzo per scrittura (9 bit: 101 + 6 bit addr)
// Formato TM1621: bit8-6=101 (prefix write), bit5-0=address
// Supporta modalità emsyscode (LSB first) e Tasmota (MSB first)
void tm1621_send_address(uint8_t addr) {
    digitalWrite(PIN_LCD_CS, LOW);
    delayMicroseconds(tm1621_pulse_width / 2);  // Tasmota usa PULSE_WIDTH / 2

    if (tm1621_cmd_mode == 0) {
        // emsyscode: LSB first, pre-calcolato
        // 9 bit LSB first: bit0-5=addr, bit6=1, bit7=0, bit8=1
        uint16_t lsb_addr = (addr & 0x3F) | 0x140;
        tm1621_send_bits_lsb(lsb_addr, 9);
    } else {
        // Tasmota: MSB first (ESATTO come codice Tasmota)
        uint16_t full = (addr | 0x0140) << 7;  // 0b101aaaaaa0000000
        for (uint32_t i = 0; i < 9; i++) {
            digitalWrite(PIN_LCD_WR, LOW);
            if (full & 0x8000) {
                digitalWrite(PIN_LCD_DATA, HIGH);
            } else {
                digitalWrite(PIN_LCD_DATA, LOW);
            }
            delayMicroseconds(tm1621_pulse_width);
            digitalWrite(PIN_LCD_WR, HIGH);
            delayMicroseconds(tm1621_pulse_width);
            full <<= 1;
        }
    }
    // NON chiamare stop - i dati seguono
}

// Invia dato 4 bit - come da datasheet TM1621 (D0 D1 D2 D3, LSB first)
// NOTA: La RAM del TM1621 è 32x4 bit, ogni scrittura è 4 bit!
void tm1621_send_data_4bit(uint8_t data) {
    // Invia solo 4 bit LSB first (D0, D1, D2, D3)
    for (uint32_t i = 0; i < 4; i++) {
        digitalWrite(PIN_LCD_WR, LOW);
        if (data & 1) {
            digitalWrite(PIN_LCD_DATA, HIGH);
        } else {
            digitalWrite(PIN_LCD_DATA, LOW);
        }
        delayMicroseconds(tm1621_pulse_width);
        digitalWrite(PIN_LCD_WR, HIGH);
        delayMicroseconds(tm1621_pulse_width);
        data >>= 1;
    }
}

// Invia dato 8 bit come 2 nibble consecutivi (per compatibilità Tasmota)
// Tasmota invia 8 bit che vengono scritti su 2 indirizzi consecutivi (4+4 bit)
void tm1621_send_data(uint8_t data) {
    // Nibble basso (D0-D3) va all'indirizzo corrente
    tm1621_send_data_4bit(data & 0x0F);
    // Nibble alto (D4-D7) va all'indirizzo successivo (auto-increment)
    tm1621_send_data_4bit((data >> 4) & 0x0F);
}

// =============================================
// FUNZIONI DI TEST/DEBUG
// =============================================

// Test singolo pin
void tm1621_test_pin(uint8_t pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
    Serial.printf("[TM1621] GPIO%d = %s\n", pin, state ? "HIGH" : "LOW");
}

// Toggle tutti i pin per verificare cablaggio
void tm1621_test_all_pins() {
    Serial.println("[TM1621] === TEST PIN SEQUENCE ===");
    Serial.printf("CS=GPIO%d, DATA=GPIO%d, RD=GPIO%d, WR=GPIO%d\n",
                  PIN_LCD_CS, PIN_LCD_DATA, PIN_LCD_RD, PIN_LCD_WR);

    uint8_t pins[] = {PIN_LCD_CS, PIN_LCD_DATA, PIN_LCD_RD, PIN_LCD_WR};
    const char* names[] = {"CS", "DATA", "RD", "WR"};

    for (int p = 0; p < 4; p++) {
        Serial.printf("[TM1621] Testing %s (GPIO%d)...\n", names[p], pins[p]);
        for (int i = 0; i < 3; i++) {
            digitalWrite(pins[p], HIGH);
            delay(200);
            digitalWrite(pins[p], LOW);
            delay(200);
        }
        digitalWrite(pins[p], HIGH);
    }
    Serial.println("[TM1621] === TEST COMPLETE ===");
}

// Scrivi valore raw a indirizzo specifico (8 bit = 2 nibble su 2 indirizzi)
void tm1621_write_raw(uint8_t addr, uint8_t data) {
    Serial.printf("[TM1621] Raw write: addr=0x%02X data=0x%02X (2 nibble)\n", addr, data);
    tm1621_send_address(addr);
    tm1621_send_data(data);  // Scrive 2 nibble (8 bit totali)
    tm1621_stop();
}

// Scrivi singolo nibble (4 bit) a indirizzo specifico - per test precisi
void tm1621_write_raw_4bit(uint8_t addr, uint8_t data) {
    Serial.printf("[TM1621] Raw write 4bit: addr=0x%02X data=0x%X\n", addr, data & 0x0F);
    tm1621_send_address(addr);
    tm1621_send_data_4bit(data & 0x0F);  // Solo 4 bit
    tm1621_stop();
}

// Scrivi più byte consecutivi (come Tasmota TM1621SendRows)
// Invia indirizzo una volta, poi tutti i dati, poi stop
void tm1621_write_raw_multi(uint8_t start_addr, uint8_t* data, uint8_t count) {
    Serial.printf("[TM1621] Raw multi write: addr=0x%02X count=%d\n", start_addr, count);
    tm1621_send_address(start_addr);
    for (uint8_t i = 0; i < count; i++) {
        tm1621_send_data(data[i]);
        Serial.printf("  [%d] = 0x%02X\n", i, data[i]);
    }
    tm1621_stop();
}

// Riempi tutta la memoria con valore
void tm1621_fill_memory(uint8_t value, uint8_t start_addr, uint8_t count) {
    Serial.printf("[TM1621] Fill: addr=0x%02X count=%d value=0x%02X\n",
                  start_addr, count, value);
    tm1621_send_address(start_addr);
    for (int i = 0; i < count; i++) {
        tm1621_send_data(value);
    }
    tm1621_stop();
}

// Test pattern: tutti segmenti ON
void tm1621_all_on() {
    Serial.println("[TM1621] All segments ON");
    tm1621_fill_memory(0xFF, 0x00, 16);
    tm1621_fill_memory(0xFF, 0x10, 16);
    tm1621_last_content = "8888";
}

// Test pattern: tutti segmenti OFF
void tm1621_all_off() {
    Serial.println("[TM1621] All segments OFF");
    tm1621_fill_memory(0x00, 0x00, 16);
    tm1621_fill_memory(0x00, 0x10, 16);
    tm1621_last_content = "    ";
}

// Test pattern: alterna ON/OFF
void tm1621_pattern_test() {
    Serial.println("[TM1621] Pattern test: alternating");
    for (uint8_t v = 0; v <= 0xFF; v += 0x55) {
        tm1621_fill_memory(v, tm1621_mem_address, 8);
        delay(500);
    }
}

// Invia singolo comando (per test)
void tm1621_send_single_cmd(uint8_t cmd) {
    Serial.printf("[TM1621] Sending command: 0x%02X\n", cmd);
    tm1621_send_command(cmd);
}

// =============================================
// SEQUENZE DI RESET
// =============================================

void tm1621_reset_tasmota() {
    Serial.println("[TM1621] Reset: Tasmota style");

    // Sequenza esatta da Tasmota xdrv_87_esp32_sonoff_tm1621.ino
    digitalWrite(PIN_LCD_CS, LOW);
    delayMicroseconds(80);
    digitalWrite(PIN_LCD_RD, LOW);
    delayMicroseconds(15);
    digitalWrite(PIN_LCD_WR, LOW);
    delayMicroseconds(25);
    digitalWrite(PIN_LCD_DATA, LOW);
    delayMicroseconds(10);  // Tasmota usa 10us fissi qui
    digitalWrite(PIN_LCD_DATA, HIGH);

    // NOTA: Tasmota NON resetta i pin a HIGH qui - procede direttamente
    // con i comandi. Ma le nostre funzioni comando/address
    // gestiscono CS correttamente, quindi lo resettiamo per sicurezza
    digitalWrite(PIN_LCD_CS, HIGH);
    digitalWrite(PIN_LCD_RD, HIGH);
    digitalWrite(PIN_LCD_WR, HIGH);
}

void tm1621_reset_esphome() {
    Serial.println("[TM1621] Reset: ESPHome style");

    digitalWrite(PIN_LCD_CS, HIGH);
    digitalWrite(PIN_LCD_WR, HIGH);
    digitalWrite(PIN_LCD_DATA, HIGH);
    digitalWrite(PIN_LCD_RD, HIGH);
    delay(10);

    digitalWrite(PIN_LCD_CS, LOW);
    delay(1);
    digitalWrite(PIN_LCD_CS, HIGH);
    delay(1);
}

void tm1621_reset_minimal() {
    Serial.println("[TM1621] Reset: Minimal");

    digitalWrite(PIN_LCD_CS, HIGH);
    digitalWrite(PIN_LCD_WR, HIGH);
    digitalWrite(PIN_LCD_DATA, HIGH);
    digitalWrite(PIN_LCD_RD, HIGH);
    delay(5);
}

void tm1621_reset_extended() {
    Serial.println("[TM1621] Reset: Extended");

    // Power cycle simulation
    digitalWrite(PIN_LCD_CS, LOW);
    digitalWrite(PIN_LCD_WR, LOW);
    digitalWrite(PIN_LCD_DATA, LOW);
    digitalWrite(PIN_LCD_RD, LOW);
    delay(100);

    digitalWrite(PIN_LCD_CS, HIGH);
    digitalWrite(PIN_LCD_WR, HIGH);
    digitalWrite(PIN_LCD_DATA, HIGH);
    digitalWrite(PIN_LCD_RD, HIGH);
    delay(100);

    // Tasmota sequence
    tm1621_reset_tasmota();
}

void tm1621_do_reset() {
    switch (tm1621_reset_mode) {
        case 0: tm1621_reset_tasmota(); break;
        case 1: tm1621_reset_esphome(); break;
        case 2: tm1621_reset_minimal(); break;
        case 3: tm1621_reset_extended(); break;
        default: tm1621_reset_tasmota(); break;
    }
}

// =============================================
// INIZIALIZZAZIONE
// =============================================

void initDisplay() {
    Serial.println("[TM1621] ========== INIT START ==========");
    Serial.printf("[TM1621] Pin: CS=%d DATA=%d RD=%d WR=%d\n",
                  PIN_LCD_CS, PIN_LCD_DATA, PIN_LCD_RD, PIN_LCD_WR);
    Serial.printf("[TM1621] Params: cmd_mode=%s, pulse=%dus, addr=0x%02X, %s, bias=0x%02X, reset=%d\n",
                  tm1621_cmd_mode == 0 ? "emsyscode" : "Tasmota",
                  tm1621_pulse_width, tm1621_mem_address,
                  tm1621_lsb_first ? "LSB" : "MSB", tm1621_bias_cmd, tm1621_reset_mode);

    // Configura pin come OUTPUT
    pinMode(PIN_LCD_CS, OUTPUT);
    pinMode(PIN_LCD_DATA, OUTPUT);
    pinMode(PIN_LCD_RD, OUTPUT);
    pinMode(PIN_LCD_WR, OUTPUT);

    // Stato iniziale HIGH (idle) - come Tasmota
    digitalWrite(PIN_LCD_CS, HIGH);
    digitalWrite(PIN_LCD_DATA, HIGH);
    digitalWrite(PIN_LCD_RD, HIGH);
    digitalWrite(PIN_LCD_WR, HIGH);

    delay(50);

    // Reset sequence (Tasmota style di default)
    tm1621_do_reset();

    // Comandi inizializzazione - ORDINE ESATTO Tasmota:
    // SYS_EN, LCD_ON, BIAS, TIMER_DIS, WDT_DIS, TONE_OFF, IRQ_DIS
    Serial.println("[TM1621] Cmd: SYS_EN (0x01)");
    tm1621_send_command(TM1621_SYS_EN);

    Serial.println("[TM1621] Cmd: LCD_ON (0x03)");
    tm1621_send_command(TM1621_LCD_ON);

    Serial.printf("[TM1621] Cmd: BIAS (0x%02X)\n", tm1621_bias_cmd);
    tm1621_send_command(tm1621_bias_cmd);

    Serial.println("[TM1621] Cmd: TIMER_DIS (0x04)");
    tm1621_send_command(TM1621_TIMER_DIS);

    Serial.println("[TM1621] Cmd: WDT_DIS (0x05)");
    tm1621_send_command(TM1621_WDT_DIS);

    Serial.println("[TM1621] Cmd: TONE_OFF (0x08)");
    tm1621_send_command(TM1621_TONE_OFF);

    Serial.println("[TM1621] Cmd: IRQ_DIS (0x80)");
    tm1621_send_command(TM1621_IRQ_DIS);

    // Clear display memory - Tasmota usa solo 16 segmenti all'indirizzo 0x00
    Serial.println("[TM1621] Clearing memory (16 segments @ 0x00)...");
    tm1621_send_address(0x00);
    for (int i = 0; i < 16; i++) {
        tm1621_send_data(0x00);
    }
    tm1621_stop();

    memset(tm1621_buffer, 0, sizeof(tm1621_buffer));

    tm1621_initialized = true;
    tm1621_display_on = true;
    tm1621_update_count = 0;
    tm1621_last_init_time = millis();

    Serial.println("[TM1621] ========== INIT COMPLETE ==========");
}

// Re-init con nuovi parametri
void reinitDisplay() {
    tm1621_initialized = false;
    tm1621_display_on = false;
    delay(100);
    initDisplay();
}

// =============================================
// CONTROLLO DISPLAY
// =============================================

void displayOn() {
    tm1621_send_command(TM1621_LCD_ON);
    tm1621_display_on = true;
    Serial.println("[TM1621] LCD ON");
}

void displayOff() {
    tm1621_send_command(TM1621_LCD_OFF);
    tm1621_display_on = false;
    Serial.println("[TM1621] LCD OFF");
}

bool isDisplayOn() { return tm1621_display_on; }
bool isDisplayInitialized() { return tm1621_initialized; }
uint32_t getDisplayUpdateCount() { return tm1621_update_count; }
String getDisplayContent() { return tm1621_last_content; }

String getDisplayBufferHex() {
    char buf[64];
    snprintf(buf, sizeof(buf), "%02X %02X %02X %02X %02X %02X %02X %02X",
             tm1621_buffer[0], tm1621_buffer[1], tm1621_buffer[2], tm1621_buffer[3],
             tm1621_buffer[4], tm1621_buffer[5], tm1621_buffer[6], tm1621_buffer[7]);
    return String(buf);
}

String getDisplayParams() {
    char buf[180];
    snprintf(buf, sizeof(buf),
             "cmd=%s,pulse=%d,addr=0x%02X,%s,bias=0x%02X,reset=%d,test=%s",
             tm1621_cmd_mode == 0 ? "emsyscode" : "Tasmota",
             tm1621_pulse_width, tm1621_mem_address,
             tm1621_lsb_first ? "LSB" : "MSB",
             tm1621_bias_cmd, tm1621_reset_mode,
             tm1621_test_mode ? "ON" : "OFF");
    return String(buf);
}

// =============================================
// AGGIORNAMENTO DISPLAY
// =============================================

// Aggiornamento interno (rispetta test mode)
void updateDisplay() {
    // In test mode, blocca aggiornamenti automatici
    if (tm1621_test_mode) return;

    tm1621_send_address(tm1621_mem_address);
    for (int i = 0; i < 8; i++) {
        tm1621_send_data(tm1621_buffer[i]);
    }
    tm1621_stop();
    tm1621_update_count++;
}

// Aggiornamento forzato (ignora test mode) - per test manuali dalla debug page
void updateDisplayForced() {
    tm1621_send_address(tm1621_mem_address);
    for (int i = 0; i < 8; i++) {
        tm1621_send_data(tm1621_buffer[i]);
    }
    tm1621_stop();
    tm1621_update_count++;
}

void clearDisplay() {
    memset(tm1621_buffer, 0, sizeof(tm1621_buffer));
    updateDisplay();
    tm1621_last_content = "    ";
}

// Versione forzata per test manuali
void clearDisplayForced() {
    memset(tm1621_buffer, 0, sizeof(tm1621_buffer));
    updateDisplayForced();
    tm1621_last_content = "    ";
}

void displayAllOn() {
    memset(tm1621_buffer, 0xFF, 8);
    updateDisplay();
    tm1621_last_content = "8888";
}

// Versione forzata per test manuali
void displayAllOnForced() {
    memset(tm1621_buffer, 0xFF, 8);
    updateDisplayForced();
    tm1621_last_content = "8888";
}

// =============================================
// FUNZIONI DIGIT
// =============================================

void setDigit(uint8_t row, uint8_t pos, uint8_t digit, bool dp) {
    if (pos > 3 || digit > 12) return;

    uint8_t seg;
    if (row == 0) {
        seg = TM1621_DIGIT_ROW0[digit];
        if (dp) seg |= tm1621_dp_row0;
        tm1621_buffer[pos] = seg;
    } else {
        seg = TM1621_DIGIT_ROW1[digit];
        if (dp) seg |= tm1621_dp_row1;
        tm1621_buffer[7 - pos] = seg;  // Row 1 reversed
    }
}

// =============================================
// VISUALIZZAZIONI
// =============================================

void displayTime(uint8_t hour, uint8_t minute) {
    setDigit(0, 0, hour / 10 == 0 ? 11 : hour / 10, false);
    setDigit(0, 1, hour % 10, true);  // DP as separator
    setDigit(0, 2, minute / 10, false);
    setDigit(0, 3, minute % 10, false);

    setDigit(1, 0, 11, false);
    setDigit(1, 1, 11, false);
    setDigit(1, 2, 11, false);
    setDigit(1, 3, 11, false);

    updateDisplay();

    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", hour, minute);
    tm1621_last_content = String(buf);
}

void displayLoading() {
    for (int i = 0; i < 4; i++) {
        setDigit(0, i, 10, false);  // minus
        setDigit(1, i, 10, false);
    }
    updateDisplay();
    tm1621_last_content = "----";
}

void displayBell() {
    // Custom segments for "bELL"
    tm1621_buffer[0] = 0x6F;  // b
    tm1621_buffer[1] = 0x2F;  // E
    tm1621_buffer[2] = 0x0E;  // L
    tm1621_buffer[3] = 0x0E;  // L
    tm1621_buffer[4] = 0x00;
    tm1621_buffer[5] = 0x00;
    tm1621_buffer[6] = 0x00;
    tm1621_buffer[7] = 0x00;
    updateDisplay();
    tm1621_last_content = "bELL";
}

void displayConnecting() {
    tm1621_buffer[0] = 0x0F;  // C
    tm1621_buffer[1] = 0x5C;  // o
    tm1621_buffer[2] = 0x54;  // n
    tm1621_buffer[3] = 0x54;  // n
    tm1621_buffer[4] = 0x00;
    tm1621_buffer[5] = 0x00;
    tm1621_buffer[6] = 0x00;
    tm1621_buffer[7] = 0x00;
    updateDisplay();
    tm1621_last_content = "Conn";
}

void displayNumber(uint8_t row, int value, uint8_t decimals) {
    bool neg = value < 0;
    if (neg) value = -value;

    uint8_t d[4];
    d[3] = value % 10; value /= 10;
    d[2] = value % 10; value /= 10;
    d[1] = value % 10; value /= 10;
    d[0] = value % 10;

    int dpPos = (decimals > 0 && decimals < 4) ? (3 - decimals) : -1;

    int firstSig = 3;
    for (int i = 0; i < 4; i++) {
        if (d[i] != 0) { firstSig = i; break; }
    }

    for (int i = 0; i < 4; i++) {
        uint8_t digit = (i < firstSig && (dpPos < 0 || i < dpPos)) ? 11 : d[i];
        if (neg && i == firstSig - 1 && firstSig > 0) {
            setDigit(row, i, 10, false);
        } else {
            setDigit(row, i, digit, dpPos >= 0 && i == dpPos);
        }
    }
    updateDisplay();
}

// =============================================
// DEBUG SERIALE
// =============================================

void printDisplayStatus() {
    Serial.println("[TM1621] === STATUS ===");
    Serial.printf("  Initialized: %s\n", tm1621_initialized ? "YES" : "NO");
    Serial.printf("  Display ON: %s\n", tm1621_display_on ? "YES" : "NO");
    Serial.printf("  Content: %s\n", tm1621_last_content.c_str());
    Serial.printf("  Updates: %lu\n", tm1621_update_count);
    Serial.printf("  Errors: %lu\n", tm1621_error_count);
    Serial.printf("  Last init: %lu ms ago\n", millis() - tm1621_last_init_time);
    Serial.printf("  Buffer: %s\n", getDisplayBufferHex().c_str());
    Serial.printf("  Params: %s\n", getDisplayParams().c_str());
    Serial.printf("  Pins: CS=%d DATA=%d RD=%d WR=%d\n",
                  PIN_LCD_CS, PIN_LCD_DATA, PIN_LCD_RD, PIN_LCD_WR);
}

// Alias per compatibilità
void testDisplayPin(uint8_t pin, bool state) { tm1621_test_pin(pin, state); }
void testDisplayPinSequence() { tm1621_test_all_pins(); }
void testDisplayRawWrite(uint8_t addr, uint8_t data) { tm1621_write_raw(addr, data); }
void testDisplayFillRam(uint8_t value) { tm1621_fill_memory(value, 0x00, 32); }

#endif // TM1621_DISPLAY_H
