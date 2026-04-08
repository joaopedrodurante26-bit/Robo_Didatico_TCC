// =====================================================
// MÓDULO DE CONTROLE - IMPLEMENTAÇÃO (REFORMULADO)
// =====================================================
// Responsável por:
// - Receber entradas (joystick)
// - Processar lógica de movimento
// - Gerar velocidades diferenciais
// =====================================================

#include "controle.h"

// =========================
// VARIÁVEIS INTERNAS
// =========================

// Entrada (joystick)
static float inputX = 0.0;
static float inputY = 0.0;

// Saída (motores)
static int velEsq = 0;
static int velDir = 0;

// Ganho geral (limite de velocidade)
static int ganho = 255;

// =========================
// INICIALIZAÇÃO
// =========================

void initControle() {
    inputX = 0;
    inputY = 0;
    velEsq = 0;
    velDir = 0;
}

// =========================
// SET INPUT
// =========================

void setJoystick(float x, float y) {
    inputX = constrain(x, -1.0, 1.0);
    inputY = constrain(y, -1.0, 1.0);
}

// =========================
// PROCESSAMENTO
// =========================

void atualizarControle() {

    // =========================
    // NORMALIZAÇÃO
    // =========================

    float soma = inputY + inputX;
    float diff = inputY - inputX;

    float maxVal = max(abs(soma), abs(diff));

    if (maxVal > 1.0) {
        soma /= maxVal;
        diff /= maxVal;
    }

    // =========================
    // CONVERSÃO PARA PWM
    // =========================

    velEsq = soma * ganho;
    velDir = diff * ganho;
}

// =========================
// GETTERS
// =========================

int getVelEsq() {
    return velEsq;
}

int getVelDir() {
    return velDir;
}