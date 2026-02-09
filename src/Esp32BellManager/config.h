#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Configurazione Bell-Manager ESP32
// Hardware: Sonoff POW Elite 16A (POWR316D)
// ============================================

// --- GPIO Sonoff POW Elite 16A ---
#define PIN_BUTTON      0   // Pulsante utente (active LOW, BOOT pin)
#define PIN_LED_WIFI    5   // LED WiFi blu (active LOW)
#define PIN_RELAY       13  // Relay 16A (active HIGH)
#define PIN_LED_RELAY   18  // LED stato relay (active LOW)

// --- Display LCD TM1621 ---
#define PIN_LCD_CS      25  // TM1621 Chip Select
#define PIN_LCD_DATA    14  // TM1621 Data
#define PIN_LCD_RD      26  // TM1621 Read
#define PIN_LCD_WR      27  // TM1621 Write (clock)

// GPIO non utilizzato:
// GPIO16 - CSE7759B RX (sensore energia - non implementato)

// --- Configurazione WiFi Access Point (modalita' config) ---
#define AP_SSID         "Bell-Manager-Setup"
#define AP_PASSWORD     "bellmanager"
#define AP_CHANNEL      1

// --- Configurazione WiFi Station (default) ---
#define WIFI_CONNECT_TIMEOUT_MS     30000   // Timeout connessione WiFi (30 sec)
#define WIFI_RECONNECT_INTERVAL_MS  10000   // Intervallo tentativi riconnessione (10 sec)
#define WIFI_FALLBACK_TO_AP_MS      300000  // Fallback ad AP dopo 5 minuti disconnesso

// --- Configurazione NTP ---
#define NTP_SERVER_1    "pool.ntp.org"
#define NTP_SERVER_2    "time.nist.gov"
#define NTP_SERVER_3    "time.google.com"
#define NTP_SYNC_INTERVAL_MS    600000  // Sync ogni 10 minuti
#define NTP_SYNC_TIMEOUT_MS     10000   // Timeout sync NTP (10 sec)
#define GMT_OFFSET_SEC          3600    // Offset GMT default (Italia = +1)
#define DAYLIGHT_OFFSET_SEC     3600    // Ora legale default (+1)

// --- Server Web ---
#define WEB_SERVER_PORT 80

// --- Limiti Sistema ---
#define MAX_BELLS       50      // Numero massimo campanelle
#define MAX_TYPE_LENGTH 32      // Lunghezza massima tipo campanella
#define MAX_NAME_LENGTH 64      // Lunghezza massima nome istituzione
#define MAX_SSID_LENGTH 32      // Lunghezza massima SSID WiFi
#define MAX_PASS_LENGTH 64      // Lunghezza massima password WiFi
#define MAX_LOG_ENTRIES 20      // Numero massimo voci nel log

// --- Timing Pulsante ---
#define BUTTON_DEBOUNCE_MS      50      // Debounce pulsante (ms)
#define BUTTON_SHORT_PRESS_MS   100     // Pressione breve minima (ms)
#define BUTTON_LONG_PRESS_MS    3000    // Pressione lunga (3 sec)
#define BUTTON_CONFIG_PRESS_MS  10000   // Pressione config mode (10 sec)

// --- Timing LED ---
#define LED_BLINK_FAST_MS       100     // Blink veloce LED (ms)
#define LED_BLINK_MEDIUM_MS     300     // Blink medio LED (ms)
#define LED_BLINK_SLOW_MS       500     // Blink lento LED (ms)

// --- Timing Scheduler ---
#define SCHEDULER_CHECK_MS      1000    // Intervallo controllo scheduler (1 sec)
#define PRE_RING_WARNING_SEC    60      // Avviso 1 minuto prima (LED lampeggia lento)
#define PRE_RING_IMMINENT_SEC   10      // Avviso 10 secondi prima (LED lampeggia veloce)

// --- Durata Campanella ---
#define MIN_BELL_DURATION       1       // Durata minima (secondi)
#define MAX_BELL_DURATION       60      // Durata massima (secondi)
#define DEFAULT_BELL_DURATION   3       // Durata default (secondi)

// --- Storage ---
#define STORAGE_NAMESPACE       "bellmgr"
#define STORAGE_KEY_BELLS       "bells"
#define STORAGE_KEY_SETTINGS    "settings"
#define STORAGE_KEY_BELL_COUNT  "bellcount"
#define STORAGE_KEY_WIFI_SSID   "wifi_ssid"
#define STORAGE_KEY_WIFI_PASS   "wifi_pass"
#define STORAGE_KEY_GMT_OFFSET  "gmt_offset"
#define STORAGE_KEY_DST_OFFSET  "dst_offset"

// --- Versione Firmware ---
#define FIRMWARE_VERSION        "2.3.0"

// --- Stati WiFi ---
enum WiFiState {
    WIFI_STATE_DISCONNECTED,    // Disconnesso, tenta riconnessione
    WIFI_STATE_CONNECTING,      // In connessione
    WIFI_STATE_CONNECTED,       // Connesso ma non sincronizzato
    WIFI_STATE_SYNCED,          // Connesso e sincronizzato NTP
    WIFI_STATE_AP_MODE          // Modalita' Access Point (configurazione)
};

// --- Stati LED WiFi ---
// OFF         = Disconnesso, ritenta
// Blink slow  = AP mode (configurazione)
// Blink fast  = Connesso, non sincronizzato
// Solid ON    = Connesso e sincronizzato

// --- Stati LED Relay ---
// OFF         = Nessuna campanella imminente
// Blink slow  = Campanella tra 1 minuto
// Blink fast  = Campanella tra 10 secondi
// Solid ON    = Campanella in corso

#endif // CONFIG_H
