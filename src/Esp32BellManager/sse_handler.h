#ifndef SSE_HANDLER_H
#define SSE_HANDLER_H

#include <WebServer.h>
#include <WiFiClient.h>
#include "config.h"
#include "bell_types.h"
#include "time_sync.h"
#include "wifi_manager.h"

// ============================================
// SSE Handler v2.2 - Stato completo periodico
// Invia stato ogni secondo ai client connessi
// ============================================

extern WebServer server;
extern Settings settings;
extern SystemStatus systemStatus;
extern uint8_t bellCount;

#define MAX_SSE_CLIENTS 4

struct SSEClient {
    WiFiClient client;
    bool active;
    unsigned long lastSend;
};

static SSEClient sseClients[MAX_SSE_CLIENTS];
static bool sseInitialized = false;
static unsigned long lastSSEBroadcast = 0;

// Buffer statico per SSE (evita allocazioni)
static char sseBuf[256];

// ============================================
// Inizializzazione
// ============================================

void initSSE() {
    for (int i = 0; i < MAX_SSE_CLIENTS; i++) {
        sseClients[i].active = false;
        sseClients[i].lastSend = 0;
    }
    sseInitialized = true;
}

// ============================================
// Conta client attivi
// ============================================

int countSSEClients() {
    int count = 0;
    for (int i = 0; i < MAX_SSE_CLIENTS; i++) {
        if (sseClients[i].active && sseClients[i].client.connected()) {
            count++;
        }
    }
    return count;
}

// ============================================
// Costruisce JSON stato corrente
// ============================================

void buildSSEPayload() {
    struct tm timeinfo;
    int h = 0, m = 0;
    if (getLocalTime(&timeinfo)) {
        h = timeinfo.tm_hour;
        m = timeinfo.tm_min;
    }

    snprintf(sseBuf, sizeof(sseBuf),
        "{"
        "\"h\":%d,\"m\":%d,"
        "\"relay\":%d,\"ring\":%d,\"ringId\":%d,"
        "\"wifi\":%d,\"ntp\":%d,\"global\":%d,"
        "\"btn\":%d,\"ledW\":%d,\"ledR\":%d,"
        "\"heap\":%lu,\"up\":%lu"
        "}",
        h, m,
        systemStatus.relayOn ? 1 : 0,
        systemStatus.isRinging ? 1 : 0,
        systemStatus.ringingBellId,
        (int)getWiFiState(),
        isNtpSynced() ? 1 : 0,
        settings.globalEnabled ? 1 : 0,
        digitalRead(PIN_BUTTON) == LOW ? 1 : 0,
        digitalRead(PIN_LED_WIFI) == LOW ? 1 : 0,
        digitalRead(PIN_LED_RELAY) == LOW ? 1 : 0,
        ESP.getFreeHeap(),
        millis() / 1000
    );
}

// ============================================
// Invia a singolo client
// ============================================

void sendToClient(int idx) {
    if (!sseClients[idx].active) return;

    if (!sseClients[idx].client.connected()) {
        sseClients[idx].active = false;
        Serial.printf("[SSE] Client %d disconnesso\n", idx);
        return;
    }

    sseClients[idx].client.print("data: ");
    sseClients[idx].client.println(sseBuf);
    sseClients[idx].client.println();
    sseClients[idx].lastSend = millis();
}

// ============================================
// Broadcast a tutti i client
// ============================================

void broadcastSSE() {
    buildSSEPayload();
    for (int i = 0; i < MAX_SSE_CLIENTS; i++) {
        if (sseClients[i].active) {
            sendToClient(i);
        }
    }
}

// ============================================
// Handler connessione SSE
// ============================================

void handleSSEConnect() {
    if (!sseInitialized) initSSE();

    // Trova slot libero
    int freeSlot = -1;
    for (int i = 0; i < MAX_SSE_CLIENTS; i++) {
        if (!sseClients[i].active || !sseClients[i].client.connected()) {
            freeSlot = i;
            break;
        }
    }

    if (freeSlot < 0) {
        server.send(503, "text/plain", "Too many SSE clients");
        Serial.println("[SSE] Rifiutato: troppi client");
        return;
    }

    WiFiClient client = server.client();

    // Header SSE
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/event-stream");
    client.println("Cache-Control: no-cache");
    client.println("Connection: keep-alive");
    client.println("Access-Control-Allow-Origin: *");
    client.println();

    sseClients[freeSlot].client = client;
    sseClients[freeSlot].active = true;
    sseClients[freeSlot].lastSend = 0;

    Serial.printf("[SSE] Client %d connesso (%d totali)\n", freeSlot, countSSEClients());

    // Invia stato iniziale subito
    buildSSEPayload();
    sendToClient(freeSlot);
}

// ============================================
// Aggiorna SSE - chiamato dal task web
// ============================================

void updateSSEClients() {
    if (!sseInitialized) return;

    int active = countSSEClients();
    if (active == 0) return;

    unsigned long now = millis();

    // Broadcast ogni 1 secondo
    if (now - lastSSEBroadcast >= 1000) {
        lastSSEBroadcast = now;
        broadcastSSE();
    }

    // Pulisci client morti
    for (int i = 0; i < MAX_SSE_CLIENTS; i++) {
        if (sseClients[i].active && !sseClients[i].client.connected()) {
            sseClients[i].active = false;
            Serial.printf("[SSE] Client %d rimosso\n", i);
        }
    }
}

// ============================================
// Setup route SSE
// ============================================

void setupSSE() {
    initSSE();
    server.on("/api/events", HTTP_GET, handleSSEConnect);
    Serial.println("[SSE] Endpoint attivo su /api/events");
}

// ============================================
// Funzioni placeholder per compatibilita'
// ============================================

void updateDebugSSEClients() {
    // Usa stesso canale SSE
}

void notifySSEIfChanged() {
    // Il broadcast periodico gestisce tutto
}

void notifyBellChanged() {
    // Il client ricarica le bells se necessario
}

#endif // SSE_HANDLER_H
