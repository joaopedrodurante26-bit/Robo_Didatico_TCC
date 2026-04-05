// =====================================================
// MÓDULO DE SENSORES - IMPLEMENTAÇÃO
// =====================================================
// Responsável por:
// - Leitura de encoders via interrupção
// - Armazenamento de pulsos
// - Fornecimento de dados para outros módulos
//
// IMPORTANTE:
// Este módulo trabalha com interrupções (ISR),
// portanto cuidados com concorrência são essenciais.
// =====================================================

#include "sensores.h"

// =====================================================
// DEFINIÇÃO DE PINOS
// =====================================================
// Escolhidos com base na disponibilidade da Vespa
// e compatibilidade com interrupções.
//

#define PIN_ENCODER_ESQ 5
#define PIN_ENCODER_DIR 18

// =====================================================
// VARIÁVEIS GLOBAIS (COMPARTILHADAS COM ISR)
// =====================================================
// 'volatile' é obrigatório pois são alteradas
// dentro de interrupções.
//

static volatile long pulsosEsq = 0;
static volatile long pulsosDir = 0;

// =====================================================
// ROTINAS DE INTERRUPÇÃO (ISR)
// =====================================================
// IRAM_ATTR → garante execução rápida no ESP32
// Deve conter código mínimo (apenas incremento)
//

void IRAM_ATTR contarPulsoEsq() {
    pulsosEsq++;
}

void IRAM_ATTR contarPulsoDir() {
    pulsosDir++;
}

// =====================================================
// INICIALIZAÇÃO
// =====================================================

void initSensores() {
    // Configura pinos como entrada
    pinMode(PIN_ENCODER_ESQ, INPUT);
    pinMode(PIN_ENCODER_DIR, INPUT);

    // Associa interrupções aos pinos
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_ESQ), contarPulsoEsq, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_DIR), contarPulsoDir, RISING);
}

// =====================================================
// ATUALIZAÇÃO (PROCESSAMENTO FUTURO)
// =====================================================
// Aqui futuramente serão calculados:
//
// - velocidade (RPM)
// - distância percorrida
// - filtros de ruído
//

void atualizarSensores() {
    // Implementação futura
}

// =====================================================
// GETTERS (ACESSO SEGURO)
// =====================================================
// IMPORTANTE:
// Como as variáveis são alteradas por interrupção,
// precisamos garantir leitura consistente.
//

long getPulsosEsq() {
    noInterrupts();
    long valor = pulsosEsq;
    interrupts();
    return valor;
}

long getPulsosDir() {
    noInterrupts();
    long valor = pulsosDir;
    interrupts();
    return valor;
}

// =====================================================
// RESET DOS ENCODERS
// =====================================================
// Também precisa ser protegido contra interrupções
//

void resetEncoders() {
    noInterrupts();
    pulsosEsq = 0;
    pulsosDir = 0;
    interrupts();
}