#ifndef TM1621_DISPLAY_H
#define TM1621_DISPLAY_H

#include <Arduino.h>
#include "config.h"

// ============================================
// Driver Display LCD TM1621
// Per Sonoff POW Elite 16A (POWR316D)
// Basato su Tasmota xdrv_87_esp32_sonoff_tm1621.ino
// https://github.com/arendst/Tasmota/blob/development/tasmota/tasmota_xdrv_driver/xdrv_87_esp32_sonoff_tm1621.ino
// ============================================

// Comandi TM1621 (da Tasmota)
#define TM1621_PULSE_WIDTH  10  // microsecondi
#define TM1621_SYS_EN       0x01
#define TM1621_LCD_OFF      0x02
#define TM1621_LCD_ON       0x03
#define TM1621_TIMER_DIS    0x04
#define TM1621_WDT_DIS      0x05
#define TM1621_TONE_OFF     0x08
#define TM1621_BIAS         0x29  // 1/3 bias, 4 commons
#define TM1621_IRQ_DIS      0x80

// Array comandi inizializzazione (da Tasmota)
static const uint8_t TM1621_COMMANDS[] = {
    TM1621_SYS_EN,
    TM1621_LCD_ON,
    TM1621_BIAS,
    TM1621_TIMER_DIS,
    TM1621_WDT_DIS,
    TM1621_TONE_OFF,
    TM1621_IRQ_DIS
};

// Stato display per diagnostica
static bool tm1621_initialized = false;
static bool tm1621_display_on = false;
static String tm1621_last_content = "----";
static uint32_t tm1621_update_count = 0;

// Mappatura segmenti per RIGA 0 e RIGA 1 (DIVERSE!)
// Da Tasmota: Indici: 0-9 = cifre, 10 = minus, 11 = blank
static const uint8_t TM1621_DIGIT_ROW0[] = {
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
    0x00   // blank
};

static const uint8_t TM1621_DIGIT_ROW1[] = {
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
    0x00   // blank
};

// Punto decimale
#define TM1621_DP_ROW0  0x80
#define TM1621_DP_ROW1  0x08

// Buffer display (8 bytes)
static uint8_t tm1621_buffer[8];

// ============================================
// Funzioni di basso livello
// ============================================

// --- Delay preciso ---
void tm1621_delay() {
    delayMicroseconds(TM1621_PULSE_WIDTH);
}

// --- Stop sequenza ---
void tm1621_stop() {
    digitalWrite(PIN_LCD_CS, HIGH);
    delayMicroseconds(TM1621_PULSE_WIDTH / 2);
    digitalWrite(PIN_LCD_DATA, HIGH);
}

// --- Invia comando (12 bit, MSB first) ---
// Formato: 100 + 9 bit comando
void tm1621_send_command(uint16_t cmd) {
    uint16_t full_cmd = (0x0400 | cmd) << 5;

    digitalWrite(PIN_LCD_CS, LOW);
    delayMicroseconds(TM1621_PULSE_WIDTH / 2);

    for (int i = 0; i < 12; i++) {
        digitalWrite(PIN_LCD_WR, LOW);
        digitalWrite(PIN_LCD_DATA, (full_cmd & 0x8000) ? HIGH : LOW);
        tm1621_delay();
        digitalWrite(PIN_LCD_WR, HIGH);
        tm1621_delay();
        full_cmd <<= 1;
    }

    tm1621_stop();
}

// --- Invia indirizzo (9 bit, MSB first) ---
// Formato: 101 + 6 bit indirizzo
void tm1621_send_address(uint16_t addr) {
    uint16_t full_addr = (addr | 0x0140) << 7;

    digitalWrite(PIN_LCD_CS, LOW);
    delayMicroseconds(TM1621_PULSE_WIDTH / 2);

    for (int i = 0; i < 9; i++) {
        digitalWrite(PIN_LCD_WR, LOW);
        digitalWrite(PIN_LCD_DATA, (full_addr & 0x8000) ? HIGH : LOW);
        tm1621_delay();
        digitalWrite(PIN_LCD_WR, HIGH);
        tm1621_delay();
        full_addr <<= 1;
    }
    // NON chiamare stop qui - i dati seguono
}

// --- Invia dati (8 bit, LSB first) ---
void tm1621_send_common(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        digitalWrite(PIN_LCD_WR, LOW);
        digitalWrite(PIN_LCD_DATA, (data & 0x01) ? HIGH : LOW);
        tm1621_delay();
        digitalWrite(PIN_LCD_WR, HIGH);
        tm1621_delay();
        data >>= 1;
    }
}

// ============================================
// Funzioni di controllo display
// ============================================

// --- Accendi LCD ---
void displayOn() {
    tm1621_send_command(TM1621_LCD_ON);
    tm1621_display_on = true;
    Serial.println("[DISPLAY] LCD ON");
}

// --- Spegni LCD ---
void displayOff() {
    tm1621_send_command(TM1621_LCD_OFF);
    tm1621_display_on = false;
    Serial.println("[DISPLAY] LCD OFF");
}

// --- Verifica se display e' acceso ---
bool isDisplayOn() {
    return tm1621_display_on;
}

// --- Verifica se display e' inizializzato ---
bool isDisplayInitialized() {
    return tm1621_initialized;
}

// --- Ottieni conteggio aggiornamenti ---
uint32_t getDisplayUpdateCount() {
    return tm1621_update_count;
}

// --- Ottieni ultimo contenuto ---
String getDisplayContent() {
    return tm1621_last_content;
}

// --- Ottieni buffer come stringa hex ---
String getDisplayBufferHex() {
    char buf[32];
    snprintf(buf, sizeof(buf), "%02X %02X %02X %02X %02X %02X %02X %02X",
             tm1621_buffer[0], tm1621_buffer[1], tm1621_buffer[2], tm1621_buffer[3],
             tm1621_buffer[4], tm1621_buffer[5], tm1621_buffer[6], tm1621_buffer[7]);
    return String(buf);
}

// ============================================
// Funzioni di diagnostica / test semplici
// ============================================

// --- Test singolo pin (per debug hardware) ---
void testDisplayPin(uint8_t pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
    Serial.printf("[DISPLAY] Pin %d = %s\n", pin, state ? "HIGH" : "LOW");
}

// --- Test sequenza pin (verifica cablaggio) ---
void testDisplayPinSequence() {
    Serial.println("[DISPLAY] === TEST SEQUENZA PIN ===");
    Serial.printf("[DISPLAY] CS=GPIO%d, DATA=GPIO%d, RD=GPIO%d, WR=GPIO%d\n",
                  PIN_LCD_CS, PIN_LCD_DATA, PIN_LCD_RD, PIN_LCD_WR);

    // Tutti LOW
    Serial.println("[DISPLAY] Tutti LOW...");
    digitalWrite(PIN_LCD_CS, LOW);
    digitalWrite(PIN_LCD_DATA, LOW);
    digitalWrite(PIN_LCD_RD, LOW);
    digitalWrite(PIN_LCD_WR, LOW);
    delay(500);

    // CS HIGH
    Serial.println("[DISPLAY] CS HIGH...");
    digitalWrite(PIN_LCD_CS, HIGH);
    delay(500);

    // DATA HIGH
    Serial.println("[DISPLAY] DATA HIGH...");
    digitalWrite(PIN_LCD_DATA, HIGH);
    delay(500);

    // WR toggle
    Serial.println("[DISPLAY] WR toggle...");
    for (int i = 0; i < 5; i++) {
        digitalWrite(PIN_LCD_WR, LOW);
        delay(100);
        digitalWrite(PIN_LCD_WR, HIGH);
        delay(100);
    }

    // Tutti HIGH (stato riposo)
    Serial.println("[DISPLAY] Tutti HIGH (riposo)");
    digitalWrite(PIN_LCD_CS, HIGH);
    digitalWrite(PIN_LCD_DATA, HIGH);
    digitalWrite(PIN_LCD_RD, HIGH);
    digitalWrite(PIN_LCD_WR, HIGH);

    Serial.println("[DISPLAY] === FINE TEST PIN ===");
}

// --- Scrivi byte raw diretto alla RAM display ---
void testDisplayRawWrite(uint8_t addr, uint8_t data) {
    Serial.printf("[DISPLAY] Raw write: addr=0x%02X data=0x%02X\n", addr, data);
    tm1621_send_address(addr);
    tm1621_send_common(data);
    tm1621_stop();
}

// --- Riempi tutta la RAM display con un valore ---
void testDisplayFillRam(uint8_t value) {
    Serial.printf("[DISPLAY] Fill RAM with 0x%02X\n", value);

    // Scrivi a partire dall'indirizzo 0
    tm1621_send_address(0x00);
    for (int i = 0; i < 16; i++) {
        tm1621_send_common(value);
    }
    tm1621_stop();

    // Scrivi anche a partire dall'indirizzo 0x10 (usato da Sonoff)
    tm1621_send_address(0x10);
    for (int i = 0; i < 16; i++) {
        tm1621_send_common(value);
    }
    tm1621_stop();
}

// ============================================
// Inizializzazione
// ============================================

void initDisplay() {
    Serial.println("[DISPLAY] === INIZIALIZZAZIONE TM1621 ===");
    Serial.printf("[DISPLAY] Pin: CS=%d, DATA=%d, RD=%d, WR=%d\n",
                  PIN_LCD_CS, PIN_LCD_DATA, PIN_LCD_RD, PIN_LCD_WR);

    // Configura pin come output
    pinMode(PIN_LCD_CS, OUTPUT);
    pinMode(PIN_LCD_DATA, OUTPUT);
    pinMode(PIN_LCD_RD, OUTPUT);
    pinMode(PIN_LCD_WR, OUTPUT);

    // Stato iniziale: tutti HIGH
    digitalWrite(PIN_LCD_CS, HIGH);
    digitalWrite(PIN_LCD_DATA, HIGH);
    digitalWrite(PIN_LCD_RD, HIGH);
    digitalWrite(PIN_LCD_WR, HIGH);

    delay(100);
    Serial.println("[DISPLAY] Pin configurati, inizio sequenza reset...");

    // Sequenza di reset (da Tasmota/ESPHome)
    digitalWrite(PIN_LCD_CS, LOW);
    delayMicroseconds(80);
    digitalWrite(PIN_LCD_RD, LOW);
    delayMicroseconds(15);
    digitalWrite(PIN_LCD_WR, LOW);
    delayMicroseconds(25);
    digitalWrite(PIN_LCD_DATA, LOW);
    delayMicroseconds(TM1621_PULSE_WIDTH);
    digitalWrite(PIN_LCD_DATA, HIGH);

    Serial.println("[DISPLAY] Reset completato, invio comandi...");

    // Invia comandi di inizializzazione
    for (size_t i = 0; i < sizeof(TM1621_COMMANDS); i++) {
        Serial.printf("[DISPLAY] Comando %d: 0x%02X\n", i, TM1621_COMMANDS[i]);
        tm1621_send_command(TM1621_COMMANDS[i]);
    }

    Serial.println("[DISPLAY] Pulizia memoria display...");

    // Pulisci tutti i segmenti (indirizzo 0x00, 16 byte)
    tm1621_send_address(0x00);
    for (int i = 0; i < 16; i++) {
        tm1621_send_common(0x00);
    }
    tm1621_stop();

    // Pulisci anche area 0x10 (usata da Sonoff)
    tm1621_send_address(0x10);
    for (int i = 0; i < 16; i++) {
        tm1621_send_common(0x00);
    }
    tm1621_stop();

    // Pulisci buffer locale
    memset(tm1621_buffer, 0, sizeof(tm1621_buffer));

    tm1621_initialized = true;
    tm1621_display_on = true;
    tm1621_update_count = 0;

    Serial.println("[DISPLAY] TM1621 inizializzato con successo");
    Serial.println("[DISPLAY] === FINE INIZIALIZZAZIONE ===");
}

// ============================================
// Funzioni di aggiornamento display
// ============================================

// --- Aggiorna display dal buffer ---
void updateDisplay() {
    tm1621_send_address(0x10);  // Sonoff usa segmenti da 0x10
    for (int i = 0; i < 8; i++) {
        tm1621_send_common(tm1621_buffer[i]);
    }
    tm1621_stop();
    tm1621_update_count++;
    yield();  // Permetti al WiFi di processare
}

// --- Pulisci display ---
void clearDisplay() {
    memset(tm1621_buffer, 0, sizeof(tm1621_buffer));
    updateDisplay();
    tm1621_last_content = "    ";
    Serial.println("[DISPLAY] Clear");
}

// --- Mostra tutti i segmenti ON ---
void displayAllOn() {
    memset(tm1621_buffer, 0xFF, sizeof(tm1621_buffer));
    updateDisplay();
    tm1621_last_content = "8888";
    Serial.println("[DISPLAY] All ON (0xFF)");
}

// --- Imposta cifra ---
// row: 0=superiore, 1=inferiore
// pos: 0-3 (da sinistra)
// digit: 0-9, oppure 10=minus, 11=blank
// dp: punto decimale
void setDigit(uint8_t row, uint8_t pos, uint8_t digit, bool dp) {
    if (pos > 3 || digit > 11) return;

    uint8_t seg;

    if (row == 0) {
        seg = TM1621_DIGIT_ROW0[digit];
        if (dp) seg |= TM1621_DP_ROW0;
        tm1621_buffer[pos] = seg;
    } else {
        seg = TM1621_DIGIT_ROW1[digit];
        if (dp) seg |= TM1621_DP_ROW1;
        // Riga 1: ordine inverso nel buffer (pos 0 -> buffer[7])
        tm1621_buffer[7 - pos] = seg;
    }
}

// ============================================
// Funzioni di visualizzazione
// ============================================

// --- Mostra ora HH:MM ---
void displayTime(uint8_t hour, uint8_t minute) {
    uint8_t h1 = hour / 10;
    uint8_t h2 = hour % 10;
    uint8_t m1 = minute / 10;
    uint8_t m2 = minute % 10;

    setDigit(0, 0, (h1 == 0) ? 11 : h1, false);  // blank se 0
    setDigit(0, 1, h2, true);   // punto come separatore
    setDigit(0, 2, m1, false);
    setDigit(0, 3, m2, false);

    // Riga 1: vuota
    setDigit(1, 0, 11, false);
    setDigit(1, 1, 11, false);
    setDigit(1, 2, 11, false);
    setDigit(1, 3, 11, false);

    updateDisplay();

    char buf[16];
    snprintf(buf, sizeof(buf), "%d%d:%d%d", h1, h2, m1, m2);
    tm1621_last_content = String(buf);
}

// --- Mostra "----" ---
void displayLoading() {
    for (int i = 0; i < 4; i++) {
        setDigit(0, i, 10, false);  // minus
        setDigit(1, i, 10, false);
    }
    updateDisplay();
    tm1621_last_content = "----";
}

// --- Mostra numero su riga ---
void displayNumber(uint8_t row, int value, uint8_t decimals) {
    bool negative = (value < 0);
    if (negative) value = -value;

    uint8_t d[4];
    d[3] = value % 10; value /= 10;
    d[2] = value % 10; value /= 10;
    d[1] = value % 10; value /= 10;
    d[0] = value % 10;

    int8_t dpPos = (decimals > 0 && decimals < 4) ? (3 - decimals) : -1;

    // Trova prima cifra significativa
    int firstSig = 3;
    for (int i = 0; i < 4; i++) {
        if (d[i] != 0) {
            firstSig = i;
            break;
        }
    }

    for (int i = 0; i < 4; i++) {
        uint8_t digit;
        if (i < firstSig && (dpPos < 0 || i < dpPos)) {
            digit = 11;  // blank
        } else {
            digit = d[i];
        }

        if (negative && i == firstSig - 1 && firstSig > 0) {
            setDigit(row, i, 10, false);  // minus
        } else {
            bool showDp = (dpPos >= 0 && i == dpPos);
            setDigit(row, i, digit, showDp);
        }
    }

    updateDisplay();
}

// --- Mostra "bELL" sulla riga 0 ---
void displayBell() {
    // Valori custom per lettere su riga 0
    tm1621_buffer[0] = 0x6F;  // b
    tm1621_buffer[1] = 0x6D;  // E
    tm1621_buffer[2] = 0x0E;  // L
    tm1621_buffer[3] = 0x0E;  // L

    // Riga 1 vuota
    tm1621_buffer[4] = 0x00;
    tm1621_buffer[5] = 0x00;
    tm1621_buffer[6] = 0x00;
    tm1621_buffer[7] = 0x00;

    updateDisplay();
    tm1621_last_content = "bELL";
}

// --- Mostra "Conn" sulla riga 0 ---
void displayConnecting() {
    tm1621_buffer[0] = 0x0F;  // C
    tm1621_buffer[1] = 0x6C;  // o
    tm1621_buffer[2] = 0x64;  // n
    tm1621_buffer[3] = 0x64;  // n

    tm1621_buffer[4] = 0x00;
    tm1621_buffer[5] = 0x00;
    tm1621_buffer[6] = 0x00;
    tm1621_buffer[7] = 0x00;

    updateDisplay();
    tm1621_last_content = "Conn";
}

// ============================================
// Funzione di stampa stato per debug seriale
// ============================================

void printDisplayStatus() {
    Serial.println("[DISPLAY] --- Stato Display ---");
    Serial.printf("[DISPLAY] Inizializzato: %s\n", tm1621_initialized ? "SI" : "NO");
    Serial.printf("[DISPLAY] Acceso: %s\n", tm1621_display_on ? "SI" : "NO");
    Serial.printf("[DISPLAY] Contenuto: %s\n", tm1621_last_content.c_str());
    Serial.printf("[DISPLAY] Aggiornamenti: %lu\n", tm1621_update_count);
    Serial.printf("[DISPLAY] Buffer: ");
    for (int i = 0; i < 8; i++) {
        Serial.printf("%02X ", tm1621_buffer[i]);
    }
    Serial.println();
    Serial.printf("[DISPLAY] Pin: CS=%d DATA=%d RD=%d WR=%d\n",
                  PIN_LCD_CS, PIN_LCD_DATA, PIN_LCD_RD, PIN_LCD_WR);
}

#endif // TM1621_DISPLAY_H
