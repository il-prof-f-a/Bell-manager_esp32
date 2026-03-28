#ifndef STATE_SYNC_H
#define STATE_SYNC_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// ============================================
// Sincronizzazione stato condiviso tra task
// Usa un mutex ricorsivo per permettere helper annidati
// ============================================

static SemaphoreHandle_t stateMutex = NULL;

void initStateSync() {
    if (stateMutex != NULL) {
        return;
    }

    stateMutex = xSemaphoreCreateRecursiveMutex();
    if (stateMutex == NULL) {
        Serial.println("[STATE] ERRORE: impossibile creare mutex stato");
    } else {
        Serial.println("[STATE] Mutex stato inizializzato");
    }
}

bool lockSharedState(TickType_t timeout = portMAX_DELAY) {
    if (stateMutex == NULL) {
        return true;
    }
    return xSemaphoreTakeRecursive(stateMutex, timeout) == pdTRUE;
}

void unlockSharedState() {
    if (stateMutex != NULL) {
        xSemaphoreGiveRecursive(stateMutex);
    }
}

#endif // STATE_SYNC_H
