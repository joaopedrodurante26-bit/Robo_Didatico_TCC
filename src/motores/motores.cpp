// =====================================================
// MÓDULO DE MOTORES - IMPLEMENTAÇÃO (PWM DIRETO)
// =====================================================
// Responsável por:
// - Controle direto dos motores via PWM (ESP32)
// - Controle diferencial (tanque)
// =====================================================

#include "motores.h"
#include <Arduino.h>
#include "../controle/controle.h"

// =====================================================
// DEFINIÇÃO DE PINOS
// =====================================================
// Ajuste conforme sua placa Vespa

#define PWM_ESQ  13
#define DIR_ESQ  14

#define PWM_DIR  27
#define DIR_DIR  4

// =====================================================
// CONFIGURAÇÃO PWM (LEDC)
// =====================================================

#define CANAL_ESQ 0
#define CANAL_DIR 1

#define FREQ 1000
#define RES 8

// =====================================================
// VARIÁVEIS INTERNAS
// =====================================================

static int velEsqAtual = 0;
static int velDirAtual = 0;

// =====================================================
// FUNÇÃO DE SUAVIZAÇÃO (RAMP)
// =====================================================

int suavizar(int atual, int alvo, int passo = 5) {
    if (atual < alvo) return min(atual + passo, alvo);
    if (atual > alvo) return max(atual - passo, alvo);
    return atual;
}

// =====================================================
// INICIALIZAÇÃO
// =====================================================

void initMotores() {
    ledcSetup(CANAL_ESQ, FREQ, RES);
    ledcAttachPin(PWM_ESQ, CANAL_ESQ);

    ledcSetup(CANAL_DIR, FREQ, RES);
    ledcAttachPin(PWM_DIR, CANAL_DIR);

    pinMode(DIR_ESQ, OUTPUT);
    pinMode(DIR_DIR, OUTPUT);

    pararMotores();
}

// =====================================================
// CONTROLE DE BAIXO NÍVEL
// =====================================================

void aplicarPWM(int velEsq, int velDir) {

    // =========================
    // DIREÇÃO
    // =========================

    if (velEsq >= 0) {
        digitalWrite(DIR_ESQ, HIGH);
    } else {
        digitalWrite(DIR_ESQ, LOW);
        velEsq = -velEsq;
    }

    if (velDir >= 0) {
        digitalWrite(DIR_DIR, HIGH);
    } else {
        digitalWrite(DIR_DIR, LOW);
        velDir = -velDir;
    }

    // =========================
    // LIMITAÇÃO
    // =========================

    velEsq = constrain(velEsq, 0, 255);
    velDir = constrain(velDir, 0, 255);

    // =========================
    // APLICA PWM
    // =========================

    ledcWrite(CANAL_ESQ, velEsq);
    ledcWrite(CANAL_DIR, velDir);
}

// =====================================================
// INTERFACE PRINCIPAL
// =====================================================

void setVelocidade(int velEsq, int velDir) {

    // Suavização
    velEsqAtual = suavizar(velEsqAtual, velEsq);
    velDirAtual = suavizar(velDirAtual, velDir);

    aplicarPWM(velEsqAtual, velDirAtual);
}

// =====================================================
// MOVIMENTOS DE ALTO NÍVEL
// =====================================================

void moverFrente(int v) {
    setVelocidade(v, v);
}

void moverTras(int v) {
    setVelocidade(-v, -v);
}

void virarEsquerda(int v) {
    setVelocidade(-v, v);
}

void virarDireita(int v) {
    setVelocidade(v, -v);
}

void pararMotores() {
    setVelocidade(0, 0);
}

// =====================================================
// ATUALIZAÇÃO BASEADA NO CONTROLE
// =====================================================

void atualizarMotores() {
    setVelocidade(getVelEsq(), getVelDir());
}