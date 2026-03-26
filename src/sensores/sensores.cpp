#include "sensores.h"

// =========================
// DEFINIÇÃO DE PINOS
// =========================

#define PIN_ENCODER_ESQ 18
#define PIN_ENCODER_DIR 5

// =========================
// VARIÁVEIS GLOBAIS
// =========================

volatile long pulsosEsq = 0;
volatile long pulsosDir = 0;

// =========================
// INTERRUPÇÕES
// =========================

void IRAM_ATTR contarPulsoEsq() {
    pulsosEsq++;
}

void IRAM_ATTR contarPulsoDir() {
    pulsosDir++;
}

// =========================
// INICIALIZAÇÃO
// =========================

void initSensores() {

    pinMode(PIN_ENCODER_ESQ, INPUT);
    pinMode(PIN_ENCODER_DIR, INPUT);

    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_ESQ), contarPulsoEsq, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_DIR), contarPulsoDir, RISING);
}

// =========================
// ATUALIZAÇÃO (futuro)
// =========================

void atualizarSensores() {
    // Futuramente: cálculo de velocidade
}

// =========================
// GETTERS
// =========================

long getPulsosEsq() {
    return pulsosEsq;
}

long getPulsosDir() {
    return pulsosDir;
}

// =========================
// RESET
// =========================

void resetEncoders() {
    pulsosEsq = 0;
    pulsosDir = 0;
}