# Bell-manager_esp32
Bell manager for work or school environment managed by web interface with ESP32 based device

Prendendo spunto da:
https://bangertech.de/sonoff-pow-elite/
https://github.com/arendst/Tasmota/issues/15856

# Sonoff POW Elite 16A: Pinout e Protocollo di Comunicazione

_Sonoff POW Elite 16A con display LCD integrato (il display mostra 2 linee di 4 cifre più simboli di unità come "kWh" e "W") - immagine illustrativa._

## Display LCD (TM1621): Modello, Controller e Interfaccia

Il Sonoff POW Elite 16A è dotato di un display LCD segmentato (non grafico) a 2 linee da 4 cifre ciascuna, usato per mostrare in tempo reale i parametri elettrici (corrente, tensione, potenza ed energia consumata). Il driver del display è il **TM1621**, un controller LCD a 32×4 segmenti (supporta fino a 128 segmenti) tipicamente usato nei dispositivi Sonoff Elite[\[1\]](https://esphome.io/components/display/tm1621/#:~:text=The%20,THR316D%2C%20THR320D%2C%20POWR316D%20or%20POWR320D). Il TM1621 comunica con l'ESP32 attraverso un'interfaccia **seriale a 4 fili** dedicata: linee **CS** (Chip Select), **DATA** (dati seriali), **RD** (lettura) e **WR** (scrittura)[\[2\]](https://esphome.io/components/display/tm1621/#:~:text=The%20LCD%20have%20four%20signal%2C,for%20writing%20data%20dir). Questa interfaccia non è **I²C** né **SPI** standard, ma un protocollo proprietario sincrono: in pratica, l'ESP32 gestisce il segnale CS per selezionare il chip e invia i dati serialmente sulla linea DATA, sincronizzati tramite impulsi sulle linee RD/WR (di solito WR funge da clock per inviare comandi/dati, mentre RD viene usato se il TM1621 deve restituire dati)[\[2\]](https://esphome.io/components/display/tm1621/#:~:text=The%20LCD%20have%20four%20signal%2C,for%20writing%20data%20dir).

Il display LCD del POW Elite è un **7-segmenti multiplexer** (duty 1/4, bias 1/3) con alcuni segmenti aggiuntivi per i simboli delle unità. In particolare, sul lato destro del display sono presenti icone combinate: _V/A_ (Volt sull'alto, Ampere in basso) e _kWh/W_ (chilowattora in alto, Watt in basso), oltre ai simboli °C/°F e %RH (presenti sui modelli TH Elite)[\[3\]](https://espeasy.readthedocs.io/en/latest/Plugin/P148.html#:~:text=The%20LCD%20used%20on%20these,digit%20is%20a%20decimal%20dot)[\[4\]](https://espeasy.readthedocs.io/en/latest/Plugin/P148.html#:~:text=). Nel modello POW Elite, i simboli _V/A_ e _kWh/W_ **non sono indipendenti** ma accoppiati: ciò significa che il display può mostrare contemporaneamente solo il set V&A **oppure** il set kWh&W, commutando tra i due. Infatti, il firmware originale Sonoff alterna automaticamente ogni pochi secondi la visualizzazione tra tensione/corrente e energia/potenza sul display[\[5\]](https://notenoughtech.com/home-automation-review/elite-measurements-sonoff-pow-elite/#:~:text=LCD%20screen%20displays%20the%20most,between%20Voltage%2FCurrent%20and%20Power%20Consumption%2FPower). Il firmware Tasmota ed ESPHome permettono di controllare questa visualizzazione: ad esempio, in ESPHome si può usare display_voltage(true) per attivare i simboli V/A e display_kwh(false) per disattivare kWh/W, o viceversa[\[6\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=it). In Tasmota è disponibile il comando DspLine per configurare quali valori mostrare sul display e come alternarli[\[7\]](https://github.com/arendst/Tasmota/discussions/19745#:~:text=Do%20not%20use%20tasmota32,Use%20the%20regular%20binary%20instead).

Dal punto di vista hardware, i collegamenti fra ESP32 e TM1621 nel POW Elite 16A sono i seguenti:

- **GPIO25** dell'ESP32 collegato a **CS** del TM1621 (chip select);
- **GPIO14** collegato a **DATA** (linea dati seriale);
- **GPIO26** collegato a **RD** (lettura);
- **GPIO27** collegato a **WR** (scrittura).

Questa mappatura è confermata sia dalla documentazione Tasmota/ESPHome[\[8\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO14%20TM1621%20DA%20GPIO16%20CSE7766,TM1621%20RD%20GPIO27%20TM1621%20WR) che dallo schema di ESPEasy per i dispositivi Elite[\[9\]](https://espeasy.readthedocs.io/en/latest/Plugin/P148.html#:~:text=,devices%C2%B6). In pratica l'ESP32 gestisce il display inviando comandi e dati ai segmenti tramite queste 4 linee. Durante l'inizializzazione, il firmware configura il TM1621 (impostando il modo di multiplex, bias, accensione del driver LCD, ecc.) e poi aggiorna i registri dei segmenti per visualizzare i numeri desiderati. Ad esempio, Tasmota/ESPHome mostrano la tensione (V) sulla prima riga e la corrente (A) sulla seconda riga quando i simboli V/A sono attivi, oppure l'energia consumata (kWh) sulla prima riga e la potenza istantanea (W) sulla seconda riga quando sono attivi i simboli kWh/W[\[10\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=if%20%28id%28page%29.state%29%20)[\[6\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=it).

**Protocollo di comunicazione:** Il TM1621 utilizza un semplice protocollo seriale: quando **CS** viene portato LOW, l'ESP32 invia un pacchetto composto da un comando (o indirizzo) e dati attraverso la linea **DATA**, sincronizzandoli con i fronti di salita del segnale **WR**[\[11\]](https://espeasy.readthedocs.io/en/latest/Plugin/P148.html#:~:text=The%20TM1621%20LCD%20controller%20chip,4%20pins%20to%20allow%20communication). Ad esempio, per scrivere nei registri segmenti del display, il master (ESP32) invia un comando "Write" seguito dall'indirizzo iniziale e quindi i byte dei dati segmenti; ogni bit viene letto dal TM1621 ad ogni fronte di **WR** mentre **CS** è basso. La linea **RD** rimane tipicamente bassa durante le scritture, e viene usata solo se si devono leggere dati dal TM1621 (cosa rara nel caso del display LCD, dato che è un driver output). In sintesi, l'ESP32 "bit-bang" i dati verso il TM1621 tramite queste linee.

## Relè di Potenza (16A) e GPIO di controllo

Il Sonoff POW Elite 16A integra un relè elettromeccanico per commutare il carico fino a 16A. Nel modello 16A, il relè è **monostabile (a singola bobina normale)** e viene pilotato da un transistor collegato a un pin dell'ESP32. Il pin GPIO utilizzato è **GPIO13**, configurato come **Relay1** sia in Tasmota che in ESPHome[\[12\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO00%20Push%20Button%20,GPIO13%20Relay1%20GPIO14%20TM1621%20DA)[\[13\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=switch%3A). Impostando **GPIO13 alto** l'ESP32 alimenta la bobina del relè (tramite il transistor driver), chiudendo i contatti e alimentando il carico, mentre impostandolo basso il relè si apre. In altre parole, il relè è attivato in logica positiva (HIGH = ON).

**Nota:** Esiste anche una variante **20A (POWR320D)** del POW Elite che utilizza un relè bistabile (latching) con due avvolgimenti separati per ON e OFF. In quel caso il controllo avviene su due GPIO distinti (ad es. GPIO02 e GPIO04 per attivare/spegnere)[\[14\]](https://devices.esphome.io/devices/Sonoff-POW-Elite-20a#:~:text=Sonoff%20POW%20Elite%2020a%20,Wifi_LED%20%3B%20GPIO14%2C%20TM1621%20DA). Tuttavia, nel modello 16A tradizionale qui discusso, il relè è _normale_ (non latching) e richiede un solo GPIO di controllo[\[12\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO00%20Push%20Button%20,GPIO13%20Relay1%20GPIO14%20TM1621%20DA).

Il relè sul POW Elite 16A è generalmente un modello adatto a montaggio PCB con contatti 16A; secondo fonti community, potrebbe trattarsi di un FANHAR serie W15L (potenzialmente latching secondo alcuni, ma nel nostro caso usato come monostabile)[\[15\]](https://github.com/arendst/Tasmota/issues/15856#:~:text=Support%20for%20SONOFF%20Smart%20Power,to%20enable%20the%20flash). Indipendentemente dal modello, dal punto di vista di pinout è importante sapere che **GPIO13** è il pin dell'ESP32 collegato al circuito di pilotaggio del relè.

## Sensore di Energia (CSE7759B) e Collegamento

Per il monitoraggio di corrente, tensione ed energia, il Sonoff POW Elite utilizza un chip dedicato: tipicamente il **CSE7759B** (lo stesso impiegato nel Sonoff POW R2)[\[16\]](https://github.com/arendst/Tasmota/issues/15856#:~:text=It%C2%B4s%20based%20on%20an%20ESP32,what%C2%B4s%20about%20the%20LCD%20driver). Questo IC misura tensione di rete (attraverso un divisore resistivo) e corrente (tramite uno shunt resistivo inserito sul percorso del carico) e calcola potenza ed energia consumata. La comunicazione tra il CSE7759B e l'ESP32 avviene tramite un'interfaccia seriale UART a singolo filo: il chip invia periodicamente pacchetti dati in formato seriale TTL verso l'ESP32. Sul lato ESP32, il pin collegato è **GPIO16**, configurato come ingresso RX UART[\[17\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO13%20Relay1%20GPIO14%20TM1621%20DA,off%2C%20LOW%20%3D%20on).

In pratica il CSE7759B trasmette continuamente (o su richiesta) i valori misurati codificati in frame seriali (baud tipicamente **4800 baud, parità pari** secondo la configurazione standard per il CSE77xx)[\[18\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=uart%3A). L'ESP32 ascolta questi dati sul suo RX e, tramite il firmware, li decodifica per ottenere tensione (V), corrente (A), potenza (W) ed energia (Wh/kWh). **Non è presente una linea TX dall'ESP32 verso il sensore** nei collegamenti noti, poiché il protocollo CSE7759B/CSE7766 è unidirezionale (il microcontrollore riceve soltanto). Dunque, dal punto di vista del pinout, l'ESP32 usa un solo pin (GPIO16) per ricevere i dati dal sensore di energia, mentre il CSE7759B è alimentato dal lato AC e isolato opportunamente nel circuito di misura.

Nel firmware Tasmota/ESPHome questo sensore è gestito tramite il driver **CSE7766** (compatibile con CSE7759B) configurato sul UART RX di GPIO16[\[17\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO13%20Relay1%20GPIO14%20TM1621%20DA,off%2C%20LOW%20%3D%20on). Ciò conferma che, per scopi pratici, CSE7759B e CSE7766 sono simili dal lato comunicazione seriale e vengono intercambiati nei template di configurazione.

**Pin principali CSE7759B:** oltre al pin di uscita dati collegato a GPIO16, il chip presenta pin collegati al circuito di misura (ingresso di tensione da rete attraverso divisore resistivo, ingresso di corrente dallo shunt, alimentazione Vcc e GND, ecc.). Tali pin però non sono collegati direttamente all'ESP32 ma al circuito elettrico di potenza sul PCB, dunque non fanno parte del pinout logico verso il microcontrollore. Dal punto di vista dell'ESP32 conta principalmente il collegamento del **pin DATA_OUT del CSE7759B → GPIO16 (ESP32 RX)**.

## Pulsante e LED di Stato: GPIO e Gestione

Il Sonoff POW Elite dispone di **un unico pulsante** fisico sul frontale e di **due LED di stato** interni visibili attraverso il case. Vediamo nel dettaglio:

- **Pulsante:** collegato all'ESP32 sul pin **GPIO0**[\[19\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=Pin%20Function%20GPIO00%20Push%20Button,GPIO13%20Relay1%20GPIO14%20TM1621%20DA). Questo è il _tasto unico_ dell'unità, usato per accendere/spegnere manualmente il relè (con pressione breve) e per altre funzioni secondarie (ad esempio modalità accoppiamento Bluetooth e reset, tipicamente con pressioni lunghe). Il GPIO0 dell'ESP32 è configurato come input con pull-up interno; di conseguenza, **a riposo il livello è HIGH** e quando l'utente preme il pulsante, il pin viene portato a massa (LOW)[\[19\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=Pin%20Function%20GPIO00%20Push%20Button,GPIO13%20Relay1%20GPIO14%20TM1621%20DA). Infatti, la scheda indica _HIGH = off, LOW = on_ riferito al pulsante[\[19\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=Pin%20Function%20GPIO00%20Push%20Button,GPIO13%20Relay1%20GPIO14%20TM1621%20DA), nel senso che la lettura è alta quando rilasciato (nessun press) e bassa quando premuto.
- **Nota importante:** La scelta di GPIO0 non è casuale - esso è il pin _BOOT_ dell'ESP32. Tenendo premuto il pulsante all'accensione, il dispositivo entra in flash mode per programmare il firmware (come riportato dagli utenti nelle procedure di flashing)[\[20\]](https://notenoughtech.com/home-automation-review/elite-measurements-sonoff-pow-elite/#:~:text=is%20wired%20to%20GPIO00%20to,enable%20the%20flash%20mode). Dunque il pulsante funge anche da switch per l'upload del firmware custom (hold GPIO0 low on reset). Durante il funzionamento normale, Tasmota/ESPHome intercettano le pressioni su GPIO0 per controllare il relè o cambiare la pagina del display: ad esempio, **click breve** sul pulsante attiva/disattiva il **Relay1**, mentre **press lunga** può comandare la commutazione delle informazioni mostrate sul display (cambiando da V/A a kWh/W)[\[21\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=on_click%3A)[\[22\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=,press%20to%20cycle%20display%20info).
- **LED WiFi (LED "link"):** uno dei due LED sulla scheda è collegato a **GPIO5** dell'ESP32[\[19\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=Pin%20Function%20GPIO00%20Push%20Button,GPIO13%20Relay1%20GPIO14%20TM1621%20DA). In Tasmota questo è etichettato come **LedLink** (indicatore di stato WiFi) ed è configurato come **attivo basso** ("LedLinki" nel template)[\[23\]](https://templates.blakadder.com/sonoff_POWR316D.html#:~:text=GPIO%20,None%20GPIO10%20%20None). Significa che il LED è collegato probabilmente al +3.3V tramite resistenza e il catodo al GPIO5: quando GPIO5 viene impostato LOW, il LED si accende (corrente scorre a terra), mentre con GPIO5 HIGH il LED è spento. Il firmware originale Sonoff lo utilizza per indicare lo stato della connessione (ad esempio lampeggio in fase di pairing, fisso quando connesso). Anche ESPHome/Tasmota configurano questo LED come **WiFi status**, accendendolo solo quando il dispositivo è connesso al WiFi[\[24\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=).
- **LED di stato relè:** il secondo LED, collegato a **GPIO18**[\[25\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO13%20Relay1%20GPIO14%20TM1621%20DA,GPIO25%20TM1621%20CS), viene utilizzato per indicare lo stato di alimentazione/relè. Anch'esso è cablato in modo **attivo LOW** (nel template Tasmota è "Led_i 1", dove "i" indica inverted)[\[26\]](https://templates.blakadder.com/sonoff_POWR316D.html#:~:text=GPIO17%20%20None%20GPIO18%20,None%20GPIO25%20%20TM1621%20CS). In pratica, quando il relè è attivato l'ESP32 spegne logicamente GPIO18 (stato LOW) per accendere il LED, mentre quando il relè è off mantiene GPIO18 HIGH (LED spento). ESPHome ad esempio definisce GPIO18 come **Status LED**, impostandolo acceso per indicare relay ON[\[27\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=restore_mode%3A%20RESTORE_DEFAULT_OFF)[\[28\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=light%3A). Nel firmware originale Sonoff, questo LED potrebbe servire da generica spia di alimentazione (dato che il display non ha retroilluminazione, un LED può segnalare il dispositivo acceso). Tuttavia, recensioni notano che _i LED interni sono molto deboli_, difficili da vedere in piena luce[\[29\]](https://notenoughtech.com/home-automation-review/elite-measurements-sonoff-pow-elite/#:~:text=status%20LEDs%20are%20very%20dim%2C,output%20without%20opening%20the%20app), per cui la loro funzione è secondaria.

In sintesi, **GPIO5** (LED blu WiFi) lampeggia o si illumina per lo stato di rete, e **GPIO18** (LED rosso/verde, a seconda del colore scelto da Sonoff, spesso verde) indica lo stato del relè. Entrambi i LED sono **collegati in logica inversa (attivi bassi)**, dettaglio da gestire nel software accendendoli con LOW.

**Schema riassuntivo dei collegamenti ESP32 ⇔ Componenti:**

- GPIO0 → Pulsante utente (pull-up interno, attivo LOW)[\[19\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=Pin%20Function%20GPIO00%20Push%20Button,GPIO13%20Relay1%20GPIO14%20TM1621%20DA)
- GPIO5 → LED WiFi **(attivo LOW)**[\[19\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=Pin%20Function%20GPIO00%20Push%20Button,GPIO13%20Relay1%20GPIO14%20TM1621%20DA)
- GPIO13 → Relè 16A (pilotaggio tramite transistor, attivo HIGH)[\[12\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO00%20Push%20Button%20,GPIO13%20Relay1%20GPIO14%20TM1621%20DA)
- GPIO14 → TM1621 **DATA** (bus seriale display)[\[30\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO14%20TM1621%20DA%20GPIO16%20CSE7766,GPIO25%20TM1621%20CS)
- GPIO16 → Ingresso UART dal sensore di energia CSE7759B (RX @ 4800bps)[\[17\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO13%20Relay1%20GPIO14%20TM1621%20DA,off%2C%20LOW%20%3D%20on)
- GPIO18 → LED di stato relè **(attivo LOW)**[\[25\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO13%20Relay1%20GPIO14%20TM1621%20DA,GPIO25%20TM1621%20CS)
- GPIO25 → TM1621 **CS** (chip select display)[\[31\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO16%20CSE7766%20Rx%20GPIO18%20Status,TM1621%20RD%20GPIO27%20TM1621%20WR)
- GPIO26 → TM1621 **RD** (lettura display)[\[31\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO16%20CSE7766%20Rx%20GPIO18%20Status,TM1621%20RD%20GPIO27%20TM1621%20WR)
- GPIO27 → TM1621 **WR** (scrittura display)[\[31\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO16%20CSE7766%20Rx%20GPIO18%20Status,TM1621%20RD%20GPIO27%20TM1621%20WR)

_(Altri GPIO dell'ESP32 non vengono utilizzati in questo modello, come confermato dal template Tasmota_[_\[32\]_](https://templates.blakadder.com/sonoff_POWR316D.html#:~:text=GPIO%20,None%20GPIO12%20%20None)[_\[33\]_](https://templates.blakadder.com/sonoff_POWR316D.html#:~:text=GPIO13%20%20Relay%201%20GPIO14,None%20GPIO21%20%20None)_. Il modulo ESP32 montato ha anche pin UART0 per programmazione: TX0, RX0 su GPIO1 e GPIO3 esposti su pad per flashing, insieme a 3V3, GND e GPIO0)._

## Funzionamento del Display e Supporto Firmware (cenni)

Il display LCD segmentato del POW Elite è progettato per mostrare i principali dati elettrici localmente. In modalità standard, il firmware originale Sonoff eWeLink fa ruotare automaticamente le due schermate: **(V, A)** e **(kWh, W)** ogni pochi secondi[\[5\]](https://notenoughtech.com/home-automation-review/elite-measurements-sonoff-pow-elite/#:~:text=LCD%20screen%20displays%20the%20most,between%20Voltage%2FCurrent%20and%20Power%20Consumption%2FPower), così l'utente può leggere sia i valori istantanei di tensione/corrente, sia l'energia consumata e la potenza istantanea, sul piccolo schermo. L'**unico pulsante** presente, oltre a controllare il relè, permette anche di **ciclare manualmente le schermate** o resettare i dati: ad esempio una pressione lunga potrebbe fissare la visualizzazione su una certa pagina o effettuare il reset dei consumi (dipende dalla programmazione firmware). Come riportato, _"l'unico pulsante funge anche da pulsante di reset, pairing e toggle del relè"_[\[29\]](https://notenoughtech.com/home-automation-review/elite-measurements-sonoff-pow-elite/#:~:text=status%20LEDs%20are%20very%20dim%2C,output%20without%20opening%20the%20app), indicando la multifunzione legata alla durata della pressione.

Con firmware alternativi come **Tasmota**, il Sonoff POW Elite 16A è pienamente supportato (da Tasmota versione 12.x in poi, su base ESP32). Tasmota include un driver specifico per il controller TM1621 in combinazione con questo display Sonoff[\[34\]](https://github.com/arendst/Tasmota/discussions/19745#:~:text=Answered%20by%20%20sfromis%20,65). Ciò consente di personalizzare le cifre visualizzate: di default Tasmota mostra sul display gli stessi parametri del firmware originale (commutando V/A e kWh/W). Tramite comandi come DspLine è possibile selezionare quali sensori o informazioni mostrare sulle due righe del display[\[7\]](https://github.com/arendst/Tasmota/discussions/19745#:~:text=Do%20not%20use%20tasmota32,Use%20the%20regular%20binary%20instead). Ad esempio, l'utente può scegliere di fissare la visualizzazione di potenza e corrente, o inserire la temperatura interna ESP32 se volesse - nei limiti dei 4 digit. Anche **ESPHome** offre una componente display: tm1621 che permette, via codice (lambda C++), di scrivere numeri o testo formattato sulle due righe[\[35\]](https://esphome.io/components/display/tm1621/#:~:text=The%20most%20basic%20operation%20with,that%20the%20TM1621%20can%20understand)[\[36\]](https://esphome.io/components/display/tm1621/#:~:text=,display_kwh%28bool). Entrambi i firmware sfruttano funzioni che accendono o spengono le icone di unità: ad esempio display_voltage(true) accende i simboli "V" (sulla prima riga) e "A" (sulla seconda) sullo schermo[\[37\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=it), mentre display_kwh(true) attiva le icone "kWh" (prima riga) e "W" (seconda riga)[\[6\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=it). Queste API semplificano la gestione del particolare layout del display (dove alcuni simboli sono combinati) senza dover gestire manualmente ogni segmento.

## Esempio di Codice Arduino (ESP32)

Di seguito forniremo un esempio di codice per ambiente Arduino (utilizzabile su un ESP32) che dimostra come:

- scrivere un testo o numero personalizzato sul display LCD TM1621;
- attivare/disattivare il relè di potenza;
- leggere lo stato del pulsante;
- gestire i LED di stato;

Il tutto rispettando il hardware del Sonoff POW Elite 16A. L'esempio è semplificato per scopi dimostrativi - un codice completo dovrebbe includere anche il parsing dei dati del sensore di energia via UART, calibrazione, debouncing del pulsante, ecc.

// Definizione dei pin (ESP32 POW Elite 16A)  
# define PIN_BUTTON 0 // Pulsante (GPIO0)  
# define PIN_LED_WIFI 5 // LED WiFi (GPIO5, attivo basso)  
# define PIN_RELAY 13 // Relè (GPIO13, attivo alto)  
# define PIN_LED_RELAY 18 // LED Stato Relè (GPIO18, attivo basso)  
# define PIN_LCD_CS 25 // TM1621 CS  
# define PIN_LCD_DATA 14 // TM1621 Data  
# define PIN_LCD_RD 26 // TM1621 Read (not used for write-only ops)  
# define PIN_LCD_WR 27 // TM1621 Write (acts as clock)  
<br/>void setup() {  
// Configura pin I/O  
pinMode(PIN_BUTTON, INPUT_PULLUP); // pulsante (pull-up interno, premuto = LOW)  
pinMode(PIN_LED_WIFI, OUTPUT);  
pinMode(PIN_LED_RELAY, OUTPUT);  
pinMode(PIN_RELAY, OUTPUT);  
pinMode(PIN_LCD_CS, OUTPUT);  
pinMode(PIN_LCD_DATA, OUTPUT);  
pinMode(PIN_LCD_RD, OUTPUT);  
pinMode(PIN_LCD_WR, OUTPUT);  
<br/>// Imposta stati iniziali  
digitalWrite(PIN_RELAY, LOW); // Relè inizialmente off (LOW = coil off)  
digitalWrite(PIN_LED_WIFI, HIGH); // LED WiFi spento (alto perché è attivo basso)  
digitalWrite(PIN_LED_RELAY, HIGH); // LED relè spento (anche questo attivo LOW)  
digitalWrite(PIN_LCD_CS, HIGH); // CS inattivo (alto)  
digitalWrite(PIN_LCD_RD, LOW); // RD tenuto basso (non si legge dal TM1621 in questo esempio)  
digitalWrite(PIN_LCD_WR, HIGH); // WR inattivo (idle alto)  
<br/>// Inizializza il display TM1621  
initTM1621();  
clearDisplay();  
<br/>// Scrivi un messaggio di esempio sul display  
displayValue(123.4, true); // ad esempio mostra "123.4" V sulla prima riga e abilita simbolo "V"  
}  
<br/>void loop() {  
// Esempio: gestione pulsante per toggling relè manuale  
if (digitalRead(PIN_BUTTON) == LOW) { // pulsante premuto  
delay(20); // anti-rimbalzo semplice  
if (digitalRead(PIN_BUTTON) == LOW) {  
toggleRelay();  
// Indica lo stato tramite LED relè  
if (digitalRead(PIN_RELAY) == HIGH) {  
// appena acceso relè  
digitalWrite(PIN_LED_RELAY, LOW); // accendi LED relè (attivo basso)  
} else {  
// appena spento relè  
digitalWrite(PIN_LED_RELAY, HIGH); // spegni LED relè  
}  
// Attendi rilascio per evitare ripetizioni  
while(digitalRead(PIN_BUTTON) == LOW) { delay(10); }  
}  
}  
<br/>// (Qui si potrebbe aggiungere gestione LED WiFi, ad es. blink se non connesso, ecc.)  
}  
<br/>// Funzione per attivare/disattivare il relè  
void toggleRelay() {  
bool relayState = digitalRead(PIN_RELAY);  
digitalWrite(PIN_RELAY, relayState ? LOW : HIGH); // inverte stato (HIGH=on)  
}  
<br/>// Invia un word (16 bit) al TM1621 bit-bang (MSB first)  
void sendTM1621Word(uint16_t data, uint8_t bits) {  
for(int i = bits - 1; i >= 0; --i) {  
digitalWrite(PIN_LCD_WR, LOW);  
digitalWrite(PIN_LCD_DATA, (data & (1 << i)) ? HIGH : LOW);  
// il TM1621 legge il bit sul rising edge di WR  
digitalWrite(PIN_LCD_WR, HIGH);  
}  
}  
<br/>// Invio comando al TM1621 (comandi sono 9 bit: 3 bit di ID + 6 bit di comando)  
void sendTM1621Command(uint8_t cmd) {  
digitalWrite(PIN_LCD_CS, LOW);  
// ID per comando è 100b (come da datasheet HT1621)  
uint16_t commandWord = 0x80 | (cmd & 0x3F); // formattazione: 100 + 6-bit comando  
sendTM1621Word(commandWord, 9);  
digitalWrite(PIN_LCD_CS, HIGH);  
}  
<br/>// Scrive 4 byte a partire da un certo indirizzo di memoria display del TM1621  
void sendTM1621Data(uint8_t addr, const uint8_t \*data, uint8_t length) {  
digitalWrite(PIN_LCD_CS, LOW);  
// ID per scrittura dati è 101b, seguito da 6 bit di indirizzo  
uint16_t startBits = 0xA0 | (addr & 0x3F); // 101 + 6-bit indirizzo  
sendTM1621Word(startBits, 9);  
// Invia i byte di data (8 bit ciascuno)  
for (uint8_t i = 0; i < length; ++i) {  
sendTM1621Word(data\[i\], 8);  
}  
digitalWrite(PIN_LCD_CS, HIGH);  
}  
<br/>// Inizializzazione TM1621: bias, comuni, attiva LCD  
void initTM1621() {  
// TM1621 datasheet commands (6-bit):  
const uint8_t CMD_BIAS_COM = 0x52; // 1/3 bias, 4 commons (duty 1/4)  
const uint8_t CMD_RC_INT = 0x30; // usa oscillatore interno RC (256kHz default)  
const uint8_t CMD_LCD_ON = 0x06; // accende il display (LCD ON)  
const uint8_t CMD_SYS_EN = 0x02; // abilita sistema oscillatore (SYS EN)  
const uint8_t CMD_LCD_OFF = 0x04; // (per spegnere LCD se serve)  
// Esecuzione sequenza di setup  
sendTM1621Command(CMD_SYS_EN);  
sendTM1621Command(CMD_RC_INT);  
sendTM1621Command(CMD_BIAS_COM);  
sendTM1621Command(CMD_LCD_ON);  
}  
<br/>// Pulizia del display (tutte cifre spente)  
void clearDisplay() {  
// Il TM1621 ha 16 indirizzi di memoria (0x00-0x0F) per i segmenti.  
// Ogni cifra 7-seg occupa tipicamente 2 nibbles (4 bit su 2 word adiacenti) a seconda di come è mappata.  
// Per semplicità inviamo zeri su tutti i 16 indirizzi:  
uint8_t clear\[16\];  
memset(clear, 0x00, sizeof(clear));  
sendTM1621Data(0x00, clear, 16);  
}  
<br/>// Visualizza un valore numerico sulla prima riga del display (4 digit max)  
void displayValue(float value, bool showVoltage) {  
// Converte il valore in stringa con una cifra decimale, es: 123.4 -> "123.4"  
char buf\[6\];  
dtostrf(value, 4, 1, buf); // 4 caratteri di cui 1 decimale  
// Mappa i caratteri in segmenti (necessario definire una mappa per il display specifico)  
uint8_t segmentData\[8\]; // fino a 4 cifre \* 2 bytes per cifre + punti  
// (Per brevità, supponiamo una mappatura ipotetica dove ogni digit usa 7 segmenti + DP in 8 bit)  
for(int i=0; i<8; ++i) segmentData\[i\] = 0x00;  
// Esempio semplice: mappa solo cifre 0-9 e punto decimale  
for (int pos = 0, segIndex = 0; pos < 5 && segIndex < 8; ++pos) {  
char c = buf\[pos\];  
uint8_t seg = 0;  
switch(c) {  
case '0': seg = 0b0111111; break;  
case '1': seg = 0b0000110; break;  
case '2': seg = 0b1011011; break;  
case '3': seg = 0b1001111; break;  
case '4': seg = 0b1100110; break;  
case '5': seg = 0b1101101; break;  
case '6': seg = 0b1111101; break;  
case '7': seg = 0b0000111; break;  
case '8': seg = 0b1111111; break;  
case '9': seg = 0b1101111; break;  
case '.':  
// accende il punto decimale del digit precedente  
segmentData\[segIndex-1\] |= 0x80; // ipotizzando bit MSB = punto  
continue;  
case ' ': // spazio  
default: seg = 0x00; break;  
}  
// Ogni digit inviamo come due nibbles (qui semplificato: seg7bit + DP -> 8 bit)  
segmentData\[segIndex++\] = seg & 0xFF;  
}  
// Invia i dati mappati all'LCD (supponendo indirizzo 0x00 inizio prima linea)  
sendTM1621Data(0x00, segmentData, 8);  
<br/>// Attiva simboli di unità sul display a seconda di showVoltage flag  
// Nel display Sonoff Elite, l'attivazione simboli avviene impostando bit specifici di segmenti extra.  
// EspHome fornisce funzioni high-level, ma qui ipotizziamo che l'icona "V" sia collegata a un segmento noto.  
if(showVoltage) {  
// Accende simbolo "V" (ad esempio, indirizzo 0x0E bit0) e "A" (0x0F bit0) - ipotetico  
uint8_t symbolData\[2\] = {0x01, 0x01};  
sendTM1621Data(0x0E, symbolData, 2);  
} else {  
// Accende simbolo "kWh" e "W" al posto di V/A  
uint8_t symbolData\[2\] = {0x02, 0x02};  
sendTM1621Data(0x0E, symbolData, 2);  
}  
}

**Spiegazione del codice:**

- Definiamo i GPIO in base al pinout del dispositivo. I LED sono attivi bassi, quindi li inizializziamo a livello **HIGH** (spenti). Il relè è inizializzato **LOW** (off). Il pulsante è un input con **INPUT_PULLUP** (quindi normalmente HIGH, premuto = LOW).
- Nel setup(), dopo la configurazione dei pin, viene chiamata initTM1621() che invia al controller del display una sequenza di comandi di inizializzazione: SYS EN (enable oscillatore di sistema), RC256K (usa clock interno a 256kHz), impostazione bias 1/3 e duty 1/4 (BIAS 1/3 4COM), e infine LCD ON per accendere il driver LCD. Questi comandi sono basati sul datasheet HT1621. Dopo l'init, clearDisplay() pulisce tutti i segmenti.
- La funzione sendTM1621Word() realizza il **bit-banging**: abbassa **WR**, emette un bit su DATA, poi alza **WR** così il TM1621 legge quel bit sul fronte di salita (il TM1621 acquisisce i dati a fronte di WR)[\[38\]](https://espeasy.readthedocs.io/en/latest/Plugin/P148.html#:~:text=TM1621). Questo viene usato sia per inviare comandi (9 bit) sia dati (8 bit). sendTM1621Command() costruisce un _word_ di 9 bit (3 bit di header 100 per indicare comando + 6 bit di comando vero e proprio) e lo invia. sendTM1621Data() invece prepara l'header 101 (indicante operazione di scrittura dati) concatenato con un indirizzo di 6 bit, e poi invia una sequenza di byte di dati (scrivendo a partire da quell'indirizzo nei registri del display) - questa serve per aggiornare le cifre sul display.
- La funzione displayValue(float value, bool showVoltage) mostra un valore numerico sulla prima riga del display, con o senza simboli di volt/ampere. In breve, converte il numero in stringa con una cifra decimale (es. 123.4 -> "123.4"), poi _mappa_ ogni carattere su un byte di segmenti (segmentData). Per semplicità abbiamo ipotizzato una mappatura dove ogni cifra 7-segmenti occupa 1 byte e il bit MSB è usato per il punto decimale. (In realtà, sul TM1621 del Sonoff, la mappatura dei segmenti è più complessa e distribuita su 4 commons, ma per chiarezza non entriamo nel dettaglio). La funzione accende i segmenti appropriati per visualizzare le cifre e posiziona il punto decimale se presente. Infine, tramite sendTM1621Data(0x00, segmentData, 8) invia questi byte ai registri del display (partendo dall'indirizzo 0x00, relativo alla prima riga). Poi attiva i simboli di unità: se showVoltage == true accende l'icona "V" e "A" sul display; se false accenderebbe "kWh"/"W". Nel codice, è mostrato come inviare due byte a indirizzi ipotetici 0x0E-0x0F che controllerebbero quei simboli. (In un uso reale bisognerebbe conoscere esattamente quali bit di quale registro comandano le icone - questa informazione è ricavabile dal datasheet del display o sperimentalmente, ed ESPHome/Tasmota lo gestiscono con funzioni già pronte come display_voltage()[\[37\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=it)).
- Nella loop(), l'esempio mostra la lettura del pulsante: quando viene rilevata una pressione (GPIO0 LOW), si attende un piccolo debounce (delay(20)) e poi, se il pulsante è ancora premuto, si chiama toggleRelay(). Questa funzione semplicemente legge lo stato attuale del relè (GPIO13) e lo inverte: se era spento (LOW) lo porta HIGH accendendolo, e viceversa. Subito dopo, il codice aggiorna il LED associato al relè (GPIO18) impostandolo LOW se il relè è ora acceso, o HIGH se è spento, in modo da riflettere lo stato. Infine, aspetta che il pulsante venga rilasciato prima di continuare (per evitare multipli toggle dovuti a un lungo press).
- (Per brevità il codice loop non mostra la gestione del LED WiFi, ma in un caso reale si potrebbe far lampeggiare **PIN_LED_WIFI** fino a connessione stabilita, poi accenderlo fisso, ecc., ricordando che il LED è invertito).

Questo esempio di codice illustra l'accesso di basso livello all'hardware del Sonoff POW Elite 16A. In pratica, librerie o framework come ESPHome semplificano queste operazioni: ad esempio, invece di scrivere a mano la logica del TM1621, si può usare la componente tm1621 di ESPHome con metodi come print()/printf() e funzioni display_\* per i simboli[\[35\]](https://esphome.io/components/display/tm1621/#:~:text=The%20most%20basic%20operation%20with,that%20the%20TM1621%20can%20understand)[\[36\]](https://esphome.io/components/display/tm1621/#:~:text=,display_kwh%28bool). Tuttavia, il codice mostrato aiuta a comprendere la corrispondenza tra i **pin GPIO** e le **periferiche** del dispositivo e come controllarli manualmente:

- **Scrittura su LCD:** inviando i comandi corretti al TM1621 e impostando i bit dei segmenti corrispondenti ai caratteri desiderati.
- **Controllo Relè:** impostando HIGH/LOW il GPIO13.
- **Lettura Pulsante:** leggendo GPIO0 (ricordando che è invertito, LOW quando premuto).
- **Gestione LED:** accendendo o spegnendo GPIO5/GPIO18 (con logica inversa, quindi scrivendo LOW per accendere).

Con queste informazioni, un utente avanzato può sviluppare firmware personalizzati compatibili con l'hardware del Sonoff POW Elite 16A, oppure sfruttare progetti esistenti come Tasmota e ESPHome che già supportano il dispositivo con le corrette impostazioni di pin e driver[\[39\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=Pin%20Function%20GPIO00%20Push%20Button,TM1621%20RD%20GPIO27%20TM1621%20WR)[\[7\]](https://github.com/arendst/Tasmota/discussions/19745#:~:text=Do%20not%20use%20tasmota32,Use%20the%20regular%20binary%20instead).

**Fonti e Riferimenti:**

- Documentazione dispositivi Sonoff Elite (POWR316D) - pinout ESP32 e componenti[\[39\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=Pin%20Function%20GPIO00%20Push%20Button,TM1621%20RD%20GPIO27%20TM1621%20WR)
- Template Tasmota per POW Elite 16A (POWR316D)[\[32\]](https://templates.blakadder.com/sonoff_POWR316D.html#:~:text=GPIO%20,None%20GPIO12%20%20None)[\[33\]](https://templates.blakadder.com/sonoff_POWR316D.html#:~:text=GPIO13%20%20Relay%201%20GPIO14,None%20GPIO21%20%20None)
- Documentazione ESPHome - driver display TM1621 e utilizzo nel Sonoff POW/TH Elite[\[2\]](https://esphome.io/components/display/tm1621/#:~:text=The%20LCD%20have%20four%20signal%2C,for%20writing%20data%20dir)[\[36\]](https://esphome.io/components/display/tm1621/#:~:text=,display_kwh%28bool)
- Forum e teardown (NotEnoughTech) - descrizione funzionamento display e note hardware (GPIO0 per flash, ecc.)[\[5\]](https://notenoughtech.com/home-automation-review/elite-measurements-sonoff-pow-elite/#:~:text=LCD%20screen%20displays%20the%20most,between%20Voltage%2FCurrent%20and%20Power%20Consumption%2FPower)[\[20\]](https://notenoughtech.com/home-automation-review/elite-measurements-sonoff-pow-elite/#:~:text=is%20wired%20to%20GPIO00%20to,enable%20the%20flash%20mode)
- Issue Tracker Tasmota - conferma chip sensore (CSE7759B) e supporto display[\[16\]](https://github.com/arendst/Tasmota/issues/15856#:~:text=It%C2%B4s%20based%20on%20an%20ESP32,what%C2%B4s%20about%20the%20LCD%20driver)[\[7\]](https://github.com/arendst/Tasmota/discussions/19745#:~:text=Do%20not%20use%20tasmota32,Use%20the%20regular%20binary%20instead)
- ESPEasy documentation - dettagli sul layout del display (2×4 digit, simboli combinati)[\[3\]](https://espeasy.readthedocs.io/en/latest/Plugin/P148.html#:~:text=The%20LCD%20used%20on%20these,digit%20is%20a%20decimal%20dot)[\[40\]](https://espeasy.readthedocs.io/en/latest/Plugin/P148.html#:~:text=)

[\[1\]](https://esphome.io/components/display/tm1621/#:~:text=The%20,THR316D%2C%20THR320D%2C%20POWR316D%20or%20POWR320D) [\[2\]](https://esphome.io/components/display/tm1621/#:~:text=The%20LCD%20have%20four%20signal%2C,for%20writing%20data%20dir) [\[35\]](https://esphome.io/components/display/tm1621/#:~:text=The%20most%20basic%20operation%20with,that%20the%20TM1621%20can%20understand) [\[36\]](https://esphome.io/components/display/tm1621/#:~:text=,display_kwh%28bool) TM1621 LCD Display - ESPHome - Smart Home Made Simple

<https://esphome.io/components/display/tm1621/>

[\[3\]](https://espeasy.readthedocs.io/en/latest/Plugin/P148.html#:~:text=The%20LCD%20used%20on%20these,digit%20is%20a%20decimal%20dot) [\[4\]](https://espeasy.readthedocs.io/en/latest/Plugin/P148.html#:~:text=) [\[9\]](https://espeasy.readthedocs.io/en/latest/Plugin/P148.html#:~:text=,devices%C2%B6) [\[11\]](https://espeasy.readthedocs.io/en/latest/Plugin/P148.html#:~:text=The%20TM1621%20LCD%20controller%20chip,4%20pins%20to%20allow%20communication) [\[38\]](https://espeasy.readthedocs.io/en/latest/Plugin/P148.html#:~:text=TM1621) [\[40\]](https://espeasy.readthedocs.io/en/latest/Plugin/P148.html#:~:text=) Display - POWR3xxD/THR3xxD - ESP Easy 2.1-beta1 documentation

<https://espeasy.readthedocs.io/en/latest/Plugin/P148.html>

[\[5\]](https://notenoughtech.com/home-automation-review/elite-measurements-sonoff-pow-elite/#:~:text=LCD%20screen%20displays%20the%20most,between%20Voltage%2FCurrent%20and%20Power%20Consumption%2FPower) [\[20\]](https://notenoughtech.com/home-automation-review/elite-measurements-sonoff-pow-elite/#:~:text=is%20wired%20to%20GPIO00%20to,enable%20the%20flash%20mode) [\[29\]](https://notenoughtech.com/home-automation-review/elite-measurements-sonoff-pow-elite/#:~:text=status%20LEDs%20are%20very%20dim%2C,output%20without%20opening%20the%20app) Elite measurements! Sonoff POW Elite - NotEnoughTech

<https://notenoughtech.com/home-automation-review/elite-measurements-sonoff-pow-elite/>

[\[6\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=it) [\[8\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO14%20TM1621%20DA%20GPIO16%20CSE7766,TM1621%20RD%20GPIO27%20TM1621%20WR) [\[10\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=if%20%28id%28page%29.state%29%20) [\[12\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO00%20Push%20Button%20,GPIO13%20Relay1%20GPIO14%20TM1621%20DA) [\[13\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=switch%3A) [\[17\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO13%20Relay1%20GPIO14%20TM1621%20DA,off%2C%20LOW%20%3D%20on) [\[18\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=uart%3A) [\[19\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=Pin%20Function%20GPIO00%20Push%20Button,GPIO13%20Relay1%20GPIO14%20TM1621%20DA) [\[21\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=on_click%3A) [\[22\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=,press%20to%20cycle%20display%20info) [\[24\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=) [\[25\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO13%20Relay1%20GPIO14%20TM1621%20DA,GPIO25%20TM1621%20CS) [\[27\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=restore_mode%3A%20RESTORE_DEFAULT_OFF) [\[28\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=light%3A) [\[30\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO14%20TM1621%20DA%20GPIO16%20CSE7766,GPIO25%20TM1621%20CS) [\[31\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=GPIO16%20CSE7766%20Rx%20GPIO18%20Status,TM1621%20RD%20GPIO27%20TM1621%20WR) [\[37\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=it) [\[39\]](https://devices.esphome.io/devices/sonoff-pow-elite-16a/#:~:text=Pin%20Function%20GPIO00%20Push%20Button,TM1621%20RD%20GPIO27%20TM1621%20WR) Sonoff POW Elite 16a (POWR316D) | devices.esphome.io

<https://devices.esphome.io/devices/sonoff-pow-elite-16a/>

[\[7\]](https://github.com/arendst/Tasmota/discussions/19745#:~:text=Do%20not%20use%20tasmota32,Use%20the%20regular%20binary%20instead) [\[34\]](https://github.com/arendst/Tasmota/discussions/19745#:~:text=Answered%20by%20%20sfromis%20,65) Sonoff TH16 Elite - missing display driver TM1621 LCD · arendst Tasmota · Discussion #19745 · GitHub

<https://github.com/arendst/Tasmota/discussions/19745>

[\[14\]](https://devices.esphome.io/devices/Sonoff-POW-Elite-20a#:~:text=Sonoff%20POW%20Elite%2020a%20,Wifi_LED%20%3B%20GPIO14%2C%20TM1621%20DA) Sonoff POW Elite 20a (POWR320D) - devices.esphome.io

<https://devices.esphome.io/devices/Sonoff-POW-Elite-20a>

[\[15\]](https://github.com/arendst/Tasmota/issues/15856#:~:text=Support%20for%20SONOFF%20Smart%20Power,to%20enable%20the%20flash) [\[16\]](https://github.com/arendst/Tasmota/issues/15856#:~:text=It%C2%B4s%20based%20on%20an%20ESP32,what%C2%B4s%20about%20the%20LCD%20driver) Support for SONOFF Smart Power Meter Switch | POW Elite #15856

<https://github.com/arendst/Tasmota/issues/15856>

[\[23\]](https://templates.blakadder.com/sonoff_POWR316D.html#:~:text=GPIO%20,None%20GPIO10%20%20None) [\[26\]](https://templates.blakadder.com/sonoff_POWR316D.html#:~:text=GPIO17%20%20None%20GPIO18%20,None%20GPIO25%20%20TM1621%20CS) [\[32\]](https://templates.blakadder.com/sonoff_POWR316D.html#:~:text=GPIO%20,None%20GPIO12%20%20None) [\[33\]](https://templates.blakadder.com/sonoff_POWR316D.html#:~:text=GPIO13%20%20Relay%201%20GPIO14,None%20GPIO21%20%20None) Sonoff POW Elite 16A Power Monitoring Switch Module (POWR316D) Configuration for Tasmota

<https://templates.blakadder.com/sonoff_POWR316D.html>
