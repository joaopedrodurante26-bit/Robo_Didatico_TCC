#include "motores.h"
#include <Arduino.h>
#include "../controle/controle.h"

// 🔧 DEFINA SEUS PINOS AQUI (exemplo)
#define PWM_ESQ  13
#define DIR_ESQ  14
#define PWM_DIR  27
#define DIR_DIR  4

// configurações de pinos para controle de direção
#define CANAL_ESQ 0
#define CANAL_DIR 1
#define FREQ 1000
#define RES 8

void initMotores() {
    ledcSetup(CANAL_ESQ, FREQ, RES);
    ledcAttachPin(PWM_ESQ, CANAL_ESQ);

    ledcSetup(CANAL_DIR, FREQ, RES);
    ledcAttachPin(PWM_DIR, CANAL_DIR);

    pinMode(DIR_ESQ, OUTPUT);
    pinMode(DIR_DIR, OUTPUT);

    pararMotores();
}

// =========================
// MOVIMENTOS
// =========================

void setVelocidade(int velEsq, int velDir) {
    // Direção esquerda
    if (velEsq >= 0) {
        digitalWrite(DIR_ESQ, HIGH);
    } else {
        digitalWrite(DIR_ESQ, LOW);
        velEsq = -velEsq;
    }

    // Direção direita
    if (velDir >= 0) {
        digitalWrite(DIR_DIR, HIGH);
    } else {
        digitalWrite(DIR_DIR, LOW);
        velDir = -velDir;
    }

    // Limita PWM (0–255)
    velEsq = constrain(velEsq, 0, 255);
    velDir = constrain(velDir, 0, 255);

    ledcWrite(CANAL_ESQ, velEsq);
    ledcWrite(CANAL_DIR, velDir);
}

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

void atualizarMotores() {
    Comando cmd = getComando();

    switch (cmd) {
        case FRENTE:
            moverFrente(200);
            break;

        case TRAS:
            moverTras(200);
            break;

        case ESQUERDA:
            virarEsquerda(200);
            break;

        case DIREITA:
            virarDireita(200);
            break;

        case PARAR:
        default:
            pararMotores();
            break;
    }
}