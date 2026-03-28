# Piano di sviluppo affidabilita Bell-Manager

## Obiettivo

Rendere la campanella resistente a Wi-Fi instabile e riavvii, separando il flusso real-time della campanella dalla gestione rete/web e aggiungendo persistenza periodica dell'ora.

## Stato

- [x] Analisi firmware attuale e individuazione punti critici
- [x] Separazione scheduler/relay/display su task dedicato
- [x] Protezione accesso concorrente a campanelle, impostazioni e stato
- [x] Persistenza oraria su NVS a ogni inizio minuto
- [x] Ripristino orario al boot prima del tentativo NTP
- [x] Fallback Wi-Fi su hotspot di emergenza
- [x] Aggiornamento API/stato per mostrare rete attiva e sorgente tempo
- [x] Verifica finale del firmware e note operative

## Problemi individuati

1. Lo scheduler gira ancora dentro `loop()`, insieme a Wi-Fi e NTP.
2. `syncNTP()` puo' bloccare fino a 10 secondi e durante quel periodo il controllo della campanella non viene eseguito.
3. `bells`, `settings` e `systemStatus` sono condivisi tra task web e firmware senza sincronizzazione.
4. L'ora non viene ripristinata da memoria persistente dopo un riavvio.
5. Il Wi-Fi prova una sola rete salvata prima del fallback ad AP.

## Strategia tecnica

### 1. Task dedicato campanella

- Creare un task `bellControlTask` su Core 1 con priorita' superiore al `loop()`.
- Spostare nel task dedicato:
  - scheduler campanelle
  - stop automatico relay
  - gestione pulsante
  - aggiornamento LED relay
  - aggiornamento display
  - salvataggio periodico dell'ora
- Lasciare nel `loop()`:
  - gestione Wi-Fi
  - tentativi NTP
  - LED Wi-Fi
  - diagnostica e stato seriale

### 2. Sincronizzazione dati

- Introdurre mutex FreeRTOS per proteggere:
  - array `bells`
  - `settings`
  - `systemStatus`
  - log campanelle
- Esporre helper piccoli per snapshot e scrittura atomica.

### 3. Persistenza oraria

- Salvare su NVS:
  - ultimo timestamp Unix noto
  - flag che identifica sorgente tempo ripristinata
- Eseguire il salvataggio solo al cambio minuto, idealmente allo `second == 0`, con deduplica sul minuto gia' scritto.
- Al boot:
  - leggere il timestamp persistito
  - impostare il clock di sistema con `settimeofday()`
  - marcare il tempo come disponibile ma non ancora sincronizzato NTP
  - tentare poi l'NTP appena la rete e' disponibile

### 4. Wi-Fi con rete di emergenza

- Gestire una lista di reti candidate:
  - rete configurata dall'utente via web
  - hotspot di emergenza fisso da `config.h`
- Provare in sequenza le reti candidate a ogni ciclo di connessione.
- Esportare via API quale rete e' in uso e se e' attiva la rete di emergenza.

## Note operative

- La precisione dell'ora ripristinata dopo un riavvio dipende dal tempo trascorso a dispositivo spento, perche' non c'e' RTC con batteria.
- Il fallback hotspot verra' implementato con credenziali statiche in `config.h`, da personalizzare prima del flash se non si vogliono lasciare placeholder.

## Diario avanzamento

- 2026-03-28: analisi completata, avvio implementazione.
- 2026-03-28: introdotto task dedicato `bellControlTask` per scheduler, relay, display, pulsante e persistenza oraria.
- 2026-03-28: aggiunti mutex ricorsivi per stato condiviso e storage NVS.
- 2026-03-28: implementato salvataggio del tempo corrente ogni cambio minuto e ripristino da NVS al boot.
- 2026-03-28: esteso Wi-Fi manager con fallback tra rete configurata e hotspot di emergenza statico.
- 2026-03-28: aggiornate API e UI per esporre sorgente tempo, SSID configurato e uso della rete di emergenza.
- 2026-03-28: aggiornata la home web con secondi correnti e countdown animato rosso negli ultimi 10 secondi prima della prossima campanella.
- 2026-03-28: aggiunta impostazione persistente della potenza TX WiFi nel popup Impostazioni, con applicazione immediata al manager WiFi.
- 2026-03-28: verifica eseguita tramite revisione statica e diff locale; build automatica non eseguita per assenza di `arduino-cli` in ambiente.
