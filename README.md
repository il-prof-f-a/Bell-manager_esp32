# Bell-Manager ESP32 v2.0.1

Sistema di gestione campanelle per ambienti scolastici o lavorativi, basato su **Sonoff POW Elite 16A (POWR316D)** con interfaccia web integrata e sincronizzazione oraria via NTP.

---

## Indice

1. [Caratteristiche](#caratteristiche)
2. [Requisiti Hardware](#requisiti-hardware)
3. [Configurazione Arduino IDE](#configurazione-arduino-ide)
4. [Caricamento del Firmware](#caricamento-del-firmware)
5. [Primo Avvio](#primo-avvio)
6. [Interfaccia Web](#interfaccia-web)
7. [Funzioni del Pulsante](#funzioni-del-pulsante)
8. [Indicatori LED](#indicatori-led)
9. [Specifiche Tecniche](#specifiche-tecniche)
10. [Riferimenti Hardware](#riferimenti-hardware)

---

## Caratteristiche

- **Gestione campanelle programmabili** con orario, durata e giorni della settimana
- **Sincronizzazione automatica NTP** ogni 10 minuti
- **Interfaccia web responsive** accessibile da qualsiasi dispositivo
- **Modalita WiFi Station** con fallback automatico ad Access Point
- **Persistenza dati** su memoria non volatile (NVS)
- **LED di pre-avviso**: lampeggia 1 minuto prima e rapidamente 10 secondi prima della campanella
- **Controllo manuale** via pulsante fisico o interfaccia web
- **Configurazione fuso orario** e ora legale

---

## Requisiti Hardware

### Dispositivo Supportato
- **Sonoff POW Elite 16A** (modello POWR316D)

### Per il Caricamento del Firmware
- Adattatore USB-Seriale **CH340** o **CP2102** (3.3V)
- Cavi jumper per collegamento (GND, TX, RX, 3V3)
- Cacciavite per aprire il dispositivo

### Pinout per Flashing

| Pad Sonoff | Adattatore USB |
|------------|----------------|
| GND        | GND            |
| TX         | RX             |
| RX         | TX             |
| 3V3        | 3.3V           |

**IMPORTANTE**: Non alimentare il dispositivo dalla rete 220V durante il flashing!

---

## Configurazione Arduino IDE

### 1. Installare il Supporto ESP32

1. Apri Arduino IDE
2. Vai su **File → Preferenze**
3. Nel campo "URL aggiuntive per il Gestore schede" aggiungi:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
4. Vai su **Strumenti → Scheda → Gestore schede**
5. Cerca "esp32" e installa **esp32 by Espressif Systems**

### 2. Configurazione Scheda

Vai su **Strumenti** e imposta:

| Impostazione | Valore |
|--------------|--------|
| **Scheda** | ESP32 Dev Module |
| **PSRAM** | Disabled |
| **Flash Size** | 4MB (32Mb) |
| **Partition Scheme** | Default 4MB with spiffs |
| **Flash Mode** | QIO |
| **Flash Frequency** | 80MHz |
| **Upload Speed** | 115200 |
| **Core Debug Level** | None |

### 3. Installare la Libreria ArduinoJson

1. Vai su **Strumenti → Gestione librerie**
2. Cerca "ArduinoJson"
3. Installa **ArduinoJson by Benoit Blanchon** (versione 7.x o superiore)

---

## Caricamento del Firmware

### Preparazione

1. **Scollega** il Sonoff dalla rete elettrica 220V
2. **Apri** il dispositivo svitando le viti posteriori
3. **Collega** l'adattatore USB-Seriale ai pad di programmazione
4. **Tieni premuto** il pulsante sul dispositivo
5. **Collega** l'adattatore USB al PC (mentre tieni premuto il pulsante)
6. **Rilascia** il pulsante dopo 2 secondi

### Upload

1. Apri il file `Esp32BellManager.ino` in Arduino IDE
2. Seleziona la porta COM corretta in **Strumenti → Porta**
3. Clicca su **Carica** (freccia →)
4. Attendi il completamento (circa 1-2 minuti)
5. Apri il **Monitor Seriale** (115200 baud) per verificare l'avvio

### Verifica

Se tutto e corretto, vedrai nel Monitor Seriale:
```
============================================
    Bell-Manager ESP32 v2.0.0
    Sonoff POW Elite 16A
============================================

[INIT] GPIO...
[INIT] Sistema...
[INIT] NTP...
[INIT] Caricamento dati...
[INIT] WiFi Manager...
[INIT] Nessuna credenziale WiFi, avvio AP...
[WIFI] AP attivo: SSID=Bell-Manager-Setup, IP=192.168.4.1

[INIT] === SISTEMA PRONTO ===
```

---

## Primo Avvio

Al primo avvio, il dispositivo non ha credenziali WiFi salvate e si avvia in **modalita Access Point**.

### Configurazione WiFi

1. **Connetti** il tuo smartphone/PC alla rete WiFi:
   - **SSID**: `Bell-Manager-Setup`
   - **Password**: `bellmanager`

2. **Apri** il browser e vai a: `http://192.168.4.1`

3. **Clicca** sul pulsante "WiFi" nell'interfaccia

4. **Inserisci** SSID e password della tua rete WiFi

5. **Clicca** "Salva e Riavvia"

6. Il dispositivo si riavvia e si connette alla rete configurata

### Dopo la Configurazione

Una volta connesso alla tua rete WiFi:
- Il LED WiFi diventa **fisso** (sincronizzato)
- L'orario viene sincronizzato automaticamente via NTP
- Puoi accedere all'interfaccia dal nuovo indirizzo IP (visibile nel Monitor Seriale o nel router)

---

## Interfaccia Web

### Pagina Principale

L'interfaccia mostra:
- **Ora corrente** e data (sincronizzata via NTP)
- **Prossima campanella** programmata
- **Stato connessione** WiFi/NTP
- **Tabella campanelle** con tutte le programmazioni

### Gestione Campanelle

**Aggiungere una campanella:**
1. Clicca "+ Aggiungi Campanella"
2. Imposta ora, durata (in secondi), tipo/nome
3. Seleziona i giorni della settimana
4. Clicca "Salva"

**Modificare una campanella:**
1. Clicca l'icona matita sulla riga della campanella
2. Modifica i parametri desiderati
3. Clicca "Salva"

**Eliminare una campanella:**
1. Clicca l'icona cestino sulla riga
2. Conferma l'eliminazione

**Abilitare/Disabilitare:**
- Usa lo switch nella colonna "Attiva" per abilitare/disabilitare singole campanelle

### Impostazioni

Nel pannello Impostazioni puoi:
- Modificare il **nome dell'istituzione**
- **Abilitare/Disabilitare** globalmente tutte le campanelle
- Impostare il **fuso orario** (default: GMT+1 Italia)
- Attivare/disattivare l'**ora legale**
- Visualizzare il **log** delle ultime campanelle suonate

### Configurazione WiFi

Nel pannello WiFi puoi:
- Visualizzare lo **stato connessione** attuale
- Vedere **IP**, **SSID** e stato **sincronizzazione NTP**
- Modificare le **credenziali WiFi** (causa riavvio)

---

## Funzioni del Pulsante

Il pulsante fisico sul dispositivo ha tre funzioni:

| Pressione | Durata | Azione |
|-----------|--------|--------|
| **Breve** | < 3 secondi | Attiva/ferma il relay manualmente |
| **Lunga** | 3-10 secondi | Abilita/disabilita globalmente le campanelle |
| **Config** | > 10 secondi | Entra in modalita Access Point (configurazione) |

### Feedback Visivo

- **Pressione lunga**: il LED relay lampeggia 3 volte per conferma
- **Pressione config**: entrambi i LED lampeggiano 5 volte, poi si avvia l'AP

---

## Indicatori LED

### LED WiFi (blu)

| Stato | Significato |
|-------|-------------|
| **Spento** | Disconnesso, tentativo di riconnessione |
| **Lampeggio lento** | Modalita Access Point (configurazione) |
| **Lampeggio veloce** | Connesso alla WiFi, sincronizzazione NTP in corso |
| **Fisso acceso** | Connesso e sincronizzato con NTP |

### LED Relay (verde)

| Stato | Significato |
|-------|-------------|
| **Spento** | Nessuna campanella imminente |
| **Lampeggio lento** | Campanella tra **1 minuto** |
| **Lampeggio veloce** | Campanella tra **10 secondi** |
| **Fisso acceso** | Campanella **in corso** (relay attivo) |

---

## Specifiche Tecniche

### Comportamento WiFi

- **Modalita normale**: WiFi Station (si connette alla rete configurata)
- **Timeout connessione**: 30 secondi
- **Tentativi riconnessione**: ogni 10 secondi
- **Fallback ad AP**: dopo 5 minuti di disconnessione

### Sincronizzazione NTP

- **Server NTP**: pool.ntp.org, time.nist.gov, time.google.com
- **Intervallo sync**: ogni 10 minuti
- **Timeout sync**: 10 secondi
- **Fuso orario default**: GMT+1 (Italia)

### Limiti Sistema

- **Campanelle massime**: 50
- **Durata campanella**: 1-60 secondi
- **Log eventi**: ultime 20 attivazioni

### API REST

Il dispositivo espone le seguenti API:

| Endpoint | Metodo | Descrizione |
|----------|--------|-------------|
| `/api/bells` | GET/POST | Lista/crea campanelle |
| `/api/bells/{id}` | GET/PUT/DELETE | Singola campanella |
| `/api/settings` | GET/PUT | Impostazioni |
| `/api/status` | GET | Stato sistema |
| `/api/wifi` | POST | Configura WiFi |
| `/api/wifi/status` | GET | Stato WiFi |
| `/api/timezone` | GET/POST | Fuso orario |
| `/api/log` | GET/DELETE | Log campanelle |
| `/api/relay/on` | POST | Attiva relay |
| `/api/relay/off` | POST | Disattiva relay |

---

## Riferimenti Hardware

### Pinout Sonoff POW Elite 16A (POWR316D)

| GPIO | Funzione | Note |
|------|----------|------|
| GPIO0 | Pulsante | Active LOW, pin BOOT |
| GPIO5 | LED WiFi | Active LOW |
| GPIO13 | Relay 16A | Active HIGH |
| GPIO14 | TM1621 DATA | Display LCD (non usato) |
| GPIO16 | CSE7759B RX | Sensore energia (non usato) |
| GPIO18 | LED Relay | Active LOW |
| GPIO25 | TM1621 CS | Display LCD (non usato) |
| GPIO26 | TM1621 RD | Display LCD (non usato) |
| GPIO27 | TM1621 WR | Display LCD (non usato) |

### Componenti Hardware

- **Relay**: Monostabile 16A, pilotato da GPIO13 (HIGH = ON)
- **Display LCD**: Controller TM1621 (non utilizzato in questo firmware)
- **Sensore energia**: CSE7759B via UART (non utilizzato in questo firmware)
- **LED**: Entrambi active LOW (accesi con livello logico basso)

---

## Risoluzione Problemi

### Il dispositivo non si avvia

- Verifica che PSRAM sia impostato su "Disabled" nell'IDE
- Controlla i collegamenti dell'adattatore USB-Seriale
- Prova a ridurre la velocita di upload a 115200

### Non riesco a connettermi all'AP

- Assicurati di essere nel raggio del dispositivo
- Password corretta: `bellmanager`
- Prova a "dimenticare" la rete e riconnetterti

### L'ora non si sincronizza

- Verifica che la rete WiFi abbia accesso a Internet
- Controlla le impostazioni del fuso orario
- Il LED WiFi deve essere fisso (non lampeggiante)

### Le campanelle non suonano

- Verifica che le campanelle siano abilitate (switch ON)
- Verifica che l'abilitazione globale sia attiva (Impostazioni)
- Controlla che l'ora sia sincronizzata (LED WiFi fisso)
- Verifica i giorni della settimana configurati

### Come resettare il dispositivo

Tieni premuto il pulsante per **10 secondi** per entrare in modalita AP e riconfigurare il WiFi.

---

## Licenza

Questo progetto e rilasciato come open source per scopi educativi e personali.

---

## Crediti

- Hardware basato su Sonoff POW Elite 16A
- Riferimenti: [ESPHome](https://devices.esphome.io/devices/sonoff-pow-elite-16a/), [Tasmota](https://templates.blakadder.com/sonoff_POWR316D.html)
- Altre info da chatgpt