#ifndef BELL_TYPES_H
#define BELL_TYPES_H

#include <Arduino.h>
#include "config.h"

// ============================================
// Strutture Dati Bell-Manager v2.1
//
// ARCHITETTURA:
//   ConfigState  = dati persistenti (NVS)
//   RuntimeState = stato volatile (RAM)
//
// REST  -> legge/scrive Config
// Core  -> legge Config, aggiorna Runtime
// SSE   -> notifica delta Runtime
// ============================================

// --- Bitmask Giorni della Settimana ---
// bit0=Lunedi, bit1=Martedi, ..., bit6=Domenica
#define DAY_MON     (1 << 0)  // 0x01 - Lunedi
#define DAY_TUE     (1 << 1)  // 0x02 - Martedi
#define DAY_WED     (1 << 2)  // 0x04 - Mercoledi
#define DAY_THU     (1 << 3)  // 0x08 - Giovedi
#define DAY_FRI     (1 << 4)  // 0x10 - Venerdi
#define DAY_SAT     (1 << 5)  // 0x20 - Sabato
#define DAY_SUN     (1 << 6)  // 0x40 - Domenica
#define DAYS_WEEKDAYS (DAY_MON | DAY_TUE | DAY_WED | DAY_THU | DAY_FRI)
#define DAYS_WEEKEND  (DAY_SAT | DAY_SUN)
#define DAYS_ALL      (DAYS_WEEKDAYS | DAYS_WEEKEND)

// --- Struttura Campanella ---
struct Bell {
    uint8_t  id;                        // ID univoco 1-255 (0 = non valido)
    uint8_t  hour;                      // Ora 0-23
    uint8_t  minute;                    // Minuto 0-59
    uint8_t  duration;                  // Durata in secondi (1-255)
    uint8_t  days;                      // Bitmask giorni attivi
    bool     enabled;                   // Campanella attiva/disattiva
    char     type[MAX_TYPE_LENGTH];     // Tipo: "Intervallo", "Fine lezione", ecc.
};

// --- Struttura Impostazioni ---
struct Settings {
    char     institutionName[MAX_NAME_LENGTH];  // Nome istituzione
    bool     globalEnabled;                      // Abilita/disabilita tutte le campanelle
};

// --- Struttura Voce Log ---
struct LogEntry {
    uint8_t  bellId;        // ID campanella suonata
    uint8_t  hour;          // Ora attivazione
    uint8_t  minute;        // Minuto attivazione
    uint8_t  day;           // Giorno del mese
    uint8_t  month;         // Mese
    uint16_t year;          // Anno
};

// --- Struttura Tempo Corrente ---
struct CurrentTime {
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint8_t  day;           // Giorno del mese (1-31)
    uint8_t  month;         // Mese (1-12)
    uint16_t year;          // Anno (es. 2025)
    uint8_t  weekday;       // Giorno settimana (0=Lun, 6=Dom)
};

// --- Struttura Stato Sistema ---
struct SystemStatus {
    bool     relayOn;           // Stato relay
    bool     isRinging;         // Campanella in corso
    uint8_t  ringingBellId;     // ID campanella che sta suonando
    unsigned long ringEndTime;  // Timestamp fine suonata (millis)
};

// --- Funzioni di utilita per i giorni ---
inline bool isDayEnabled(uint8_t daysMask, uint8_t weekday) {
    // weekday: 0=Lunedi, 6=Domenica
    return (daysMask & (1 << weekday)) != 0;
}

inline const char* getDayName(uint8_t weekday) {
    static const char* days[] = {"Lun", "Mar", "Mer", "Gio", "Ven", "Sab", "Dom"};
    if (weekday < 7) return days[weekday];
    return "???";
}

inline String getDaysString(uint8_t daysMask) {
    String result = "";
    const char* names[] = {"Lun", "Mar", "Mer", "Gio", "Ven", "Sab", "Dom"};
    for (int i = 0; i < 7; i++) {
        if (daysMask & (1 << i)) {
            if (result.length() > 0) result += " ";
            result += names[i];
        }
    }
    return result.length() > 0 ? result : "Nessuno";
}

// --- Inizializzazione strutture ---
inline void initBell(Bell& bell) {
    bell.id = 0;
    bell.hour = 0;
    bell.minute = 0;
    bell.duration = DEFAULT_BELL_DURATION;
    bell.days = 0;
    bell.enabled = true;
    memset(bell.type, 0, MAX_TYPE_LENGTH);
}

inline void initSettings(Settings& settings) {
    memset(settings.institutionName, 0, MAX_NAME_LENGTH);
    strcpy(settings.institutionName, "Istituzione");
    settings.globalEnabled = true;
}

inline void initSystemStatus(SystemStatus& status) {
    status.relayOn = false;
    status.isRinging = false;
    status.ringingBellId = 0;
    status.ringEndTime = 0;
}

// ============================================
// RuntimeState - Stato volatile (solo RAM)
// Aggiornato dal Core, letto da SSE/REST
// ============================================

struct RuntimeState {
    // Time
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint8_t  weekday;       // 0=Lun, 6=Dom

    // I/O
    bool     relayOn;
    bool     buttonPressed;
    bool     ledWifi;
    bool     ledRelay;

    // Scheduler
    bool     isRinging;
    uint8_t  ringingBellId;
    uint8_t  nextBellHour;
    uint8_t  nextBellMinute;
    char     nextBellType[MAX_TYPE_LENGTH];

    // System
    bool     ntpSynced;
    uint8_t  wifiState;     // WiFiState enum
    uint32_t uptime;        // secondi
    uint32_t freeHeap;
};

inline void initRuntimeState(RuntimeState& rt) {
    rt.hour = 0;
    rt.minute = 0;
    rt.second = 0;
    rt.weekday = 0;
    rt.relayOn = false;
    rt.buttonPressed = false;
    rt.ledWifi = false;
    rt.ledRelay = false;
    rt.isRinging = false;
    rt.ringingBellId = 0;
    rt.nextBellHour = 0;
    rt.nextBellMinute = 0;
    memset(rt.nextBellType, 0, MAX_TYPE_LENGTH);
    rt.ntpSynced = false;
    rt.wifiState = 0;
    rt.uptime = 0;
    rt.freeHeap = 0;
}

#endif // BELL_TYPES_H
