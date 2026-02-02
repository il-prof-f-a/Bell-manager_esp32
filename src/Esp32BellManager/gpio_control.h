#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

#include <Arduino.h>
#include "config.h"
#include "bell_types.h"

// ============================================
// Controllo GPIO - Sonoff POW Elite 16A
// ============================================

// === Variabili stato pulsante ===
static unsigned long buttonPressStart = 0;
static bool buttonWasPressed = false;
static uint8_t lastButtonEvent = 0;

// === Variabili stato LED WiFi ===
static unsigned long lastWifiLedBlink = 0;
static bool wifiLedState = false;
static uint8_t wifiLedMode = 0;  // 0=off, 1=on, 2=blink slow, 3=blink fast

// === Variabili stato LED Relay ===
static unsigned long lastRelayLedBlink = 0;
static bool relayLedState = false;
static uint8_t relayLedMode = 0;  // 0=off, 1=on, 2=blink slow, 3=blink fast

// === Inizializzazione GPIO ===
void initGPIO() {
    // Pulsante (input con pull-up interno, active LOW)
    pinMode(PIN_BUTTON, INPUT_PULLUP);

    // LED WiFi (output, active LOW - acceso = LOW)
    pinMode(PIN_LED_WIFI, OUTPUT);
    digitalWrite(PIN_LED_WIFI, HIGH);  // Spento inizialmente

    // Relay (output, active HIGH - acceso = HIGH)
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);  // Spento inizialmente

    // LED Relay (output, active LOW)
    pinMode(PIN_LED_RELAY, OUTPUT);
    digitalWrite(PIN_LED_RELAY, HIGH);  // Spento inizialmente

    Serial.println("[GPIO] Inizializzazione completata");
}

// === Controllo Relay ===
void setRelay(bool on) {
    digitalWrite(PIN_RELAY, on ? HIGH : LOW);
    Serial.printf("[GPIO] Relay: %s\n", on ? "ON" : "OFF");
}

bool getRelayState() {
    return digitalRead(PIN_RELAY) == HIGH;
}

void toggleRelay() {
    setRelay(!getRelayState());
}

// === Controllo LED WiFi (raw) ===
void setWifiLedRaw(bool on) {
    // Active LOW: LOW = acceso, HIGH = spento
    digitalWrite(PIN_LED_WIFI, on ? LOW : HIGH);
    wifiLedState = on;
}

// === Modalita' LED WiFi ===
// 0 = OFF (disconnesso, ritenta)
// 1 = ON fisso (connesso e sincronizzato)
// 2 = Blink lento (AP mode)
// 3 = Blink veloce (connesso, non sincronizzato)
void setWifiLedMode(uint8_t mode) {
    wifiLedMode = mode;
    if (mode == 0) {
        setWifiLedRaw(false);
    } else if (mode == 1) {
        setWifiLedRaw(true);
    }
}

void updateWifiLed() {
    unsigned long now = millis();

    if (wifiLedMode == 2) {
        // Blink lento (AP mode)
        if (now - lastWifiLedBlink >= LED_BLINK_SLOW_MS) {
            lastWifiLedBlink = now;
            setWifiLedRaw(!wifiLedState);
        }
    } else if (wifiLedMode == 3) {
        // Blink veloce (connesso non sync)
        if (now - lastWifiLedBlink >= LED_BLINK_FAST_MS) {
            lastWifiLedBlink = now;
            setWifiLedRaw(!wifiLedState);
        }
    }
    // Per mode 0 e 1 lo stato e' gia' impostato in setWifiLedMode
}

// === Controllo LED Relay (raw) ===
void setRelayLedRaw(bool on) {
    // Active LOW: LOW = acceso, HIGH = spento
    digitalWrite(PIN_LED_RELAY, on ? LOW : HIGH);
    relayLedState = on;
}

// === Modalita' LED Relay ===
// 0 = OFF (nessuna campanella imminente)
// 1 = ON fisso (campanella in corso)
// 2 = Blink lento (campanella tra 1 minuto)
// 3 = Blink veloce (campanella tra 10 secondi)
void setRelayLedMode(uint8_t mode) {
    relayLedMode = mode;
    if (mode == 0) {
        setRelayLedRaw(false);
    } else if (mode == 1) {
        setRelayLedRaw(true);
    }
}

void updateRelayLed() {
    unsigned long now = millis();

    if (relayLedMode == 2) {
        // Blink lento (1 minuto prima)
        if (now - lastRelayLedBlink >= LED_BLINK_SLOW_MS) {
            lastRelayLedBlink = now;
            setRelayLedRaw(!relayLedState);
        }
    } else if (relayLedMode == 3) {
        // Blink veloce (10 secondi prima)
        if (now - lastRelayLedBlink >= LED_BLINK_FAST_MS) {
            lastRelayLedBlink = now;
            setRelayLedRaw(!relayLedState);
        }
    }
    // Per mode 0 e 1 lo stato e' gia' impostato in setRelayLedMode
}

// === Aggiorna tutti i LED ===
void updateLEDs() {
    updateWifiLed();
    updateRelayLed();
}

// === Lettura Pulsante ===
// Restituisce: 0 = nessun evento
//              1 = pressione breve (< 3 sec)
//              2 = pressione lunga (3-10 sec)
//              3 = pressione config (> 10 sec)
uint8_t readButton() {
    bool isPressed = (digitalRead(PIN_BUTTON) == LOW);
    unsigned long now = millis();
    uint8_t event = 0;

    if (isPressed && !buttonWasPressed) {
        // Pulsante appena premuto
        buttonPressStart = now;
        buttonWasPressed = true;
        lastButtonEvent = 0;
    }
    else if (isPressed && buttonWasPressed) {
        // Pulsante ancora premuto
        unsigned long pressDuration = now - buttonPressStart;

        // Rileva pressione config (10 sec) mentre premuto
        if (pressDuration >= BUTTON_CONFIG_PRESS_MS && lastButtonEvent < 3) {
            lastButtonEvent = 3;
            event = 3;
            Serial.println("[GPIO] Pulsante: pressione CONFIG (10s)");
        }
        // Rileva pressione lunga (3 sec) mentre premuto
        else if (pressDuration >= BUTTON_LONG_PRESS_MS && lastButtonEvent < 2) {
            lastButtonEvent = 2;
            // Non restituire evento qui, aspetta rilascio o config
        }
    }
    else if (!isPressed && buttonWasPressed) {
        // Pulsante rilasciato
        unsigned long pressDuration = now - buttonPressStart;

        if (pressDuration >= BUTTON_DEBOUNCE_MS) {
            if (lastButtonEvent == 0) {
                // Nessun evento ancora generato
                if (pressDuration >= BUTTON_LONG_PRESS_MS && pressDuration < BUTTON_CONFIG_PRESS_MS) {
                    event = 2;
                    Serial.println("[GPIO] Pulsante: pressione LUNGA (3s)");
                } else if (pressDuration >= BUTTON_SHORT_PRESS_MS && pressDuration < BUTTON_LONG_PRESS_MS) {
                    event = 1;
                    Serial.println("[GPIO] Pulsante: pressione BREVE");
                }
            }
        }

        buttonWasPressed = false;
        lastButtonEvent = 0;
    }

    return event;
}

// === Controlla durata pressione corrente ===
unsigned long getButtonPressDuration() {
    if (buttonWasPressed) {
        return millis() - buttonPressStart;
    }
    return 0;
}

bool isButtonCurrentlyPressed() {
    return digitalRead(PIN_BUTTON) == LOW;
}

// === Funzione di test hardware ===
void testHardware() {
    Serial.println("[GPIO] Test hardware in corso...");

    // Test LED WiFi
    Serial.println("[GPIO] Test LED WiFi");
    for (int i = 0; i < 3; i++) {
        setWifiLedRaw(true);
        delay(200);
        setWifiLedRaw(false);
        delay(200);
    }

    // Test LED Relay
    Serial.println("[GPIO] Test LED Relay");
    for (int i = 0; i < 3; i++) {
        setRelayLedRaw(true);
        delay(200);
        setRelayLedRaw(false);
        delay(200);
    }

    // Test Relay (breve)
    Serial.println("[GPIO] Test Relay (500ms)");
    setRelay(true);
    delay(500);
    setRelay(false);

    Serial.println("[GPIO] Test hardware completato");
}

#endif // GPIO_CONTROL_H
