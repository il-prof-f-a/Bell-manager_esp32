#ifndef SCHEDULE_ENGINE_H
#define SCHEDULE_ENGINE_H

#include <Arduino.h>
#include "config.h"
#include "bell_types.h"
#include "gpio_control.h"
#include "time_sync.h"

// ============================================
// Motore Scheduling Campanelle
// Controlla gli orari e attiva il relay
// ============================================

// Riferimenti esterni (definiti nel .ino principale)
extern Bell bells[];
extern uint8_t bellCount;
extern Settings settings;
extern SystemStatus systemStatus;

// Variabili interne
static unsigned long lastSchedulerCheck = 0;
static uint8_t lastMinuteChecked = 255;

// --- Log delle campanelle suonate ---
static LogEntry bellLog[MAX_LOG_ENTRIES];
static uint8_t logCount = 0;

// --- Stato pre-ring ---
static int8_t preRingBellIndex = -1;  // Indice campanella imminente
static uint8_t preRingState = 0;       // 0=nessuna, 1=warning (1min), 2=imminent (10sec)

// --- Aggiungi voce al log ---
void addLogEntry(uint8_t bellId) {
    CurrentTime t = getTime();

    // Shift del log se pieno (rimuove il piu' vecchio)
    if (logCount >= MAX_LOG_ENTRIES) {
        for (int i = 0; i < MAX_LOG_ENTRIES - 1; i++) {
            bellLog[i] = bellLog[i + 1];
        }
        logCount = MAX_LOG_ENTRIES - 1;
    }

    // Aggiungi nuova voce
    bellLog[logCount].bellId = bellId;
    bellLog[logCount].hour = t.hour;
    bellLog[logCount].minute = t.minute;
    bellLog[logCount].day = t.day;
    bellLog[logCount].month = t.month;
    bellLog[logCount].year = t.year;
    logCount++;

    Serial.printf("[SCHEDULER] Log: campanella %d alle %02d:%02d del %02d/%02d\n",
                  bellId, t.hour, t.minute, t.day, t.month);
}

// --- Ottieni log ---
LogEntry* getLog() {
    return bellLog;
}

uint8_t getLogCount() {
    return logCount;
}

void clearLog() {
    logCount = 0;
    Serial.println("[SCHEDULER] Log cancellato");
}

// --- Attiva campanella ---
void ringBell(uint8_t bellIndex) {
    if (bellIndex >= bellCount) return;

    Bell& bell = bells[bellIndex];

    Serial.printf("[SCHEDULER] ATTIVAZIONE campanella %d: %s (durata: %d sec)\n",
                  bell.id, bell.type, bell.duration);

    // Imposta stato sistema
    systemStatus.isRinging = true;
    systemStatus.ringingBellId = bell.id;
    systemStatus.ringEndTime = millis() + (bell.duration * 1000UL);

    // Attiva relay
    setRelay(true);
    systemStatus.relayOn = true;

    // LED relay fisso durante suono
    setRelayLedMode(1);

    // Reset pre-ring
    preRingBellIndex = -1;
    preRingState = 0;

    // Aggiungi al log
    addLogEntry(bell.id);
}

// --- Ferma campanella ---
void stopRinging() {
    if (systemStatus.isRinging) {
        Serial.printf("[SCHEDULER] STOP campanella %d\n", systemStatus.ringingBellId);
    }

    setRelay(false);
    systemStatus.relayOn = false;
    systemStatus.isRinging = false;
    systemStatus.ringingBellId = 0;
    systemStatus.ringEndTime = 0;

    // LED relay spento
    setRelayLedMode(0);
}

// --- Trova prossima campanella ---
// Restituisce l'indice della prossima campanella o -1 se non ce ne sono
int findNextBell() {
    if (!isTimeSet() || !settings.globalEnabled || bellCount == 0) {
        return -1;
    }

    CurrentTime t = getTime();
    int currentMinutes = t.hour * 60 + t.minute;
    uint8_t currentWeekday = t.weekday;

    int nextIndex = -1;
    int minDiff = 24 * 60 * 7 + 1;  // Piu' di una settimana in minuti

    for (uint8_t i = 0; i < bellCount; i++) {
        Bell& bell = bells[i];
        if (!bell.enabled) continue;

        int bellMinutes = bell.hour * 60 + bell.minute;

        // Controlla ogni giorno a partire da oggi
        for (int dayOffset = 0; dayOffset < 7; dayOffset++) {
            uint8_t checkDay = (currentWeekday + dayOffset) % 7;

            if (!isDayEnabled(bell.days, checkDay)) continue;

            int diff;
            if (dayOffset == 0) {
                // Stesso giorno
                if (bellMinutes > currentMinutes) {
                    diff = bellMinutes - currentMinutes;
                } else if (bellMinutes == currentMinutes && t.second < 2) {
                    diff = 0;  // Sta per suonare
                } else {
                    continue;  // Gia' passata oggi
                }
            } else {
                // Giorni futuri
                diff = (dayOffset * 24 * 60) + bellMinutes - currentMinutes;
                if (diff < 0) diff += 7 * 24 * 60;
            }

            if (diff < minDiff) {
                minDiff = diff;
                nextIndex = i;
            }
        }
    }

    return nextIndex;
}

// --- Calcola secondi alla prossima campanella di oggi ---
int getSecondsToNextBellToday() {
    if (!isTimeSet() || !settings.globalEnabled || bellCount == 0) {
        return -1;
    }

    CurrentTime t = getTime();
    uint8_t currentWeekday = t.weekday;
    int currentSeconds = t.hour * 3600 + t.minute * 60 + t.second;

    int minSeconds = 86400 + 1;  // Piu' di un giorno

    for (uint8_t i = 0; i < bellCount; i++) {
        Bell& bell = bells[i];
        if (!bell.enabled) continue;
        if (!isDayEnabled(bell.days, currentWeekday)) continue;

        int bellSeconds = bell.hour * 3600 + bell.minute * 60;
        int diff = bellSeconds - currentSeconds;

        if (diff > 0 && diff < minSeconds) {
            minSeconds = diff;
        }
    }

    return (minSeconds > 86400) ? -1 : minSeconds;
}

// --- Aggiorna stato LED pre-ring ---
void updatePreRingLed() {
    if (systemStatus.isRinging) {
        // Campanella in corso, LED gia' gestito
        return;
    }

    int secondsToNext = getSecondsToNextBellToday();

    if (secondsToNext < 0) {
        // Nessuna campanella oggi
        if (preRingState != 0) {
            preRingState = 0;
            setRelayLedMode(0);
        }
    }
    else if (secondsToNext <= PRE_RING_IMMINENT_SEC) {
        // Meno di 10 secondi: blink veloce
        if (preRingState != 2) {
            preRingState = 2;
            setRelayLedMode(3);  // Blink fast
            Serial.printf("[SCHEDULER] Campanella imminente: %d secondi\n", secondsToNext);
        }
    }
    else if (secondsToNext <= PRE_RING_WARNING_SEC) {
        // Meno di 1 minuto: blink lento
        if (preRingState != 1) {
            preRingState = 1;
            setRelayLedMode(2);  // Blink slow
            Serial.printf("[SCHEDULER] Campanella in arrivo: %d secondi\n", secondsToNext);
        }
    }
    else {
        // Piu' di 1 minuto: LED spento
        if (preRingState != 0) {
            preRingState = 0;
            setRelayLedMode(0);
        }
    }
}

// --- Ottieni info prossima campanella ---
String getNextBellInfo() {
    int nextIdx = findNextBell();
    if (nextIdx < 0) {
        return "Nessuna";
    }

    Bell& bell = bells[nextIdx];
    char buf[64];
    snprintf(buf, sizeof(buf), "%s alle %02d:%02d",
             bell.type, bell.hour, bell.minute);
    return String(buf);
}

String getNextBellTime() {
    int nextIdx = findNextBell();
    if (nextIdx < 0) {
        return "--:--";
    }

    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d",
             bells[nextIdx].hour, bells[nextIdx].minute);
    return String(buf);
}

String getNextBellType() {
    int nextIdx = findNextBell();
    if (nextIdx < 0) {
        return "Nessuna";
    }
    return String(bells[nextIdx].type);
}

// --- Loop principale scheduler ---
void runScheduler() {
    unsigned long now = millis();

    // Controlla se la campanella in corso deve essere fermata
    if (systemStatus.isRinging) {
        if (now >= systemStatus.ringEndTime) {
            stopRinging();
        }
        return;  // Non attivare altre campanelle mentre una e' in corso
    }

    // Controlla ogni secondo
    if (now - lastSchedulerCheck < SCHEDULER_CHECK_MS) {
        return;
    }
    lastSchedulerCheck = now;

    // Aggiorna LED pre-ring
    updatePreRingLed();

    // Se l'ora non e' sincronizzata, non attivare campanelle
    if (!isTimeSet()) {
        return;
    }

    // Se le campanelle sono disabilitate globalmente, non fare nulla
    if (!settings.globalEnabled) {
        return;
    }

    CurrentTime t = getTime();

    // Evita attivazioni multiple nello stesso minuto
    if (t.minute == lastMinuteChecked && t.second > 1) {
        return;
    }

    // Controlla solo nei primi 2 secondi del minuto
    if (t.second > 1) {
        return;
    }

    lastMinuteChecked = t.minute;

    // Cerca campanelle che devono suonare adesso
    for (uint8_t i = 0; i < bellCount; i++) {
        Bell& bell = bells[i];

        // Salta se disabilitata
        if (!bell.enabled) continue;

        // Controlla ora
        if (bell.hour != t.hour || bell.minute != t.minute) continue;

        // Controlla giorno della settimana
        if (!isDayEnabled(bell.days, t.weekday)) continue;

        // Trovata! Attiva la campanella
        Serial.printf("[SCHEDULER] Match trovato: campanella %d\n", bell.id);
        ringBell(i);
        break;  // Solo una campanella alla volta
    }
}

// --- Attivazione manuale ---
void manualRing(uint8_t durationSeconds) {
    if (durationSeconds < MIN_BELL_DURATION) durationSeconds = MIN_BELL_DURATION;
    if (durationSeconds > MAX_BELL_DURATION) durationSeconds = MAX_BELL_DURATION;

    Serial.printf("[SCHEDULER] Attivazione MANUALE: %d secondi\n", durationSeconds);

    systemStatus.isRinging = true;
    systemStatus.ringingBellId = 0;  // 0 = manuale
    systemStatus.ringEndTime = millis() + (durationSeconds * 1000UL);

    setRelay(true);
    systemStatus.relayOn = true;

    // LED relay fisso
    setRelayLedMode(1);

    // Reset pre-ring
    preRingBellIndex = -1;
    preRingState = 0;
}

// --- Debug: stampa stato scheduler ---
void debugPrintScheduler() {
    Serial.println("[SCHEDULER] === Debug Scheduler ===");
    Serial.printf("Ora sincronizzata: %s\n", isTimeSet() ? "SI" : "NO");
    Serial.printf("Global enabled: %s\n", settings.globalEnabled ? "SI" : "NO");
    Serial.printf("Campanelle: %d\n", bellCount);
    Serial.printf("Relay: %s\n", systemStatus.relayOn ? "ON" : "OFF");
    Serial.printf("Suonando: %s\n", systemStatus.isRinging ? "SI" : "NO");
    Serial.printf("Pre-ring state: %d\n", preRingState);
    Serial.printf("Secondi a prossima: %d\n", getSecondsToNextBellToday());
    Serial.printf("Prossima: %s\n", getNextBellInfo().c_str());
    Serial.println("[SCHEDULER] === Fine Debug ===");
}

#endif // SCHEDULE_ENGINE_H
