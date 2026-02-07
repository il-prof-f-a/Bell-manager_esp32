#ifndef API_HANDLERS_H
#define API_HANDLERS_H

#include <WebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "bell_types.h"
#include "gpio_control.h"
#include "time_sync.h"
#include "storage.h"
#include "schedule_engine.h"
#include "wifi_manager.h"
#include "tm1621_display.h"

// ============================================
// Handler API REST v2.0
// ============================================

// Riferimenti esterni
extern WebServer server;
extern Bell bells[];
extern uint8_t bellCount;
extern Settings settings;
extern SystemStatus systemStatus;

// === Helper Functions ===

uint8_t findFreeId() {
    for (uint8_t id = 1; id <= 255; id++) {
        bool found = false;
        for (uint8_t i = 0; i < bellCount; i++) {
            if (bells[i].id == id) { found = true; break; }
        }
        if (!found) return id;
    }
    return 0;
}

int findBellById(uint8_t id) {
    for (uint8_t i = 0; i < bellCount; i++) {
        if (bells[i].id == id) return i;
    }
    return -1;
}

// Buffer statico per risposte piccole (evita frammentazione heap)
static char jsonBuffer[256];

void sendError(int code, const char* message) {
    snprintf(jsonBuffer, sizeof(jsonBuffer),
             "{\"error\":true,\"message\":\"%s\"}", message);
    server.send(code, "application/json", jsonBuffer);
    yield();
}

void sendSuccess(const char* message) {
    snprintf(jsonBuffer, sizeof(jsonBuffer),
             "{\"success\":true,\"message\":\"%s\"}", message);
    server.send(200, "application/json", jsonBuffer);
    yield();
}

// Helper per controllare memoria disponibile
bool checkHeapMemory() {
    size_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < 10000) {
        Serial.printf("[API] WARNING: Low heap memory: %d bytes\n", freeHeap);
        return false;
    }
    return true;
}

void addCorsHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleOptions() {
    addCorsHeaders();
    server.send(204);
    yield();
}

// === BELLS API ===

void handleGetBells() {
    addCorsHeaders();

    if (!checkHeapMemory()) {
        sendError(503, "Memoria insufficiente");
        return;
    }

    // Costruisci JSON manualmente per evitare frammentazione
    String response = "[";

    for (uint8_t i = 0; i < bellCount; i++) {
        if (i > 0) response += ",";
        response += "{\"id\":";
        response += bells[i].id;
        response += ",\"hour\":";
        response += bells[i].hour;
        response += ",\"minute\":";
        response += bells[i].minute;
        response += ",\"duration\":";
        response += bells[i].duration;
        response += ",\"days\":";
        response += bells[i].days;
        response += ",\"enabled\":";
        response += bells[i].enabled ? "true" : "false";
        response += ",\"type\":\"";
        response += bells[i].type;
        response += "\",\"daysStr\":\"";
        response += getDaysString(bells[i].days);
        response += "\"}";
        yield();  // Cedi al WiFi stack tra ogni bell
    }
    response += "]";

    server.send(200, "application/json", response);
    yield();
}

void handleCreateBell() {
    addCorsHeaders();

    if (bellCount >= MAX_BELLS) {
        sendError(400, "Numero massimo campanelle raggiunto");
        return;
    }

    if (!server.hasArg("plain")) {
        sendError(400, "Corpo richiesta mancante");
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) {
        sendError(400, "JSON non valido");
        return;
    }

    if (!doc.containsKey("hour") || !doc.containsKey("minute")) {
        sendError(400, "Ora e minuto obbligatori");
        return;
    }

    Bell& bell = bells[bellCount];
    initBell(bell);

    bell.id = findFreeId();
    if (bell.id == 0) {
        sendError(500, "Impossibile generare ID");
        return;
    }

    bell.hour = doc["hour"] | 0;
    bell.minute = doc["minute"] | 0;
    bell.duration = doc["duration"] | DEFAULT_BELL_DURATION;
    bell.days = doc["days"] | DAYS_WEEKDAYS;
    bell.enabled = doc["enabled"] | true;

    if (doc.containsKey("type")) {
        strncpy(bell.type, doc["type"] | "Campanella", MAX_TYPE_LENGTH - 1);
    } else {
        strcpy(bell.type, "Campanella");
    }

    bellCount++;
    saveBells(bells, bellCount);

    JsonDocument respDoc;
    respDoc["success"] = true;
    respDoc["id"] = bell.id;
    String response;
    serializeJson(respDoc, response);
    server.send(201, "application/json", response);
}

void handleUpdateBell() {
    addCorsHeaders();

    String uri = server.uri();
    int lastSlash = uri.lastIndexOf('/');
    uint8_t id = uri.substring(lastSlash + 1).toInt();
    int idx = findBellById(id);

    if (idx < 0) {
        sendError(404, "Campanella non trovata");
        return;
    }

    if (!server.hasArg("plain")) {
        sendError(400, "Corpo richiesta mancante");
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) {
        sendError(400, "JSON non valido");
        return;
    }

    Bell& bell = bells[idx];

    if (doc.containsKey("hour")) bell.hour = doc["hour"];
    if (doc.containsKey("minute")) bell.minute = doc["minute"];
    if (doc.containsKey("duration")) bell.duration = doc["duration"];
    if (doc.containsKey("days")) bell.days = doc["days"];
    if (doc.containsKey("enabled")) bell.enabled = doc["enabled"];
    if (doc.containsKey("type")) {
        strncpy(bell.type, doc["type"] | "", MAX_TYPE_LENGTH - 1);
    }

    saveBells(bells, bellCount);
    sendSuccess("Campanella aggiornata");
}

void handleDeleteBell() {
    addCorsHeaders();

    String uri = server.uri();
    int lastSlash = uri.lastIndexOf('/');
    uint8_t id = uri.substring(lastSlash + 1).toInt();
    int idx = findBellById(id);

    if (idx < 0) {
        sendError(404, "Campanella non trovata");
        return;
    }

    for (int i = idx; i < bellCount - 1; i++) {
        bells[i] = bells[i + 1];
    }
    bellCount--;

    saveBells(bells, bellCount);
    sendSuccess("Campanella eliminata");
}

// === SETTINGS API ===

void handleGetSettings() {
    addCorsHeaders();

    // Risposta piccola - usa buffer statico
    snprintf(jsonBuffer, sizeof(jsonBuffer),
             "{\"institutionName\":\"%s\",\"globalEnabled\":%s}",
             settings.institutionName,
             settings.globalEnabled ? "true" : "false");
    server.send(200, "application/json", jsonBuffer);
    yield();
}

void handleUpdateSettings() {
    addCorsHeaders();

    if (!server.hasArg("plain")) {
        sendError(400, "Corpo richiesta mancante");
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) {
        sendError(400, "JSON non valido");
        return;
    }

    if (doc.containsKey("institutionName")) {
        strncpy(settings.institutionName, doc["institutionName"] | "", MAX_NAME_LENGTH - 1);
    }
    if (doc.containsKey("globalEnabled")) {
        settings.globalEnabled = doc["globalEnabled"];
    }

    saveSettings(settings);
    sendSuccess("Impostazioni salvate");
}

// === STATUS API ===

void handleGetStatus() {
    addCorsHeaders();

    if (!checkHeapMemory()) {
        sendError(503, "Memoria insufficiente");
        return;
    }

    // Usa buffer locale più grande per status
    static char statusBuffer[512];

    snprintf(statusBuffer, sizeof(statusBuffer),
        "{"
        "\"time\":\"%s\","
        "\"date\":\"%s\","
        "\"timeSet\":%s,"
        "\"ntpSynced\":%s,"
        "\"relayOn\":%s,"
        "\"isRinging\":%s,"
        "\"ringingBellId\":%d,"
        "\"globalEnabled\":%s,"
        "\"institutionName\":\"%s\","
        "\"nextBellTime\":\"%s\","
        "\"nextBellType\":\"%s\","
        "\"bellCount\":%d,"
        "\"logCount\":%d,"
        "\"wifiState\":%d,"
        "\"wifiStateName\":\"%s\","
        "\"version\":\"%s\""
        "}",
        getTimeStringShort().c_str(),
        getDateString().c_str(),
        isTimeSet() ? "true" : "false",
        isNtpSynced() ? "true" : "false",
        systemStatus.relayOn ? "true" : "false",
        systemStatus.isRinging ? "true" : "false",
        systemStatus.ringingBellId,
        settings.globalEnabled ? "true" : "false",
        settings.institutionName,
        getNextBellTime().c_str(),
        getNextBellType().c_str(),
        bellCount,
        getLogCount(),
        (int)getWiFiState(),
        getWiFiStateName(),
        FIRMWARE_VERSION
    );

    server.send(200, "application/json", statusBuffer);
    yield();
}

// === RELAY API ===

void handleRelayOn() {
    addCorsHeaders();

    uint8_t duration = DEFAULT_BELL_DURATION;
    if (server.hasArg("plain")) {
        JsonDocument doc;
        if (!deserializeJson(doc, server.arg("plain"))) {
            duration = doc["duration"] | DEFAULT_BELL_DURATION;
        }
    }

    manualRing(duration);
    sendSuccess("Relay acceso");
}

void handleRelayOff() {
    addCorsHeaders();
    stopRinging();
    sendSuccess("Relay spento");
}

// === LOG API ===

void handleGetLog() {
    addCorsHeaders();

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    LogEntry* log = getLog();
    uint8_t count = getLogCount();

    for (int i = count - 1; i >= 0; i--) {
        JsonObject obj = arr.add<JsonObject>();
        obj["bellId"] = log[i].bellId;
        obj["hour"] = log[i].hour;
        obj["minute"] = log[i].minute;
        obj["day"] = log[i].day;
        obj["month"] = log[i].month;
        obj["year"] = log[i].year;

        char timeStr[20];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d %02d/%02d/%04d",
                 log[i].hour, log[i].minute, log[i].day, log[i].month, log[i].year);
        obj["timeStr"] = timeStr;
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleClearLog() {
    addCorsHeaders();
    clearLog();
    sendSuccess("Log cancellato");
}

// === WIFI API ===

void handleGetWifiStatus() {
    addCorsHeaders();

    // Usa buffer statico per risposta WiFi
    static char wifiBuffer[256];

    snprintf(wifiBuffer, sizeof(wifiBuffer),
        "{"
        "\"state\":%d,"
        "\"stateName\":\"%s\","
        "\"ssid\":\"%s\","
        "\"ip\":\"%s\","
        "\"rssi\":%d,"
        "\"ntpSynced\":%s,"
        "\"isAP\":%s"
        "}",
        (int)getWiFiState(),
        getWiFiStateName(),
        getWiFiSSID(),
        getLocalIP().c_str(),
        WiFi.RSSI(),
        isNtpSynced() ? "true" : "false",
        isInAPMode() ? "true" : "false"
    );

    server.send(200, "application/json", wifiBuffer);
    yield();
}

// Variabile per gestire scan asincrono
static bool scanInProgress = false;

void handleWifiScan() {
    addCorsHeaders();

    // Controlla se c'e' gia' una scansione in corso
    int n = WiFi.scanComplete();

    if (n == WIFI_SCAN_RUNNING) {
        // Scan ancora in corso
        JsonDocument doc;
        doc["scanning"] = true;
        doc["message"] = "Scansione in corso...";
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
        return;
    }

    if (n == WIFI_SCAN_FAILED || n < 0) {
        // Nessuna scansione in corso, avviane una nuova (asincrona)
        Serial.println("[API] Avvio scansione WiFi asincrona...");
        WiFi.scanNetworks(true, false);  // true = async

        JsonDocument doc;
        doc["scanning"] = true;
        doc["message"] = "Scansione avviata...";
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
        return;
    }

    // Scansione completata, restituisci risultati
    Serial.printf("[API] Scansione completata: %d reti\n", n);

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (int i = 0; i < n; i++) {
        JsonObject net = arr.add<JsonObject>();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
        net["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }

    // Pulisci risultati per prossima scansione
    WiFi.scanDelete();

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleSetWifi() {
    addCorsHeaders();

    if (!server.hasArg("plain")) {
        sendError(400, "Corpo richiesta mancante");
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) {
        sendError(400, "JSON non valido");
        return;
    }

    const char* ssid = doc["ssid"] | "";
    const char* password = doc["password"] | "";

    if (strlen(ssid) == 0) {
        sendError(400, "SSID obbligatorio");
        return;
    }

    // Salva credenziali
    saveWiFiCredentials(ssid, password);

    JsonDocument respDoc;
    respDoc["success"] = true;
    respDoc["message"] = "Credenziali salvate, riavvio in corso...";
    String response;
    serializeJson(respDoc, response);
    server.send(200, "application/json", response);

    // Riavvia dopo un breve delay
    delay(1000);
    restartDevice();
}

// === TIMEZONE API ===

void handleGetTimezone() {
    addCorsHeaders();

    snprintf(jsonBuffer, sizeof(jsonBuffer),
             "{\"gmtOffset\":%ld,\"dstOffset\":%ld}",
             getGmtOffset(), getDstOffset());
    server.send(200, "application/json", jsonBuffer);
    yield();
}

void handleSetTimezone() {
    addCorsHeaders();

    if (!server.hasArg("plain")) {
        sendError(400, "Corpo richiesta mancante");
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) {
        sendError(400, "JSON non valido");
        return;
    }

    int32_t gmtOffset = doc["gmtOffset"] | GMT_OFFSET_SEC;
    int32_t dstOffset = doc["dstOffset"] | 0;

    setTimezone(gmtOffset, dstOffset);
    saveTimezone(gmtOffset, dstOffset);

    // Forza risincronizzazione NTP
    if (isWiFiConnected()) {
        syncNTP();
    }

    sendSuccess("Timezone salvato");
}

// === DEBUG API ===

void handleDebugStatus() {
    addCorsHeaders();

    if (!checkHeapMemory()) {
        sendError(503, "Memoria insufficiente");
        return;
    }

    // Buffer per debug status
    static char debugBuffer[1024];

    // Leggi GPIO una volta sola
    bool btnPressed = digitalRead(PIN_BUTTON) == LOW;
    bool relayOn = digitalRead(PIN_RELAY) == HIGH;
    bool ledWifiOn = digitalRead(PIN_LED_WIFI) == LOW;
    bool ledRelayOn = digitalRead(PIN_LED_RELAY) == LOW;

    yield();  // Cedi prima di costruire JSON

    snprintf(debugBuffer, sizeof(debugBuffer),
        "{"
        // GPIO
        "\"button\":%s,\"relay\":%s,\"ledWifi\":%s,\"ledRelay\":%s,"
        // System
        "\"freeHeap\":%u,\"minFreeHeap\":%u,\"uptime\":%lu,"
        "\"cpuFreq\":%u,\"flashSize\":%u,"
        // WiFi
        "\"wifiState\":%d,\"wifiStateName\":\"%s\",\"wifiRSSI\":%d,\"wifiIP\":\"%s\","
        "\"wifiSSID\":\"%s\",\"wifiMAC\":\"%s\",\"wifiChannel\":%d,"
        // NTP
        "\"ntpSynced\":%s,\"timeSet\":%s,\"currentTime\":\"%s\",\"currentDate\":\"%s\","
        // Bells
        "\"bellCount\":%d,\"globalEnabled\":%s,\"isRinging\":%s,"
        // Display TM1621 v8.0 (ESPEasy P148)
        "\"dispInit\":%s,\"dispOn\":%s,\"dispRow0\":\"%s\",\"dispRow1\":\"%s\","
        "\"dispUnits\":\"%s\",\"dispWrites\":%u,\"dispCmds\":%u,"
        "\"dispBuffer\":\"%s\",\"testMode\":%s,"
        // Version
        "\"version\":\"%s\""
        "}",
        // GPIO values
        btnPressed ? "true" : "false",
        relayOn ? "true" : "false",
        ledWifiOn ? "true" : "false",
        ledRelayOn ? "true" : "false",
        // System values
        ESP.getFreeHeap(),
        ESP.getMinFreeHeap(),
        millis() / 1000,
        ESP.getCpuFreqMHz(),
        ESP.getFlashChipSize(),
        // WiFi values
        (int)getWiFiState(),
        getWiFiStateName(),
        WiFi.RSSI(),
        getLocalIP().c_str(),
        getWiFiSSID(),
        WiFi.macAddress().c_str(),
        WiFi.channel(),
        // NTP values
        isNtpSynced() ? "true" : "false",
        isTimeSet() ? "true" : "false",
        getTimeStringShort().c_str(),
        getDateString().c_str(),
        // Bells values
        bellCount,
        settings.globalEnabled ? "true" : "false",
        systemStatus.isRinging ? "true" : "false",
        // Display TM1621 v8.0 (ESPEasy P148)
        tm1621_is_initialized() ? "true" : "false",
        tm1621_is_lcd_on() ? "true" : "false",
        tm1621_get_row(0).c_str(),
        tm1621_get_row(1).c_str(),
        tm1621_get_units_str().c_str(),
        tm1621_get_write_count(),
        tm1621_get_cmd_count(),
        tm1621_get_buffer_hex().c_str(),
        tm1621_is_test_mode() ? "true" : "false",
        // Version
        FIRMWARE_VERSION
    );

    server.send(200, "application/json", debugBuffer);
    yield();
}

void handleDebugSetGpio() {
    addCorsHeaders();

    if (!server.hasArg("plain")) {
        sendError(400, "Body mancante");
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) {
        sendError(400, "JSON non valido");
        return;
    }

    const char* gpio = doc["gpio"] | "";
    bool state = doc["state"] | false;

    if (strcmp(gpio, "relay") == 0) {
        digitalWrite(PIN_RELAY, state ? HIGH : LOW);
        Serial.printf("[DEBUG] Relay -> %s\n", state ? "ON" : "OFF");
    }
    else if (strcmp(gpio, "ledWifi") == 0) {
        digitalWrite(PIN_LED_WIFI, state ? LOW : HIGH);  // active low
        Serial.printf("[DEBUG] LED WiFi -> %s\n", state ? "ON" : "OFF");
    }
    else if (strcmp(gpio, "ledRelay") == 0) {
        digitalWrite(PIN_LED_RELAY, state ? LOW : HIGH);  // active low
        Serial.printf("[DEBUG] LED Relay -> %s\n", state ? "ON" : "OFF");
    }
    else {
        sendError(400, "GPIO non valido");
        return;
    }

    sendSuccess("GPIO impostato");
}

void handleDebugTestDisplay() {
    addCorsHeaders();

    if (!server.hasArg("plain")) {
        sendError(400, "Body mancante");
        return;
    }

    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) {
        sendError(400, "JSON non valido");
        return;
    }

    const char* test = doc["test"] | "";
    JsonDocument respDoc;
    respDoc["success"] = true;

    // ============================================
    // TM1621 Driver v8.0 (ESPEasy P148)
    // Display: 2 righe x 4 digit, buffer 8 byte @ 0x10
    // ============================================

    // === POWER CONTROL ===
    if (strcmp(test, "lcd_on") == 0) {
        tm1621_lcd_on_cmd();
        respDoc["message"] = "LCD ON";
    }
    else if (strcmp(test, "lcd_off") == 0) {
        tm1621_lcd_off_cmd();
        respDoc["message"] = "LCD OFF";
    }
    else if (strcmp(test, "reinit") == 0) {
        tm1621_reinit();
        respDoc["message"] = "Display re-inizializzato (ESPEasy P148)";
    }

    // === SEGMENT TESTS ===
    else if (strcmp(test, "all_on") == 0) {
        tm1621_test_all_on();
        respDoc["message"] = "Tutti i segmenti ON (8x 0xFF @ 0x10)";
    }
    else if (strcmp(test, "all_off") == 0) {
        tm1621_test_all_off();
        respDoc["message"] = "Tutti i segmenti OFF";
    }

    // === WRITE STRING (writeString) ===
    else if (strcmp(test, "write_row") == 0) {
        int row = doc["row"] | 0;
        const char* text = doc["text"] | "----";
        bool firstrow = (row == 0);
        tm1621_write_string(firstrow, text);
        char msg[64];
        snprintf(msg, sizeof(msg), "writeString row%d: \"%s\"", row, text);
        respDoc["message"] = msg;
    }

    // === WRITE STRINGS (writeStrings) ===
    else if (strcmp(test, "write_both") == 0) {
        const char* str1 = doc["str1"] | "----";
        const char* str2 = doc["str2"] | "----";
        tm1621_write_strings(str1, str2);
        char msg[64];
        snprintf(msg, sizeof(msg), "writeStrings: \"%s\" / \"%s\"", str1, str2);
        respDoc["message"] = msg;
    }

    // === WRITE FLOAT (writeFloat) ===
    else if (strcmp(test, "write_float") == 0) {
        int row = doc["row"] | 0;
        float value = doc["value"] | 0.0f;
        bool firstrow = (row == 0);
        tm1621_write_float(firstrow, value);
        char msg[64];
        snprintf(msg, sizeof(msg), "writeFloat row%d: %.1f", row, value);
        respDoc["message"] = msg;
    }

    // === WRITE FLOATS (writeFloats) ===
    else if (strcmp(test, "write_floats") == 0) {
        float v1 = doc["value1"] | 0.0f;
        float v2 = doc["value2"] | 0.0f;
        tm1621_write_floats(v1, v2);
        char msg[64];
        snprintf(msg, sizeof(msg), "writeFloats: %.1f / %.1f", v1, v2);
        respDoc["message"] = msg;
    }

    // === WRITE RAW DATA (writeRawData) ===
    else if (strcmp(test, "write_raw") == 0) {
        const char* hex = doc["hex"] | "0000000000000000";
        uint64_t rawdata = 0;
        for (int i = 0; hex[i] && i < 16; i++) {
            rawdata <<= 4;
            char c = tolower(hex[i]);
            if (c >= '0' && c <= '9') rawdata |= (c - '0');
            else if (c >= 'a' && c <= 'f') rawdata |= (c - 'a' + 10);
        }
        tm1621_write_raw(rawdata);
        char msg[48];
        snprintf(msg, sizeof(msg), "writeRawData: 0x%s", hex);
        respDoc["message"] = msg;
    }

    // === SET UNIT (setUnit) ===
    else if (strcmp(test, "set_unit") == 0) {
        const char* unit = doc["unit"] | "none";
        char msg[64];

        if (strcmp(unit, "celsius") == 0) {
            tm1621_set_unit(TM1621_UNIT_CELSIUS, true);
            snprintf(msg, sizeof(msg), "Unit: Celsius (row 0)");
        } else if (strcmp(unit, "fahrenheit") == 0) {
            tm1621_set_unit(TM1621_UNIT_FAHRENHEIT, true);
            snprintf(msg, sizeof(msg), "Unit: Fahrenheit (row 0)");
        } else if (strcmp(unit, "humidity") == 0) {
            tm1621_set_unit(TM1621_UNIT_HUMIDITY, false);
            snprintf(msg, sizeof(msg), "Unit: Humidity (row 1)");
        } else if (strcmp(unit, "volt_amp") == 0) {
            tm1621_set_unit(TM1621_UNIT_VOLT_AMP, true);
            snprintf(msg, sizeof(msg), "Unit: V/A (row 0)");
        } else if (strcmp(unit, "volt_amp_bot") == 0) {
            tm1621_set_unit(TM1621_UNIT_VOLT_AMP, false);
            snprintf(msg, sizeof(msg), "Unit: V/A (row 1)");
        } else if (strcmp(unit, "kwh_watt") == 0) {
            tm1621_set_unit(TM1621_UNIT_KWH_WATT, true);
            snprintf(msg, sizeof(msg), "Unit: kWh/W (row 0)");
        } else if (strcmp(unit, "kwh_watt_bot") == 0) {
            tm1621_set_unit(TM1621_UNIT_KWH_WATT, false);
            snprintf(msg, sizeof(msg), "Unit: kWh/W (row 1)");
        } else if (strcmp(unit, "none_top") == 0) {
            tm1621_set_unit(TM1621_UNIT_NONE, true);
            snprintf(msg, sizeof(msg), "Unit: None (row 0)");
        } else if (strcmp(unit, "none_bot") == 0) {
            tm1621_set_unit(TM1621_UNIT_NONE, false);
            snprintf(msg, sizeof(msg), "Unit: None (row 1)");
        } else if (strcmp(unit, "clear_all") == 0) {
            tm1621_clear_all_units();
            snprintf(msg, sizeof(msg), "Cleared all unit symbols");
        } else {
            snprintf(msg, sizeof(msg), "Unit sconosciuta: %s", unit);
        }

        // Aggiorna display per mostrare i nuovi simboli
        tm1621_refresh();
        respDoc["message"] = msg;
    }

    // === PRINT STATUS TO SERIAL ===
    else if (strcmp(test, "print_status") == 0) {
        tm1621_print_status();
        respDoc["message"] = "Status stampato su Serial";
    }

    else {
        sendError(400, "Comandi: lcd_on, lcd_off, reinit, all_on, all_off, write_row, write_both, write_float, write_floats, write_raw, set_unit, print_status");
        return;
    }

    String response;
    serializeJson(respDoc, response);
    server.send(200, "application/json", response);
    Serial.printf("[DEBUG] Display test: %s\n", test);
}

void handleSetTestMode() {
    addCorsHeaders();

    if (!server.hasArg("enable")) {
        sendError(400, "Parametro 'enable' mancante (0 o 1)");
        return;
    }

    bool enable = server.arg("enable").toInt() == 1;
    tm1621_set_test_mode(enable);

    snprintf(jsonBuffer, sizeof(jsonBuffer),
             "{\"success\":true,\"message\":\"Test mode: %s\",\"testMode\":%s}",
             enable ? "ON" : "OFF", enable ? "true" : "false");
    server.send(200, "application/json", jsonBuffer);
    Serial.printf("[API] Test mode -> %s\n", enable ? "ON" : "OFF");
}

void handleDebugRestart() {
    addCorsHeaders();
    sendSuccess("Riavvio in corso...");
    delay(500);
    ESP.restart();
}

// ============================================
// STATE API - Endpoint unico per tutto lo stato
// GET /api/state restituisce:
//   system, wifi, time, bells, scheduler, io, debug
// ============================================

// Buffer statico per /api/state (evita allocazioni heap)
static char stateBuffer[4096];

void handleGetState() {
    addCorsHeaders();

    // Pre-calcola valori (evita chiamate multiple)
    struct tm timeinfo;
    bool hasTime = getLocalTime(&timeinfo);
    int hour = hasTime ? timeinfo.tm_hour : 0;
    int minute = hasTime ? timeinfo.tm_min : 0;

    int nextH = 0, nextM = 0;
    String nextTime = getNextBellTime();
    if (nextTime.length() >= 5) {
        nextH = nextTime.substring(0, 2).toInt();
        nextM = nextTime.substring(3, 5).toInt();
    }

    // Costruisci JSON con snprintf (molto piu' veloce)
    int pos = snprintf(stateBuffer, sizeof(stateBuffer),
        "{"
        "\"system\":{\"version\":\"%s\",\"name\":\"%s\",\"global\":%s},"
        "\"wifi\":{\"state\":%d,\"ssid\":\"%s\",\"ip\":\"%s\",\"rssi\":%d},"
        "\"time\":{\"h\":%d,\"m\":%d,\"date\":\"%s\",\"ntp\":%s,\"gmt\":%ld,\"dst\":%ld},"
        "\"bells\":[",
        FIRMWARE_VERSION,
        settings.institutionName,
        settings.globalEnabled ? "true" : "false",
        (int)getWiFiState(),
        getWiFiSSID(),
        getLocalIP().c_str(),
        WiFi.RSSI(),
        hour, minute,
        getDateString().c_str(),
        isNtpSynced() ? "true" : "false",
        getGmtOffset(),
        getDstOffset()
    );

    // Aggiungi bells
    for (uint8_t i = 0; i < bellCount && pos < (int)sizeof(stateBuffer) - 200; i++) {
        pos += snprintf(stateBuffer + pos, sizeof(stateBuffer) - pos,
            "%s{\"id\":%d,\"h\":%d,\"m\":%d,\"d\":%d,\"days\":%d,\"on\":%d,\"t\":\"%s\"}",
            i > 0 ? "," : "",
            bells[i].id, bells[i].hour, bells[i].minute,
            bells[i].duration, bells[i].days,
            bells[i].enabled ? 1 : 0, bells[i].type
        );
    }

    // Chiudi bells e aggiungi resto
    pos += snprintf(stateBuffer + pos, sizeof(stateBuffer) - pos,
        "],"
        "\"sched\":{\"ring\":%s,\"ringId\":%d,\"nextH\":%d,\"nextM\":%d,\"nextT\":\"%s\"},"
        "\"io\":{\"relay\":%d,\"btn\":%d,\"ledW\":%d,\"ledR\":%d},"
        "\"debug\":{\"heap\":%lu,\"up\":%lu}"
        "}",
        systemStatus.isRinging ? "true" : "false",
        systemStatus.ringingBellId,
        nextH, nextM,
        getNextBellType().c_str(),
        systemStatus.relayOn ? 1 : 0,
        digitalRead(PIN_BUTTON) == LOW ? 1 : 0,
        digitalRead(PIN_LED_WIFI) == LOW ? 1 : 0,
        digitalRead(PIN_LED_RELAY) == LOW ? 1 : 0,
        ESP.getFreeHeap(),
        millis() / 1000
    );

    server.send(200, "application/json", stateBuffer);
}

// Alias per compatibilita'
void handleGetInit() {
    handleGetState();
}

// === ROUTE SETUP ===

void setupApiRoutes() {
    // === STATE (endpoint unico per tutto lo stato) ===
    server.on("/api/state", HTTP_GET, handleGetState);
    server.on("/api/state", HTTP_OPTIONS, handleOptions);

    // Alias per compatibilita'
    server.on("/api/init", HTTP_GET, handleGetInit);
    server.on("/api/init", HTTP_OPTIONS, handleOptions);

    // CORS preflight
    server.on("/api/bells", HTTP_OPTIONS, handleOptions);
    server.on("/api/settings", HTTP_OPTIONS, handleOptions);
    server.on("/api/status", HTTP_OPTIONS, handleOptions);
    server.on("/api/relay/on", HTTP_OPTIONS, handleOptions);
    server.on("/api/relay/off", HTTP_OPTIONS, handleOptions);
    server.on("/api/log", HTTP_OPTIONS, handleOptions);
    server.on("/api/wifi", HTTP_OPTIONS, handleOptions);
    server.on("/api/wifi/status", HTTP_OPTIONS, handleOptions);
    server.on("/api/wifi/scan", HTTP_OPTIONS, handleOptions);
    server.on("/api/timezone", HTTP_OPTIONS, handleOptions);

    // Bells (CRUD)
    server.on("/api/bells", HTTP_GET, handleGetBells);
    server.on("/api/bells", HTTP_POST, handleCreateBell);

    // Settings
    server.on("/api/settings", HTTP_GET, handleGetSettings);
    server.on("/api/settings", HTTP_PUT, handleUpdateSettings);

    // Status
    server.on("/api/status", HTTP_GET, handleGetStatus);

    // Relay
    server.on("/api/relay/on", HTTP_POST, handleRelayOn);
    server.on("/api/relay/off", HTTP_POST, handleRelayOff);

    // Log
    server.on("/api/log", HTTP_GET, handleGetLog);
    server.on("/api/log", HTTP_DELETE, handleClearLog);

    // WiFi
    server.on("/api/wifi", HTTP_POST, handleSetWifi);
    server.on("/api/wifi/status", HTTP_GET, handleGetWifiStatus);
    server.on("/api/wifi/scan", HTTP_GET, handleWifiScan);

    // Timezone
    server.on("/api/timezone", HTTP_GET, handleGetTimezone);
    server.on("/api/timezone", HTTP_POST, handleSetTimezone);

    // Debug
    server.on("/api/debug/status", HTTP_GET, handleDebugStatus);
    server.on("/api/debug/status", HTTP_OPTIONS, handleOptions);
    server.on("/api/debug/gpio", HTTP_POST, handleDebugSetGpio);
    server.on("/api/debug/gpio", HTTP_OPTIONS, handleOptions);
    server.on("/api/debug/display", HTTP_POST, handleDebugTestDisplay);
    server.on("/api/debug/display", HTTP_OPTIONS, handleOptions);
    server.on("/api/debug/restart", HTTP_POST, handleDebugRestart);
    server.on("/api/debug/restart", HTTP_OPTIONS, handleOptions);
    server.on("/api/display/test_mode", HTTP_GET, handleSetTestMode);
    server.on("/api/display/test_mode", HTTP_OPTIONS, handleOptions);

    Serial.println("[API] Route registrate");
}

// === Handler per /api/bells/{id} ===
bool handleBellsWithId() {
    String uri = server.uri();

    if (!uri.startsWith("/api/bells/")) {
        return false;
    }

    addCorsHeaders();

    if (server.method() == HTTP_OPTIONS) {
        server.send(204);
        return true;
    }

    if (server.method() == HTTP_PUT) {
        handleUpdateBell();
        return true;
    }

    if (server.method() == HTTP_DELETE) {
        handleDeleteBell();
        return true;
    }

    if (server.method() == HTTP_GET) {
        int lastSlash = uri.lastIndexOf('/');
        uint8_t id = uri.substring(lastSlash + 1).toInt();
        int idx = findBellById(id);

        if (idx < 0) {
            sendError(404, "Campanella non trovata");
            return true;
        }

        JsonDocument doc;
        doc["id"] = bells[idx].id;
        doc["hour"] = bells[idx].hour;
        doc["minute"] = bells[idx].minute;
        doc["duration"] = bells[idx].duration;
        doc["days"] = bells[idx].days;
        doc["enabled"] = bells[idx].enabled;
        doc["type"] = bells[idx].type;
        doc["daysStr"] = getDaysString(bells[idx].days);

        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
        return true;
    }

    return false;
}

#endif // API_HANDLERS_H
