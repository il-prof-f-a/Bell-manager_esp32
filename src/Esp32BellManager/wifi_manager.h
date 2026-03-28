#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include "config.h"
#include "storage.h"

// ============================================
// Gestione WiFi Station/AP con fallback multi-rete
// Reti candidate:
// 1. Rete configurata dall'utente
// 2. Hotspot di emergenza statico da config.h
// ============================================

static WiFiState wifiState = WIFI_STATE_DISCONNECTED;
static unsigned long lastDisconnectTime = 0;
static unsigned long lastConnectAttempt = 0;
static unsigned long connectionStartTime = 0;
static bool apModeRequested = false;

static char wifiSSID[MAX_SSID_LENGTH] = "";
static char wifiPassword[MAX_PASS_LENGTH] = "";

static char currentAttemptSSID[MAX_SSID_LENGTH] = "";
static char currentAttemptPassword[MAX_PASS_LENGTH] = "";
static bool currentAttemptIsRescue = false;

static char activeWiFiSSID[MAX_SSID_LENGTH] = "";
static bool activeWiFiIsRescue = false;
static int8_t nextNetworkIndex = -1;
static uint8_t wifiTxPowerLevel = DEFAULT_WIFI_TX_POWER_LEVEL;

extern void onWifiStateChanged(WiFiState newState);

wifi_power_t getWiFiTxPowerEnum(uint8_t level) {
    switch (level) {
        case 0: return WIFI_POWER_19_5dBm;
        case 1: return WIFI_POWER_17dBm;
        case 2: return WIFI_POWER_15dBm;
        case 3: return WIFI_POWER_11dBm;
        case 4: return WIFI_POWER_8_5dBm;
        case 5: return WIFI_POWER_5dBm;
        case 6: return WIFI_POWER_2dBm;
        default: return WIFI_POWER_11dBm;
    }
}

const char* getWiFiTxPowerLabel(uint8_t level) {
    switch (level) {
        case 0: return "Massima (19.5 dBm)";
        case 1: return "Alta (17 dBm)";
        case 2: return "Medio-alta (15 dBm)";
        case 3: return "Media (11 dBm)";
        case 4: return "Medio-bassa (8.5 dBm)";
        case 5: return "Bassa (5 dBm)";
        case 6: return "Molto bassa (2 dBm)";
        default: return "Media (11 dBm)";
    }
}

void applyConfiguredWiFiTxPower() {
    wifi_power_t powerEnum = getWiFiTxPowerEnum(wifiTxPowerLevel);
    WiFi.setTxPower(powerEnum);
    delay(100);
    Serial.printf("[WIFI] Potenza TX impostata: livello %u - %s\n",
                  wifiTxPowerLevel,
                  getWiFiTxPowerLabel(wifiTxPowerLevel));
}

bool isRescueNetworkConfigured() {
    return strlen(RESCUE_WIFI_SSID) > 0;
}

const char* getConfiguredWiFiSSID() {
    return wifiSSID;
}

bool isUsingRescueNetwork() {
    return activeWiFiIsRescue && (wifiState == WIFI_STATE_CONNECTED || wifiState == WIFI_STATE_SYNCED);
}

uint8_t getWiFiTxPowerLevel() {
    return wifiTxPowerLevel;
}

void setWiFiTxPowerLevel(uint8_t level, bool applyNow = true) {
    if (level > 6) {
        level = DEFAULT_WIFI_TX_POWER_LEVEL;
    }

    wifiTxPowerLevel = level;

    if (applyNow) {
        applyConfiguredWiFiTxPower();
    }
}

uint8_t buildWiFiCandidates(const char** ssids, const char** passwords, bool* rescueFlags) {
    uint8_t count = 0;

    if (strlen(wifiSSID) > 0) {
        ssids[count] = wifiSSID;
        passwords[count] = wifiPassword;
        rescueFlags[count] = false;
        count++;
    }

    if (isRescueNetworkConfigured()) {
        bool duplicate = (strlen(wifiSSID) > 0 &&
                          strcmp(RESCUE_WIFI_SSID, wifiSSID) == 0 &&
                          strcmp(RESCUE_WIFI_PASS, wifiPassword) == 0);
        if (!duplicate) {
            ssids[count] = RESCUE_WIFI_SSID;
            passwords[count] = RESCUE_WIFI_PASS;
            rescueFlags[count] = true;
            count++;
        }
    }

    return count;
}

bool selectNextWiFiCandidate() {
    const char* ssids[2];
    const char* passwords[2];
    bool rescueFlags[2];

    uint8_t count = buildWiFiCandidates(ssids, passwords, rescueFlags);
    if (count == 0) {
        currentAttemptSSID[0] = '\0';
        currentAttemptPassword[0] = '\0';
        currentAttemptIsRescue = false;
        return false;
    }

    nextNetworkIndex = (nextNetworkIndex + 1) % count;

    strncpy(currentAttemptSSID, ssids[nextNetworkIndex], MAX_SSID_LENGTH - 1);
    currentAttemptSSID[MAX_SSID_LENGTH - 1] = '\0';

    strncpy(currentAttemptPassword, passwords[nextNetworkIndex], MAX_PASS_LENGTH - 1);
    currentAttemptPassword[MAX_PASS_LENGTH - 1] = '\0';

    currentAttemptIsRescue = rescueFlags[nextNetworkIndex];
    return true;
}

void clearActiveWiFiInfo() {
    activeWiFiSSID[0] = '\0';
    activeWiFiIsRescue = false;
}

// --- Inizializzazione ---
void initWiFiManager() {
    wifiState = WIFI_STATE_DISCONNECTED;
    lastDisconnectTime = 0;
    lastConnectAttempt = 0;
    connectionStartTime = 0;
    apModeRequested = false;
    nextNetworkIndex = -1;
    clearActiveWiFiInfo();

    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);

    Serial.println("[WIFI] Manager inizializzato");
}

// --- Imposta credenziali ---
void setWiFiCredentials(const char* ssid, const char* password) {
    strncpy(wifiSSID, ssid, MAX_SSID_LENGTH - 1);
    wifiSSID[MAX_SSID_LENGTH - 1] = '\0';
    strncpy(wifiPassword, password, MAX_PASS_LENGTH - 1);
    wifiPassword[MAX_PASS_LENGTH - 1] = '\0';
    nextNetworkIndex = -1;
    Serial.printf("[WIFI] Credenziali impostate: SSID=%s\n", wifiSSID);
}

const char* getWiFiSSID() {
    if (wifiState == WIFI_STATE_AP_MODE) {
        return AP_SSID;
    }
    if ((wifiState == WIFI_STATE_CONNECTED || wifiState == WIFI_STATE_SYNCED) && strlen(activeWiFiSSID) > 0) {
        return activeWiFiSSID;
    }
    if (wifiState == WIFI_STATE_CONNECTING && strlen(currentAttemptSSID) > 0) {
        return currentAttemptSSID;
    }
    if (strlen(wifiSSID) > 0) {
        return wifiSSID;
    }
    return "";
}

// --- Getter stato ---
WiFiState getWiFiState() {
    return wifiState;
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool isInAPMode() {
    return wifiState == WIFI_STATE_AP_MODE;
}

String getLocalIP() {
    if (wifiState == WIFI_STATE_AP_MODE) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

// --- Avvia modalita' Access Point ---
void startAPMode() {
    Serial.println("[WIFI] Avvio modalita' Access Point...");

    WiFi.mode(WIFI_OFF);
    delay(200);

    WiFi.mode(WIFI_AP);
    delay(200);

    applyConfiguredWiFiTxPower();

    Serial.println("[WIFI] Avvio softAP()...");
    WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL);

    clearActiveWiFiInfo();
    wifiState = WIFI_STATE_AP_MODE;
    apModeRequested = false;

    Serial.printf("[WIFI] AP attivo: SSID=%s, IP=%s\n",
                  AP_SSID, WiFi.softAPIP().toString().c_str());

    onWifiStateChanged(wifiState);
}

// --- Richiedi passaggio a modalita' AP ---
void requestAPMode() {
    Serial.println("[WIFI] Richiesta modalita' configurazione");
    apModeRequested = true;
}

// --- Avvia connessione Station ---
void startStationMode() {
    if (!selectNextWiFiCandidate()) {
        Serial.println("[WIFI] Nessuna rete disponibile, avvio AP");
        startAPMode();
        return;
    }

    Serial.printf("[WIFI] Connessione a: %s%s\n",
                  currentAttemptSSID,
                  currentAttemptIsRescue ? " [EMERGENZA]" : "");

    WiFi.mode(WIFI_OFF);
    delay(200);

    WiFi.mode(WIFI_STA);
    delay(200);

    applyConfiguredWiFiTxPower();

    Serial.println("[WIFI] Avvio WiFi.begin()...");
    WiFi.begin(currentAttemptSSID, currentAttemptPassword);

    clearActiveWiFiInfo();
    wifiState = WIFI_STATE_CONNECTING;
    connectionStartTime = millis();
    lastConnectAttempt = millis();

    Serial.println("[WIFI] WiFi.begin() completato");
    onWifiStateChanged(wifiState);
}

// --- Disconnetti e ferma WiFi ---
void stopWiFi() {
    WiFi.disconnect(false);
    delay(100);
    WiFi.mode(WIFI_OFF);
    delay(100);
    clearActiveWiFiInfo();
    wifiState = WIFI_STATE_DISCONNECTED;
    Serial.println("[WIFI] WiFi fermato");
}

// --- Riavvia ESP32 ---
void restartDevice() {
    Serial.println("[WIFI] Riavvio dispositivo...");
    delay(1000);
    ESP.restart();
}

// --- Loop gestione WiFi ---
void updateWiFi() {
    unsigned long now = millis();

    if (apModeRequested && wifiState != WIFI_STATE_AP_MODE) {
        startAPMode();
        return;
    }

    if (wifiState == WIFI_STATE_AP_MODE) {
        return;
    }

    switch (wifiState) {
        case WIFI_STATE_DISCONNECTED:
            if (strlen(wifiSSID) > 0 || isRescueNetworkConfigured()) {
                if (now - lastConnectAttempt >= WIFI_RECONNECT_INTERVAL_MS) {
                    startStationMode();
                }
            } else {
                startAPMode();
            }
            break;

        case WIFI_STATE_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                wifiState = WIFI_STATE_CONNECTED;
                lastDisconnectTime = 0;

                strncpy(activeWiFiSSID, currentAttemptSSID, MAX_SSID_LENGTH - 1);
                activeWiFiSSID[MAX_SSID_LENGTH - 1] = '\0';
                activeWiFiIsRescue = currentAttemptIsRescue;

                Serial.printf("[WIFI] Connesso! IP: %s, rete: %s%s\n",
                              WiFi.localIP().toString().c_str(),
                              activeWiFiSSID,
                              activeWiFiIsRescue ? " [EMERGENZA]" : "");
                onWifiStateChanged(wifiState);
            } else if (now - connectionStartTime >= WIFI_CONNECT_TIMEOUT_MS) {
                Serial.printf("[WIFI] Timeout connessione verso %s%s\n",
                              currentAttemptSSID,
                              currentAttemptIsRescue ? " [EMERGENZA]" : "");
                WiFi.disconnect(false);
                wifiState = WIFI_STATE_DISCONNECTED;
                lastConnectAttempt = now - WIFI_RECONNECT_INTERVAL_MS;

                if (lastDisconnectTime == 0) {
                    lastDisconnectTime = now;
                }

                onWifiStateChanged(wifiState);
            }
            break;

        case WIFI_STATE_CONNECTED:
        case WIFI_STATE_SYNCED:
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("[WIFI] Connessione persa");
                wifiState = WIFI_STATE_DISCONNECTED;
                lastConnectAttempt = now - WIFI_RECONNECT_INTERVAL_MS;
                clearActiveWiFiInfo();

                if (lastDisconnectTime == 0) {
                    lastDisconnectTime = now;
                }

                onWifiStateChanged(wifiState);
            }
            break;

        default:
            break;
    }

    if (wifiState == WIFI_STATE_DISCONNECTED && lastDisconnectTime > 0) {
        if (now - lastDisconnectTime >= WIFI_FALLBACK_TO_AP_MS) {
            Serial.println("[WIFI] Fallback ad AP dopo disconnessione prolungata");
            startAPMode();
        }
    }
}

// --- Imposta stato sincronizzato ---
void setWiFiSynced(bool synced) {
    if (wifiState == WIFI_STATE_CONNECTED && synced) {
        wifiState = WIFI_STATE_SYNCED;
        Serial.println("[WIFI] Stato aggiornato: SINCRONIZZATO");
        onWifiStateChanged(wifiState);
    } else if (wifiState == WIFI_STATE_SYNCED && !synced) {
        wifiState = WIFI_STATE_CONNECTED;
        Serial.println("[WIFI] Stato aggiornato: CONNESSO (non sync)");
        onWifiStateChanged(wifiState);
    }
}

// --- Debug ---
void debugPrintWiFi() {
    Serial.println("[WIFI] === Debug WiFi ===");
    Serial.printf("Stato: %d\n", wifiState);
    Serial.printf("SSID configurato: %s\n", wifiSSID);
    Serial.printf("SSID attivo: %s\n", activeWiFiSSID);
    Serial.printf("Tentativo corrente: %s\n", currentAttemptSSID);
    Serial.printf("Hotspot emergenza: %s\n", isRescueNetworkConfigured() ? RESCUE_WIFI_SSID : "(non configurato)");
    Serial.printf("Uso rete emergenza: %s\n", isUsingRescueNetwork() ? "SI" : "NO");
    Serial.printf("Potenza TX: livello %u - %s\n", wifiTxPowerLevel, getWiFiTxPowerLabel(wifiTxPowerLevel));
    Serial.printf("WiFi.status(): %d\n", WiFi.status());
    Serial.printf("IP: %s\n", getLocalIP().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("Ultimo disconnect: %lu ms fa\n",
                  lastDisconnectTime > 0 ? millis() - lastDisconnectTime : 0);
    Serial.println("[WIFI] === Fine Debug ===");
}

// --- Ottieni nome stato ---
const char* getWiFiStateName() {
    switch (wifiState) {
        case WIFI_STATE_DISCONNECTED: return "Disconnesso";
        case WIFI_STATE_CONNECTING: return "In connessione";
        case WIFI_STATE_CONNECTED: return "Connesso";
        case WIFI_STATE_SYNCED: return "Sincronizzato";
        case WIFI_STATE_AP_MODE: return "Access Point";
        default: return "Sconosciuto";
    }
}

#endif // WIFI_MANAGER_H
