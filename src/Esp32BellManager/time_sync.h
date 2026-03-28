#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include <WiFi.h>
#include "config.h"
#include "bell_types.h"

// ============================================
// Sincronizzazione Tempo via NTP
// Supporta anche il ripristino da tempo persistito
// ============================================

enum TimeSource {
    TIME_SOURCE_NONE = 0,
    TIME_SOURCE_PERSISTED = 1,
    TIME_SOURCE_NTP = 2
};

static bool ntpSynced = false;
static bool timeSetAvailable = false;
static unsigned long lastNtpSync = 0;
static unsigned long lastSyncAttempt = 0;
static int32_t gmtOffsetSec = GMT_OFFSET_SEC;
static int32_t dstOffsetSec = DAYLIGHT_OFFSET_SEC;
static TimeSource currentTimeSource = TIME_SOURCE_NONE;

static const uint32_t MIN_VALID_EPOCH = 1704067200UL;  // 01/01/2024 00:00:00 UTC

// --- Inizializzazione NTP ---
void initNTP() {
    ntpSynced = false;
    timeSetAvailable = false;
    lastNtpSync = 0;
    lastSyncAttempt = 0;
    currentTimeSource = TIME_SOURCE_NONE;
    Serial.println("[NTP] Inizializzazione completata");
}

// --- Imposta timezone ---
void setTimezone(int32_t gmtOffset, int32_t dstOffset) {
    gmtOffsetSec = gmtOffset;
    dstOffsetSec = dstOffset;
    configTime(gmtOffsetSec, dstOffsetSec, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
    Serial.printf("[NTP] Timezone impostato: GMT%+d, DST%+d\n",
                  gmtOffsetSec / 3600, dstOffsetSec / 3600);
}

int32_t getGmtOffset() {
    return gmtOffsetSec;
}

int32_t getDstOffset() {
    return dstOffsetSec;
}

TimeSource getTimeSource() {
    return currentTimeSource;
}

const char* getTimeSourceName() {
    switch (currentTimeSource) {
        case TIME_SOURCE_PERSISTED: return "persistito";
        case TIME_SOURCE_NTP: return "ntp";
        default: return "nessuno";
    }
}

bool isTimeRestoredFromStorage() {
    return currentTimeSource == TIME_SOURCE_PERSISTED;
}

// --- Ripristina tempo da epoch persistito ---
bool restoreTimeFromEpoch(uint32_t epochSeconds) {
    if (epochSeconds < MIN_VALID_EPOCH) {
        Serial.printf("[NTP] Epoch persistito non valido: %lu\n", epochSeconds);
        return false;
    }

    struct timeval tv;
    tv.tv_sec = epochSeconds;
    tv.tv_usec = 0;

    if (settimeofday(&tv, NULL) != 0) {
        Serial.println("[NTP] Errore settimeofday() durante il ripristino");
        return false;
    }

    timeSetAvailable = true;
    ntpSynced = false;
    currentTimeSource = TIME_SOURCE_PERSISTED;

    time_t raw = epochSeconds;
    struct tm timeinfo;
    localtime_r(&raw, &timeinfo);

    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%d/%m/%Y %H:%M:%S", &timeinfo);
    Serial.printf("[NTP] Tempo ripristinato da memoria: %s\n", timeStr);
    return true;
}

// --- Sincronizza con NTP ---
bool syncNTP() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP] WiFi non connesso, sync impossibile");
        return false;
    }

    lastSyncAttempt = millis();
    Serial.println("[NTP] Avvio sincronizzazione...");

    configTime(gmtOffsetSec, dstOffsetSec, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);

    unsigned long startWait = millis();
    struct tm timeinfo;

    while (!getLocalTime(&timeinfo, 1000)) {
        if (millis() - startWait > NTP_SYNC_TIMEOUT_MS) {
            Serial.println("[NTP] Timeout sincronizzazione");
            return false;
        }
        delay(100);
    }

    ntpSynced = true;
    timeSetAvailable = true;
    currentTimeSource = TIME_SOURCE_NTP;
    lastNtpSync = millis();

    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%d/%m/%Y %H:%M:%S", &timeinfo);
    Serial.printf("[NTP] Sincronizzato: %s\n", timeStr);

    return true;
}

// --- Verifica e risincronizza periodicamente ---
void updateNTP() {
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    unsigned long now = millis();

    if (ntpSynced && (now - lastNtpSync >= NTP_SYNC_INTERVAL_MS)) {
        Serial.println("[NTP] Risincronizzazione periodica...");
        syncNTP();
    }

    if (!ntpSynced && (now - lastSyncAttempt >= 30000)) {
        syncNTP();
    }
}

// --- Getter stato sincronizzazione ---
bool isNtpSynced() {
    return ntpSynced;
}

bool isTimeSet() {
    return timeSetAvailable;
}

unsigned long getLastSyncTime() {
    return lastNtpSync;
}

// --- Getter per il tempo corrente ---
CurrentTime getTime() {
    CurrentTime ct;
    struct tm timeinfo;

    if (getLocalTime(&timeinfo, 10)) {
        ct.hour = timeinfo.tm_hour;
        ct.minute = timeinfo.tm_min;
        ct.second = timeinfo.tm_sec;
        ct.day = timeinfo.tm_mday;
        ct.month = timeinfo.tm_mon + 1;
        ct.year = timeinfo.tm_year + 1900;
        ct.weekday = (timeinfo.tm_wday + 6) % 7;
    } else {
        ct.hour = 0;
        ct.minute = 0;
        ct.second = 0;
        ct.day = 1;
        ct.month = 1;
        ct.year = 2025;
        ct.weekday = 0;
    }

    return ct;
}

uint8_t getHour() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
        return timeinfo.tm_hour;
    }
    return 0;
}

uint8_t getMinute() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
        return timeinfo.tm_min;
    }
    return 0;
}

uint8_t getSecond() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
        return timeinfo.tm_sec;
    }
    return 0;
}

uint8_t getDay() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
        return timeinfo.tm_mday;
    }
    return 1;
}

uint8_t getMonth() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
        return timeinfo.tm_mon + 1;
    }
    return 1;
}

uint16_t getYear() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
        return timeinfo.tm_year + 1900;
    }
    return 2025;
}

uint8_t getWeekday() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
        return (timeinfo.tm_wday + 6) % 7;
    }
    return 0;
}

// --- Formattazione tempo ---
String getTimeString() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
        char buf[9];
        strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
        return String(buf);
    }
    return "--:--:--";
}

String getTimeStringShort() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
        char buf[6];
        strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
        return String(buf);
    }
    return "--:--";
}

String getDateString() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
        char buf[11];
        strftime(buf, sizeof(buf), "%d/%m/%Y", &timeinfo);
        return String(buf);
    }
    return "--/--/----";
}

String getDateTimeString() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 10)) {
        char buf[20];
        strftime(buf, sizeof(buf), "%H:%M %d/%m/%Y", &timeinfo);
        return String(buf);
    }
    return "--:-- --/--/----";
}

// --- Ottieni timestamp Unix ---
time_t getUnixTime() {
    time_t now;
    time(&now);
    return now;
}

// --- Calcola secondi fino a un orario specifico oggi ---
int getSecondsUntil(uint8_t hour, uint8_t minute) {
    if (!timeSetAvailable) return -1;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10)) return -1;

    int currentSeconds = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
    int targetSeconds = hour * 3600 + minute * 60;

    return targetSeconds - currentSeconds;
}

// --- Debug ---
void debugPrintTime() {
    Serial.println("[TIME] === Debug Tempo ===");
    Serial.printf("NTP Sincronizzato: %s\n", ntpSynced ? "SI" : "NO");
    Serial.printf("Tempo disponibile: %s\n", timeSetAvailable ? "SI" : "NO");
    Serial.printf("Sorgente tempo: %s\n", getTimeSourceName());
    Serial.printf("Ultima sync: %lu ms fa\n", millis() - lastNtpSync);
    Serial.printf("Ora: %s\n", getTimeString().c_str());
    Serial.printf("Data: %s\n", getDateString().c_str());
    Serial.printf("Giorno settimana: %s\n", getDayName(getWeekday()));
    Serial.printf("GMT Offset: %d sec\n", gmtOffsetSec);
    Serial.printf("DST Offset: %d sec\n", dstOffsetSec);
    Serial.println("[TIME] === Fine Debug ===");
}

#endif // TIME_SYNC_H
