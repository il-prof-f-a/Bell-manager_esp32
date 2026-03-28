#ifndef SCHEDULE_ENGINE_H
#define SCHEDULE_ENGINE_H

#include <Arduino.h>
#include "config.h"
#include "bell_types.h"
#include "gpio_control.h"
#include "time_sync.h"
#include "state_sync.h"

// ============================================
// Motore Scheduling Campanelle
// Controlla gli orari e attiva il relay
// ============================================

extern Bell bells[];
extern uint8_t bellCount;
extern Settings settings;
extern SystemStatus systemStatus;

static unsigned long lastSchedulerCheck = 0;
static uint8_t lastMinuteChecked = 255;

static LogEntry bellLog[MAX_LOG_ENTRIES];
static uint8_t logCount = 0;

static int8_t preRingBellIndex = -1;
static uint8_t preRingState = 0;

// --- Aggiungi voce al log ---
void addLogEntry(uint8_t bellId) {
    if (!lockSharedState()) return;

    CurrentTime t = getTime();

    if (logCount >= MAX_LOG_ENTRIES) {
        for (int i = 0; i < MAX_LOG_ENTRIES - 1; i++) {
            bellLog[i] = bellLog[i + 1];
        }
        logCount = MAX_LOG_ENTRIES - 1;
    }

    bellLog[logCount].bellId = bellId;
    bellLog[logCount].hour = t.hour;
    bellLog[logCount].minute = t.minute;
    bellLog[logCount].day = t.day;
    bellLog[logCount].month = t.month;
    bellLog[logCount].year = t.year;
    logCount++;

    unlockSharedState();

    Serial.printf("[SCHEDULER] Log: campanella %d alle %02d:%02d del %02d/%02d\n",
                  bellId, t.hour, t.minute, t.day, t.month);
}

uint8_t copyLogEntries(LogEntry* destination, uint8_t maxEntries) {
    if (destination == NULL || maxEntries == 0) {
        return 0;
    }
    if (!lockSharedState()) return 0;

    uint8_t count = logCount;
    if (count > maxEntries) count = maxEntries;

    for (uint8_t i = 0; i < count; i++) {
        destination[i] = bellLog[i];
    }

    unlockSharedState();
    return count;
}

uint8_t getLogCount() {
    if (!lockSharedState()) return 0;
    uint8_t count = logCount;
    unlockSharedState();
    return count;
}

void clearLog() {
    if (!lockSharedState()) return;
    logCount = 0;
    unlockSharedState();
    Serial.println("[SCHEDULER] Log cancellato");
}

// --- Attiva campanella ---
void ringBell(uint8_t bellIndex) {
    if (!lockSharedState()) return;
    if (bellIndex >= bellCount) {
        unlockSharedState();
        return;
    }

    Bell& bell = bells[bellIndex];

    Serial.printf("[SCHEDULER] ATTIVAZIONE campanella %d: %s (durata: %d sec)\n",
                  bell.id, bell.type, bell.duration);

    systemStatus.isRinging = true;
    systemStatus.ringingBellId = bell.id;
    systemStatus.ringEndTime = millis() + (bell.duration * 1000UL);

    setRelay(true);
    systemStatus.relayOn = true;
    setRelayLedMode(1);

    preRingBellIndex = -1;
    preRingState = 0;

    unlockSharedState();

    addLogEntry(bell.id);
}

// --- Ferma campanella ---
void stopRinging() {
    if (!lockSharedState()) return;

    if (systemStatus.isRinging) {
        Serial.printf("[SCHEDULER] STOP campanella %d\n", systemStatus.ringingBellId);
    }

    setRelay(false);
    systemStatus.relayOn = false;
    systemStatus.isRinging = false;
    systemStatus.ringingBellId = 0;
    systemStatus.ringEndTime = 0;

    setRelayLedMode(0);

    unlockSharedState();
}

// --- Trova prossima campanella ---
int findNextBell() {
    if (!lockSharedState()) return -1;

    if (!isTimeSet() || !settings.globalEnabled || bellCount == 0) {
        unlockSharedState();
        return -1;
    }

    CurrentTime t = getTime();
    int currentMinutes = t.hour * 60 + t.minute;
    uint8_t currentWeekday = t.weekday;

    int nextIndex = -1;
    int minDiff = 24 * 60 * 7 + 1;

    for (uint8_t i = 0; i < bellCount; i++) {
        Bell& bell = bells[i];
        if (!bell.enabled) continue;

        int bellMinutes = bell.hour * 60 + bell.minute;

        for (int dayOffset = 0; dayOffset < 7; dayOffset++) {
            uint8_t checkDay = (currentWeekday + dayOffset) % 7;
            if (!isDayEnabled(bell.days, checkDay)) continue;

            int diff;
            if (dayOffset == 0) {
                if (bellMinutes > currentMinutes) {
                    diff = bellMinutes - currentMinutes;
                } else if (bellMinutes == currentMinutes && t.second < 2) {
                    diff = 0;
                } else {
                    continue;
                }
            } else {
                diff = (dayOffset * 24 * 60) + bellMinutes - currentMinutes;
                if (diff < 0) diff += 7 * 24 * 60;
            }

            if (diff < minDiff) {
                minDiff = diff;
                nextIndex = i;
            }
        }
    }

    unlockSharedState();
    return nextIndex;
}

// --- Calcola secondi alla prossima campanella di oggi ---
int getSecondsToNextBellToday() {
    if (!lockSharedState()) return -1;

    if (!isTimeSet() || !settings.globalEnabled || bellCount == 0) {
        unlockSharedState();
        return -1;
    }

    CurrentTime t = getTime();
    uint8_t currentWeekday = t.weekday;
    int currentSeconds = t.hour * 3600 + t.minute * 60 + t.second;

    int minSeconds = 86400 + 1;

    for (uint8_t i = 0; i < bellCount; i++) {
        Bell& bell = bells[i];
        if (!bell.enabled) continue;
        if (!isDayEnabled(bell.days, currentWeekday)) continue;

        int bellSeconds = bell.hour * 3600 + bell.minute * 60;
        int diff = bellSeconds - currentSeconds;

        if (diff > 0 && diff < minSeconds) {
            minSeconds = diff;
            preRingBellIndex = i;
        }
    }

    unlockSharedState();
    return (minSeconds > 86400) ? -1 : minSeconds;
}

// --- Calcola secondi alla prossima campanella assoluta ---
int getSecondsToNextBell() {
    if (!lockSharedState()) return -1;

    if (!isTimeSet() || !settings.globalEnabled || bellCount == 0) {
        unlockSharedState();
        return -1;
    }

    CurrentTime t = getTime();
    int currentSeconds = t.hour * 3600 + t.minute * 60 + t.second;
    uint8_t currentWeekday = t.weekday;

    int minSeconds = (7 * 86400) + 1;

    for (uint8_t i = 0; i < bellCount; i++) {
        Bell& bell = bells[i];
        if (!bell.enabled) continue;

        int bellSeconds = bell.hour * 3600 + bell.minute * 60;

        for (int dayOffset = 0; dayOffset < 7; dayOffset++) {
            uint8_t checkDay = (currentWeekday + dayOffset) % 7;
            if (!isDayEnabled(bell.days, checkDay)) continue;

            int diff;
            if (dayOffset == 0) {
                diff = bellSeconds - currentSeconds;
                if (diff < 0) continue;
            } else {
                diff = (dayOffset * 86400) + bellSeconds - currentSeconds;
            }

            if (diff < minSeconds) {
                minSeconds = diff;
            }
        }
    }

    unlockSharedState();
    return (minSeconds > (7 * 86400)) ? -1 : minSeconds;
}

// --- Aggiorna stato LED pre-ring ---
void updatePreRingLed() {
    bool isRingingNow = false;
    if (lockSharedState()) {
        isRingingNow = systemStatus.isRinging;
        unlockSharedState();
    }

    if (isRingingNow) {
        return;
    }

    int secondsToNext = getSecondsToNextBellToday();

    if (secondsToNext < 0) {
        if (preRingState != 0) {
            preRingState = 0;
            preRingBellIndex = -1;
            setRelayLedMode(0);
        }
    }
    else if (secondsToNext <= PRE_RING_IMMINENT_SEC) {
        if (preRingState != 2) {
            preRingState = 2;
            setRelayLedMode(3);
            Serial.printf("[SCHEDULER] Campanella imminente: %d secondi\n", secondsToNext);
        }
    }
    else if (secondsToNext <= PRE_RING_WARNING_SEC) {
        if (preRingState != 1) {
            preRingState = 1;
            setRelayLedMode(2);
            Serial.printf("[SCHEDULER] Campanella in arrivo: %d secondi\n", secondsToNext);
        }
    }
    else {
        if (preRingState != 0) {
            preRingState = 0;
            preRingBellIndex = -1;
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

    if (!lockSharedState()) return "Nessuna";
    Bell bell = bells[nextIdx];
    unlockSharedState();

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

    if (!lockSharedState()) return "--:--";
    Bell bell = bells[nextIdx];
    unlockSharedState();

    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", bell.hour, bell.minute);
    return String(buf);
}

String getNextBellType() {
    int nextIdx = findNextBell();
    if (nextIdx < 0) {
        return "Nessuna";
    }

    if (!lockSharedState()) return "Nessuna";
    String bellType = String(bells[nextIdx].type);
    unlockSharedState();
    return bellType;
}

// --- Loop principale scheduler ---
void runScheduler() {
    unsigned long now = millis();

    bool shouldStopCurrentRing = false;
    if (lockSharedState()) {
        shouldStopCurrentRing = systemStatus.isRinging && now >= systemStatus.ringEndTime;
        unlockSharedState();
    }

    if (shouldStopCurrentRing) {
        stopRinging();
        return;
    }

    bool isRingingNow = false;
    if (lockSharedState()) {
        isRingingNow = systemStatus.isRinging;
        unlockSharedState();
    }

    if (isRingingNow) {
        return;
    }

    if (now - lastSchedulerCheck < SCHEDULER_CHECK_MS) {
        return;
    }
    lastSchedulerCheck = now;

    updatePreRingLed();

    if (!isTimeSet()) {
        return;
    }

    if (!lockSharedState()) return;
    if (!settings.globalEnabled) {
        unlockSharedState();
        return;
    }

    CurrentTime t = getTime();

    if (t.minute == lastMinuteChecked && t.second > 1) {
        unlockSharedState();
        return;
    }

    if (t.second > 1) {
        unlockSharedState();
        return;
    }

    lastMinuteChecked = t.minute;

    for (uint8_t i = 0; i < bellCount; i++) {
        Bell& bell = bells[i];

        if (!bell.enabled) continue;
        if (bell.hour != t.hour || bell.minute != t.minute) continue;
        if (!isDayEnabled(bell.days, t.weekday)) continue;

        Serial.printf("[SCHEDULER] Match trovato: campanella %d\n", bell.id);
        unlockSharedState();
        ringBell(i);
        return;
    }

    unlockSharedState();
}

// --- Attivazione manuale ---
void manualRing(uint8_t durationSeconds) {
    if (durationSeconds < MIN_BELL_DURATION) durationSeconds = MIN_BELL_DURATION;
    if (durationSeconds > MAX_BELL_DURATION) durationSeconds = MAX_BELL_DURATION;

    if (!lockSharedState()) return;

    Serial.printf("[SCHEDULER] Attivazione MANUALE: %d secondi\n", durationSeconds);

    systemStatus.isRinging = true;
    systemStatus.ringingBellId = 0;
    systemStatus.ringEndTime = millis() + (durationSeconds * 1000UL);

    setRelay(true);
    systemStatus.relayOn = true;

    setRelayLedMode(1);

    preRingBellIndex = -1;
    preRingState = 0;

    unlockSharedState();
}

// --- Debug: stampa stato scheduler ---
void debugPrintScheduler() {
    if (!lockSharedState()) return;

    Serial.println("[SCHEDULER] === Debug Scheduler ===");
    Serial.printf("Ora sincronizzata: %s\n", isTimeSet() ? "SI" : "NO");
    Serial.printf("Global enabled: %s\n", settings.globalEnabled ? "SI" : "NO");
    Serial.printf("Campanelle: %d\n", bellCount);
    Serial.printf("Relay: %s\n", systemStatus.relayOn ? "ON" : "OFF");
    Serial.printf("Suonando: %s\n", systemStatus.isRinging ? "SI" : "NO");
    Serial.printf("Pre-ring state: %d\n", preRingState);
    unlockSharedState();

    Serial.printf("Secondi a prossima: %d\n", getSecondsToNextBellToday());
    Serial.printf("Prossima: %s\n", getNextBellInfo().c_str());
    Serial.println("[SCHEDULER] === Fine Debug ===");
}

#endif // SCHEDULE_ENGINE_H
