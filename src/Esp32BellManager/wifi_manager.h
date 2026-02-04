#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include "config.h"
#include "storage.h"  // Per hasWiFiCredentials()

// ============================================
// Gestione WiFi Station/AP con Fallback
// ============================================

// Variabili stato WiFi
static WiFiState wifiState = WIFI_STATE_DISCONNECTED;
static unsigned long lastDisconnectTime = 0;
static unsigned long lastConnectAttempt = 0;
static unsigned long connectionStartTime = 0;
static bool apModeRequested = false;

// Credenziali WiFi (caricate da storage)
static char wifiSSID[MAX_SSID_LENGTH] = "";
static char wifiPassword[MAX_PASS_LENGTH] = "";

// Callback per notifica cambio stato (definita nel .ino)
extern void onWifiStateChanged(WiFiState newState);

// --- Inizializzazione ---
void initWiFiManager() {
    wifiState = WIFI_STATE_DISCONNECTED;
    lastDisconnectTime = 0;
    lastConnectAttempt = 0;
    connectionStartTime = 0;
    apModeRequested = false;

    // Disabilita il WiFi persistent mode per evitare conflitti
    WiFi.persistent(false);

    // Auto-reconnect disabilitato - gestiamo noi la riconnessione
    WiFi.setAutoReconnect(false);

    Serial.println("[WIFI] Manager inizializzato");
}

// --- Imposta credenziali ---
void setWiFiCredentials(const char* ssid, const char* password) {
    strncpy(wifiSSID, ssid, MAX_SSID_LENGTH - 1);
    wifiSSID[MAX_SSID_LENGTH - 1] = '\0';
    strncpy(wifiPassword, password, MAX_PASS_LENGTH - 1);
    wifiPassword[MAX_PASS_LENGTH - 1] = '\0';
    Serial.printf("[WIFI] Credenziali impostate: SSID=%s\n", wifiSSID);
}

const char* getWiFiSSID() {
    return wifiSSID;
}

// hasWiFiCredentials() e' definita in storage.h

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

    // Spegni WiFi prima
    WiFi.mode(WIFI_OFF);
    delay(200);

    // Imposta modalita' AP
    WiFi.mode(WIFI_AP);
    delay(200);

    // Riduci potenza TX
    WiFi.setTxPower(WIFI_POWER_15dBm);
    delay(100);

    Serial.println("[WIFI] Avvio softAP()...");

    // Avvia AP con parametri da config.h
    WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL);

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
    if (strlen(wifiSSID) == 0) {
        Serial.println("[WIFI] Nessun SSID configurato, avvio AP");
        startAPMode();
        return;
    }

    Serial.printf("[WIFI] Connessione a: %s\n", wifiSSID);

    // Assicurati che il WiFi sia completamente spento prima
    WiFi.mode(WIFI_OFF);
    delay(200);  // Delay aumentato per stabilita' alimentazione

    // Avvia Station mode
    WiFi.mode(WIFI_STA);
    delay(200);  // Delay aumentato

    // Riduci potenza TX per limitare consumo corrente (utile per USB)
    // WIFI_POWER_19_5dBm = max, WIFI_POWER_8_5dBm = basso
    WiFi.setTxPower(WIFI_POWER_15dBm);  // Potenza media
    delay(100);

    Serial.println("[WIFI] Avvio WiFi.begin()...");

    // Connetti alla rete
    WiFi.begin(wifiSSID, wifiPassword);

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

    // Se richiesta modalita' AP, entra immediatamente
    if (apModeRequested && wifiState != WIFI_STATE_AP_MODE) {
        startAPMode();
        return;
    }

    // Se in AP mode, non fare altro
    if (wifiState == WIFI_STATE_AP_MODE) {
        return;
    }

    // Gestione stati
    switch (wifiState) {
        case WIFI_STATE_DISCONNECTED:
            // Tenta connessione se ci sono credenziali
            if (hasWiFiCredentials()) {
                if (now - lastConnectAttempt >= WIFI_RECONNECT_INTERVAL_MS) {
                    startStationMode();
                }
            } else {
                // Nessuna credenziale, vai in AP
                startAPMode();
            }
            break;

        case WIFI_STATE_CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                // Connesso!
                wifiState = WIFI_STATE_CONNECTED;
                lastDisconnectTime = 0;
                Serial.printf("[WIFI] Connesso! IP: %s\n", WiFi.localIP().toString().c_str());
                onWifiStateChanged(wifiState);
            } else if (now - connectionStartTime >= WIFI_CONNECT_TIMEOUT_MS) {
                // Timeout connessione
                Serial.println("[WIFI] Timeout connessione");
                WiFi.disconnect(false);
                wifiState = WIFI_STATE_DISCONNECTED;
                lastConnectAttempt = now;

                // Registra inizio disconnessione se non gia' registrato
                if (lastDisconnectTime == 0) {
                    lastDisconnectTime = now;
                }

                onWifiStateChanged(wifiState);
            }
            break;

        case WIFI_STATE_CONNECTED:
        case WIFI_STATE_SYNCED:
            if (WiFi.status() != WL_CONNECTED) {
                // Persa connessione
                Serial.println("[WIFI] Connessione persa");
                wifiState = WIFI_STATE_DISCONNECTED;
                lastConnectAttempt = now;

                if (lastDisconnectTime == 0) {
                    lastDisconnectTime = now;
                }

                onWifiStateChanged(wifiState);
            }
            break;

        default:
            break;
    }

    // Fallback ad AP dopo disconnessione prolungata
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
