// ============================================
// Bell-Manager ESP32 v2.1
// Firmware per Sonoff POW Elite 16A (POWR316D)
//
// Configurazione Arduino IDE:
//   Board: ESP32 Dev Module
//   PSRAM: Disabled
//   Flash Size: 4MB (32Mb)
//   Partition Scheme: Default 4MB with spiffs
//
// Architettura Dual-Core:
//   Core 0: WiFi + Web Server (task dedicato)
//   Core 1: Loop principale (relay, display, scheduler)
//
// Funzionalita':
//   - WiFi Station con NTP
//   - Server-Sent Events per aggiornamenti real-time
//   - Fallback ad AP dopo 5 min disconnessione
//   - LED WiFi: off=disconnesso, blink lento=AP,
//               blink veloce=connesso, fisso=sincronizzato
//   - LED Relay: blink lento=1min prima, blink veloce=10sec prima,
//                fisso=suono in corso
//   - Pulsante: breve=relay, lungo=toggle globale,
//               10sec=modalita' configurazione
// ============================================

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

// Header del progetto
#include "config.h"
#include "bell_types.h"
#include "gpio_control.h"
#include "tm1621_display.h"
#include "time_sync.h"
#include "storage.h"
#include "wifi_manager.h"
#include "schedule_engine.h"
#include "api_handlers.h"
#include "sse_handler.h"
#include "web_page.h"
#include "debug_page.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "icona_BellManager.h"

// ============================================
// Variabili Globali
// ============================================

WebServer server(WEB_SERVER_PORT);

Bell bells[MAX_BELLS];
uint8_t bellCount = 0;

Settings settings;
SystemStatus systemStatus;

// Task handle per web server
TaskHandle_t webServerTaskHandle = NULL;

unsigned long lastSerialStatus = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastHeapCheck = 0;
unsigned long lastSSEUpdate = 0;
size_t lastFreeHeap = 0;

// ============================================
// Callback cambio stato WiFi
// ============================================

void onWifiStateChanged(WiFiState newState) {
  Serial.printf("[MAIN] WiFi state changed: %s\n", getWiFiStateName());

  switch (newState) {
    case WIFI_STATE_DISCONNECTED:
      // LED spento, ritenta connessione
      setWifiLedMode(0);
      break;

    case WIFI_STATE_CONNECTING:
      // LED blink veloce durante connessione
      setWifiLedMode(3);
      break;

    case WIFI_STATE_CONNECTED:
      // Connesso ma non sincronizzato: blink veloce
      setWifiLedMode(3);
      // Tenta sincronizzazione NTP
      Serial.println("[MAIN] Connesso, avvio sync NTP...");
      if (syncNTP()) {
        setWiFiSynced(true);
      }
      break;

    case WIFI_STATE_SYNCED:
      // Connesso e sincronizzato: LED fisso
      setWifiLedMode(1);
      break;

    case WIFI_STATE_AP_MODE:
      // AP mode: LED blink lento
      setWifiLedMode(2);
      break;
  }
}

// ============================================
// Handler Pagina Web
// ============================================

void handleRoot() {
  // Disattiva test mode quando si torna alla home
  if (tm1621_is_test_mode()) {
    tm1621_set_test_mode(false);
    Serial.println("[WEB] Test mode disattivato (uscita da debug)");
  }
  server.send_P(200, "text/html", WEB_PAGE);
}

void handleDebug() {
  server.send_P(200, "text/html", DEBUG_PAGE);
}

// handle per icona
void handleIcon() {
  server.send_P(
    200,
    "image/png",
    (const char*)icona_png,
    icona_png_len
  );
}


void handleNotFound() {
  if (handleBellsWithId()) {
    return;
  }
  server.send(404, "text/plain", "404 Not Found");
}
// manifest
void handleManifest() {

  String manifest = R"====(
{
  "name": "Bell-Manager",
  "short_name": "Bell",
  "start_url": "/",
  "display": "standalone",
  "background_color": "#ffffff",
  "theme_color": "#0180ff",
  "icons": [
    {
      "src": "/icona.png",
      "sizes": "192x192",
      "type": "image/png"
    },
    {
      "src": "/icona.png",
      "sizes": "512x512",
      "type": "image/png"
    }
  ]
}
)====";

  server.send(200, "application/manifest+json", manifest);
}

// service worker
void handleSW() {

String sw = R"====(
const CACHE="bell-cache-v1";

self.addEventListener("install",e=>{
 e.waitUntil(
  caches.open(CACHE).then(c=>c.addAll([
   "/",
   "/icona.png",
   "/manifest.webmanifest"
  ]))
 );
});

self.addEventListener("fetch",e=>{
 e.respondWith(
  caches.match(e.request).then(r=>r||fetch(e.request))
 );
});
)====";

server.send(200,"application/javascript",sw);
}



// ============================================
// Setup OTA (Over-The-Air update)
// ============================================

void setupOTA() {
  ArduinoOTA.setHostname("Bell-Manager");
  ArduinoOTA.setPassword("bellmanager");  // Stessa password dell'AP

  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
    Serial.printf("[OTA] Inizio aggiornamento %s...\n", type.c_str());
    // Spegni relay per sicurezza durante update
    setRelay(false);
    // Feedback visivo: LED WiFi lampeggia veloce
    setWifiLedMode(3);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] Aggiornamento completato! Riavvio...");
    setWifiLedRaw(true);
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progresso: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Errore[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth fallita");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin fallito");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connessione fallita");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Ricezione fallita");
    else if (error == OTA_END_ERROR) Serial.println("End fallito");
    setWifiLedMode(0);
  });

  ArduinoOTA.begin();
  Serial.println("[OTA] OTA attivo - hostname: Bell-Manager");
}

// ============================================
// Setup Server Web
// ============================================

void setupWebServer() {
  Serial.println("[WEB] Configurazione server...");

  // Pagine statiche
  server.on("/", HTTP_GET, handleRoot);
  server.on("/debug", HTTP_GET, handleDebug);
  server.on("/icona.png", HTTP_GET, handleIcon);
  server.on("/manifest.webmanifest", handleManifest);
  server.on("/sw.js", handleSW);

  // API routes
  setupApiRoutes();

  // Server-Sent Events per aggiornamenti real-time
  setupSSE();

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.printf("[WEB] Server avviato su porta %d\n", WEB_SERVER_PORT);
}

// ============================================
// Task Web Server (Core 0)
// ============================================

void webServerTask(void * parameter) {
  Serial.println("[WEB] Task web server avviato su Core 0");

  for (;;) {
    server.handleClient();
    ArduinoOTA.handle();

    // Invia aggiornamenti SSE (pagina principale - ogni 1s)
    updateSSEClients();

    // Invia aggiornamenti SSE debug (ogni 500ms)
    updateDebugSSEClients();

    // Piccolo delay per non saturare la CPU
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ============================================
// Carica dati da storage
// ============================================

void loadData() {
  Serial.println("[INIT] Caricamento dati...");

  initStorage();

  // Carica impostazioni
  loadSettings(settings);

  // Carica campanelle
  bellCount = loadBells(bells, MAX_BELLS);
  Serial.printf("[INIT] Caricate %d campanelle\n", bellCount);

  // Carica credenziali WiFi
  char ssid[MAX_SSID_LENGTH];
  char pass[MAX_PASS_LENGTH];
  if (loadWiFiCredentials(ssid, sizeof(ssid), pass, sizeof(pass))) {
    setWiFiCredentials(ssid, pass);
  }

  // Carica timezone
  int32_t gmtOffset, dstOffset;
  loadTimezone(&gmtOffset, &dstOffset);
  setTimezone(gmtOffset, dstOffset);
}

// ============================================
// Gestione Pulsante
// ============================================

void handleButton() {
  uint8_t event = readButton();

  if (event == 1) {
    // Pressione breve: toggle relay manuale
    if (systemStatus.isRinging) {
      stopRinging();
      Serial.println("[BUTTON] Campanella fermata");
    } else {
      manualRing(DEFAULT_BELL_DURATION);
      Serial.println("[BUTTON] Campanella manuale attivata");
    }
  }
  else if (event == 2) {
    // Pressione lunga (3s): toggle abilitazione globale
    settings.globalEnabled = !settings.globalEnabled;
    saveSettings(settings);
    Serial.printf("[BUTTON] Campanelle globali: %s\n",
                  settings.globalEnabled ? "ABILITATE" : "DISABILITATE");

    // Feedback: lampeggia LED relay
    for (int i = 0; i < 3; i++) {
      setRelayLedRaw(true);
      delay(100);
      setRelayLedRaw(false);
      delay(100);
    }
  }
  else if (event == 3) {
    // Pressione config (10s): entra in modalita' AP
    Serial.println("[BUTTON] Richiesta modalita' configurazione");

    // Feedback: lampeggia entrambi i LED
    for (int i = 0; i < 5; i++) {
      setWifiLedRaw(true);
      setRelayLedRaw(true);
      delay(100);
      setWifiLedRaw(false);
      setRelayLedRaw(false);
      delay(100);
    }

    requestAPMode();
  }
}

// ============================================
// Aggiorna stato NTP se connesso
// ============================================

void updateNTPStatus() {
  if (getWiFiState() == WIFI_STATE_CONNECTED) {
    updateNTP();

    // Controlla se NTP si e' sincronizzato
    if (isNtpSynced()) {
      setWiFiSynced(true);
    }
  }
}

// ============================================
// Monitoraggio Memoria Heap
// ============================================

void checkHeapHealth() {
  unsigned long now = millis();

  // Controlla ogni 10 secondi
  if (now - lastHeapCheck < 10000) return;
  lastHeapCheck = now;

  size_t freeHeap = ESP.getFreeHeap();
  size_t minFreeHeap = ESP.getMinFreeHeap();

  // Avvisa se memoria bassa
  if (freeHeap < 20000) {
    Serial.printf("[HEAP] WARNING: Low memory! Free: %d, Min: %d\n", freeHeap, minFreeHeap);
  }

  // Controlla se c'e' un memory leak (perdita > 1KB rispetto a prima)
  if (lastFreeHeap > 0 && freeHeap < lastFreeHeap - 1024) {
    Serial.printf("[HEAP] Memory decreased: %d -> %d (-%d bytes)\n",
                  lastFreeHeap, freeHeap, lastFreeHeap - freeHeap);
  }

  lastFreeHeap = freeHeap;
}

// ============================================
// Stampa stato periodico
// ============================================

void printSerialStatus() {
  unsigned long now = millis();

  if (now - lastSerialStatus < 30000) return;
  lastSerialStatus = now;

  Serial.println();
  Serial.println("========== STATO SISTEMA ==========");
  Serial.printf("Ora: %s %s\n", getTimeStringShort().c_str(), getDateString().c_str());
  Serial.printf("NTP Sync: %s\n", isNtpSynced() ? "SI" : "NO");
  Serial.printf("WiFi: %s (IP: %s)\n", getWiFiStateName(), getLocalIP().c_str());
  Serial.printf("Campanelle: %d/%d\n", bellCount, MAX_BELLS);
  Serial.printf("Globale: %s\n", settings.globalEnabled ? "ON" : "OFF");
  Serial.printf("Relay: %s\n", systemStatus.relayOn ? "ON" : "OFF");
  Serial.printf("Prossima: %s\n", getNextBellInfo().c_str());
  Serial.printf("Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.println("------------------------------------");

  // GPIO Status
  Serial.println("GPIO:");
  Serial.printf("  Button (GPIO%d): %s\n", PIN_BUTTON, digitalRead(PIN_BUTTON) == LOW ? "PREMUTO" : "rilasciato");
  Serial.printf("  Relay (GPIO%d): %s\n", PIN_RELAY, digitalRead(PIN_RELAY) == HIGH ? "ON" : "OFF");
  Serial.printf("  LED WiFi (GPIO%d): %s\n", PIN_LED_WIFI, digitalRead(PIN_LED_WIFI) == LOW ? "ON" : "OFF");
  Serial.printf("  LED Relay (GPIO%d): %s\n", PIN_LED_RELAY, digitalRead(PIN_LED_RELAY) == LOW ? "ON" : "OFF");
  Serial.println("------------------------------------");

  // Display Status
  printDisplayStatus();

  Serial.println("====================================");
  Serial.println();
}

// ============================================
// Setup
// ============================================

void setup() {
  
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("============================================");
  Serial.println("    Bell-Manager ESP32 v" FIRMWARE_VERSION);
  Serial.println("    Sonoff POW Elite 16A");
  Serial.println("============================================");
  Serial.println();

  // Inizializza GPIO
  Serial.println("[INIT] GPIO...");
  initGPIO();

  // Inizializza Display LCD TM1621
  Serial.println("[INIT] Display...");
  initDisplay();
  displayLoading();  // Mostra "----" durante avvio

  // Inizializza stato sistema
  Serial.println("[INIT] Sistema...");
  initSystemStatus(systemStatus);

  // Inizializza NTP
  Serial.println("[INIT] NTP...");
  initNTP();

  // Carica dati da storage
  loadData();

  // Inizializza WiFi Manager
  Serial.println("[INIT] WiFi Manager...");
  initWiFiManager();

  // Delay per stabilizzare alimentazione prima di avviare WiFi
  Serial.println("[INIT] Attesa stabilizzazione alimentazione...");
  delay(1000);

  // Avvia WiFi (AP o Station in base alle credenziali salvate)
  if (hasWiFiCredentials()) {
    Serial.println("[INIT] Avvio WiFi Station...");
    startStationMode();
  } else {
    Serial.println("[INIT] Nessuna credenziale WiFi, avvio AP...");
    startAPMode();
  }

  // Attendi inizializzazione WiFi
  delay(500);

  // Setup OTA
  Serial.println("[INIT] Setup OTA...");
  setupOTA();

  // Setup Web Server
  Serial.println("[INIT] Setup WebServer...");
  setupWebServer();

  // Crea task per web server su Core 0 (stesso core del WiFi)
  Serial.println("[INIT] Avvio task WebServer su Core 0...");
  xTaskCreatePinnedToCore(
    webServerTask,        // Funzione del task
    "WebServerTask",      // Nome del task
    8192,                 // Stack size (bytes)
    NULL,                 // Parametri
    1,                    // Priorita' (1 = bassa)
    &webServerTaskHandle, // Handle del task
    0                     // Core 0 (WiFi core)
  );

  Serial.println();
  Serial.println("[INIT] === SISTEMA PRONTO ===");
  Serial.printf("[INIT] Core 0: Web Server + WiFi\n");
  Serial.printf("[INIT] Core 1: Loop principale (relay, display, scheduler)\n");

  if (isInAPMode()) {
    Serial.printf("[INIT] Connetti a WiFi: %s\n", AP_SSID);
    Serial.printf("[INIT] Password: %s\n", AP_PASSWORD);
    Serial.printf("[INIT] Apri: http://%s\n", getLocalIP().c_str());
  } else {
    Serial.println("[INIT] Connessione a WiFi in corso...");
  }
  Serial.println();
}

// ============================================
// Controlla se ci sono campanelle programmate oggi
// ============================================

bool hasBellsToday() {
  if (!isNtpSynced() || !settings.globalEnabled) return false;

  CurrentTime t = getTime();
  // t.weekday: 0=Lun, 6=Dom

  for (uint8_t i = 0; i < bellCount; i++) {
    if (bells[i].enabled && isDayEnabled(bells[i].days, t.weekday)) {
      return true;
    }
  }
  return false;
}

// ============================================
// Aggiorna indicatori display
// °C=clock, °F=AP, V/A=allarmi, kWh/W=wifi, %RH=ring
// ============================================

void updateDisplayIndicators() {
  bool clockSync  = isNtpSynced();
  bool apMode     = (getWiFiState() == WIFI_STATE_AP_MODE);
  bool alarms     = hasBellsToday();

  // kWh/W: lampeggia se c'e' un client web connesso (SSE)
  bool clientConnected = (countSSEClients() > 0);
  bool blink = ((millis() / 500) % 2 == 0);  // Toggle ogni 500ms
  bool wifiInd = clientConnected && blink;

  // %RH: lampeggia durante il ringing
  bool ringing = systemStatus.isRinging && blink;

  tm1621_set_indicators(clockSync, apMode, alarms, wifiInd, ringing);
}

// ============================================
// Aggiorna Display LCD
// ============================================

void updateDisplayTime() {
  unsigned long now = millis();

  // Aggiorna ogni secondo
  if (now - lastDisplayUpdate < 1000) return;
  lastDisplayUpdate = now;

  // Aggiorna indicatori di stato
  updateDisplayIndicators();

  // Se campanella in corso, mostra "bELL"
  if (systemStatus.isRinging) {
    displayBell();
    return;
  }

  // Se NTP sincronizzato, mostra ora
  if (isNtpSynced()) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      displayTime(timeinfo.tm_hour, timeinfo.tm_min);
    }
  } else if (getWiFiState() == WIFI_STATE_AP_MODE) {
    // In AP mode, mostra "Conn" per invitare a configurare
    displayConnecting();
  } else {
    // Non sincronizzato, mostra trattini
    displayLoading();
  }
}

// ============================================
// Loop Principale (Core 1)
// Gestisce: relay, display, scheduler, pulsante
// Il web server gira su Core 0
// ============================================

void loop() {
  // Gestisci WiFi (riconnessione, fallback AP)
  updateWiFi();

  // Gestisci NTP
  updateNTPStatus();

  // Esegui scheduler campanelle
  runScheduler();

  // Gestisci pulsante
  handleButton();

  // Aggiorna LED
  updateLEDs();

  // Aggiorna Display LCD
  updateDisplayTime();

  // Notifica client SSE se stato cambiato
  notifySSEIfChanged();

  // Monitoraggio memoria
  checkHeapHealth();

  // Stampa stato periodico
  printSerialStatus();

  // Delay minimo per non saturare Core 1
  delay(10);
}
