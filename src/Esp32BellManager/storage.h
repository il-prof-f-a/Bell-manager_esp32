#ifndef STORAGE_H
#define STORAGE_H

#include <Preferences.h>
#include "config.h"
#include "bell_types.h"

// ============================================
// Persistenza Dati su NVS (Non-Volatile Storage)
// Usa la libreria Preferences dell'ESP32
// ============================================

static Preferences preferences;

// --- Inizializzazione Storage ---
void initStorage() {
    Serial.println("[STORAGE] Inizializzazione NVS...");
}

// === CAMPANELLE ===

bool saveBells(Bell* bells, uint8_t count) {
    if (count > MAX_BELLS) count = MAX_BELLS;

    preferences.begin(STORAGE_NAMESPACE, false);

    preferences.putUChar(STORAGE_KEY_BELL_COUNT, count);

    for (uint8_t i = 0; i < count; i++) {
        char key[16];
        snprintf(key, sizeof(key), "bell%d", i);
        preferences.putBytes(key, &bells[i], sizeof(Bell));
    }

    preferences.end();

    Serial.printf("[STORAGE] Salvate %d campanelle\n", count);
    return true;
}

uint8_t loadBells(Bell* bells, uint8_t maxCount) {
    if (maxCount > MAX_BELLS) maxCount = MAX_BELLS;

    preferences.begin(STORAGE_NAMESPACE, true);

    uint8_t count = preferences.getUChar(STORAGE_KEY_BELL_COUNT, 0);
    if (count > maxCount) count = maxCount;

    for (uint8_t i = 0; i < count; i++) {
        char key[16];
        snprintf(key, sizeof(key), "bell%d", i);
        size_t len = preferences.getBytes(key, &bells[i], sizeof(Bell));
        if (len != sizeof(Bell)) {
            initBell(bells[i]);
            bells[i].id = i + 1;
        }
    }

    preferences.end();

    Serial.printf("[STORAGE] Caricate %d campanelle\n", count);
    return count;
}

bool clearBells() {
    preferences.begin(STORAGE_NAMESPACE, false);

    uint8_t count = preferences.getUChar(STORAGE_KEY_BELL_COUNT, 0);

    for (uint8_t i = 0; i < count; i++) {
        char key[16];
        snprintf(key, sizeof(key), "bell%d", i);
        preferences.remove(key);
    }

    preferences.putUChar(STORAGE_KEY_BELL_COUNT, 0);
    preferences.end();

    Serial.println("[STORAGE] Campanelle cancellate");
    return true;
}

// === IMPOSTAZIONI ===

bool saveSettings(Settings& settings) {
    preferences.begin(STORAGE_NAMESPACE, false);

    preferences.putString("instName", settings.institutionName);
    preferences.putBool("globalEn", settings.globalEnabled);

    preferences.end();

    Serial.printf("[STORAGE] Impostazioni salvate - Nome: %s, Global: %s\n",
                  settings.institutionName,
                  settings.globalEnabled ? "ON" : "OFF");
    return true;
}

bool loadSettings(Settings& settings) {
    preferences.begin(STORAGE_NAMESPACE, true);

    String name = preferences.getString("instName", "Istituzione");
    strncpy(settings.institutionName, name.c_str(), MAX_NAME_LENGTH - 1);
    settings.institutionName[MAX_NAME_LENGTH - 1] = '\0';

    settings.globalEnabled = preferences.getBool("globalEn", true);

    preferences.end();

    Serial.printf("[STORAGE] Impostazioni caricate - Nome: %s, Global: %s\n",
                  settings.institutionName,
                  settings.globalEnabled ? "ON" : "OFF");
    return true;
}

// === CREDENZIALI WIFI ===

bool saveWiFiCredentials(const char* ssid, const char* password) {
    preferences.begin(STORAGE_NAMESPACE, false);

    preferences.putString(STORAGE_KEY_WIFI_SSID, ssid);
    preferences.putString(STORAGE_KEY_WIFI_PASS, password);

    preferences.end();

    Serial.printf("[STORAGE] Credenziali WiFi salvate: SSID=%s\n", ssid);
    return true;
}

bool loadWiFiCredentials(char* ssid, size_t ssidLen, char* password, size_t passLen) {
    preferences.begin(STORAGE_NAMESPACE, true);

    String storedSSID = preferences.getString(STORAGE_KEY_WIFI_SSID, "");
    String storedPass = preferences.getString(STORAGE_KEY_WIFI_PASS, "");

    preferences.end();

    if (storedSSID.length() == 0) {
        Serial.println("[STORAGE] Nessuna credenziale WiFi salvata");
        ssid[0] = '\0';
        password[0] = '\0';
        return false;
    }

    strncpy(ssid, storedSSID.c_str(), ssidLen - 1);
    ssid[ssidLen - 1] = '\0';

    strncpy(password, storedPass.c_str(), passLen - 1);
    password[passLen - 1] = '\0';

    Serial.printf("[STORAGE] Credenziali WiFi caricate: SSID=%s\n", ssid);
    return true;
}

bool hasWiFiCredentials() {
    preferences.begin(STORAGE_NAMESPACE, true);
    String storedSSID = preferences.getString(STORAGE_KEY_WIFI_SSID, "");
    preferences.end();
    return storedSSID.length() > 0;
}

bool clearWiFiCredentials() {
    preferences.begin(STORAGE_NAMESPACE, false);
    preferences.remove(STORAGE_KEY_WIFI_SSID);
    preferences.remove(STORAGE_KEY_WIFI_PASS);
    preferences.end();

    Serial.println("[STORAGE] Credenziali WiFi cancellate");
    return true;
}

// === TIMEZONE ===

bool saveTimezone(int32_t gmtOffset, int32_t dstOffset) {
    preferences.begin(STORAGE_NAMESPACE, false);

    preferences.putInt(STORAGE_KEY_GMT_OFFSET, gmtOffset);
    preferences.putInt(STORAGE_KEY_DST_OFFSET, dstOffset);

    preferences.end();

    Serial.printf("[STORAGE] Timezone salvato: GMT%+d, DST%+d\n",
                  gmtOffset / 3600, dstOffset / 3600);
    return true;
}

bool loadTimezone(int32_t* gmtOffset, int32_t* dstOffset) {
    preferences.begin(STORAGE_NAMESPACE, true);

    *gmtOffset = preferences.getInt(STORAGE_KEY_GMT_OFFSET, GMT_OFFSET_SEC);
    *dstOffset = preferences.getInt(STORAGE_KEY_DST_OFFSET, DAYLIGHT_OFFSET_SEC);

    preferences.end();

    Serial.printf("[STORAGE] Timezone caricato: GMT%+d, DST%+d\n",
                  *gmtOffset / 3600, *dstOffset / 3600);
    return true;
}

// === UTILITY ===

bool clearAllData() {
    preferences.begin(STORAGE_NAMESPACE, false);
    bool result = preferences.clear();
    preferences.end();

    if (result) {
        Serial.println("[STORAGE] Tutti i dati cancellati");
    } else {
        Serial.println("[STORAGE] Errore cancellazione dati");
    }
    return result;
}

size_t getFreeSpace() {
    preferences.begin(STORAGE_NAMESPACE, true);
    size_t free = preferences.freeEntries();
    preferences.end();
    return free;
}

void debugPrintStorage() {
    Serial.println("[STORAGE] === Debug Storage ===");

    preferences.begin(STORAGE_NAMESPACE, true);

    uint8_t count = preferences.getUChar(STORAGE_KEY_BELL_COUNT, 0);
    Serial.printf("Campanelle salvate: %d\n", count);

    String name = preferences.getString("instName", "(vuoto)");
    Serial.printf("Nome istituzione: %s\n", name.c_str());

    bool global = preferences.getBool("globalEn", true);
    Serial.printf("Global enabled: %s\n", global ? "SI" : "NO");

    String ssid = preferences.getString(STORAGE_KEY_WIFI_SSID, "(vuoto)");
    Serial.printf("WiFi SSID: %s\n", ssid.c_str());

    int32_t gmt = preferences.getInt(STORAGE_KEY_GMT_OFFSET, GMT_OFFSET_SEC);
    int32_t dst = preferences.getInt(STORAGE_KEY_DST_OFFSET, DAYLIGHT_OFFSET_SEC);
    Serial.printf("Timezone: GMT%+d, DST%+d\n", gmt / 3600, dst / 3600);

    size_t free = preferences.freeEntries();
    Serial.printf("Entries libere: %d\n", free);

    preferences.end();

    Serial.println("[STORAGE] === Fine Debug ===");
}

#endif // STORAGE_H
