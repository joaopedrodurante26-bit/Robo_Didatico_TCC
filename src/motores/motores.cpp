// =====================================================
// MÓDULO DE MOTORES - IMPLEMENTAÇÃO (PWM DIRETO)
// =====================================================
// Responsável por:
// - Controle direto dos motores via PWM (ESP32)
// - Controle diferencial (tanque)
// =====================================================

#include "motores.h"

#include <Arduino.h>

#include "../config/pinos.h"
#include "../config/configuracao.h"
#include "../controle/controle.h"
#include "../utils/logger.h"
#include "../config/configuracao.h"

// =====================================================
// VARIÁVEIS INTERNAS
// =====================================================

static int velEsqAtual = 0;
static int velDirAtual = 0;

// =====================================================
// FUNÇÃO DE SUAVIZAÇÃO (RAMP)
// =====================================================

static int suavizar(int atual, int alvo, int passo = 5) {
    if (atual < alvo) return min(atual + passo, alvo);
    if (atual > alvo) return max(atual - passo, alvo);
    return atual;
}

// -----------------------------------------------------
// Controla um motor individual.
// velocidade > 0 -> frente
// velocidade < 0 -> ré
// velocidade = 0 -> parado
// -----------------------------------------------------

static void controlarMotor(
    uint8_t canalPWM,
    uint8_t pinoIN1,
    uint8_t pinoIN2,
    int velocidade)
{
    velocidade = constrain(velocidade, PWM_MIN, PWM_MAX);

    if (velocidade > 0)
    {
        digitalWrite(pinoIN1, HIGH);
        digitalWrite(pinoIN2, LOW);
    }
    else if (velocidade < 0)
    {
        digitalWrite(pinoIN1, LOW);
        digitalWrite(pinoIN2, HIGH);
        velocidade = -velocidade;
    }
    else
    {
        digitalWrite(pinoIN1, LOW);
        digitalWrite(pinoIN2, LOW);
    }

    ledcWrite(canalPWM, velocidade);
}

// =====================================================
// INICIALIZAÇÃO
// =====================================================

void initMotores()
{
    pinMode(PIN_MOTOR_E_IN1, OUTPUT);
    pinMode(PIN_MOTOR_E_IN2, OUTPUT);

    pinMode(PIN_MOTOR_D_IN1, OUTPUT);
    pinMode(PIN_MOTOR_D_IN2, OUTPUT);

    ledcSetup(PWM_CANAL_MOTOR_E, PWM_FREQUENCIA, PWM_RESOLUCAO);
    ledcAttachPin(PIN_MOTOR_E_PWM, PWM_CANAL_MOTOR_E);

    ledcSetup(PWM_CANAL_MOTOR_D, PWM_FREQUENCIA, PWM_RESOLUCAO);
    ledcAttachPin(PIN_MOTOR_D_PWM, PWM_CANAL_MOTOR_D);

    pararMotores();

    logInfo("Motores inicializados.");
}

// =====================================================
// INTERFACE PRINCIPAL
// =====================================================

void setVelocidade(int velEsq, int velDir)
{
    velEsqAtual = suavizar(velEsqAtual, velEsq);
    velDirAtual = suavizar(velDirAtual, velDir);

    controlarMotor(
        PWM_CANAL_MOTOR_E,
        PIN_MOTOR_E_IN1,
        PIN_MOTOR_E_IN2,
        velEsqAtual);

    controlarMotor(
        PWM_CANAL_MOTOR_D,
        PIN_MOTOR_D_IN1,
        PIN_MOTOR_D_IN2,
        velDirAtual);
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