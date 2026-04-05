// =====================================================
// MÓDULO DE MOTORES - IMPLEMENTAÇÃO (VESPA)
// =====================================================
// Responsável por:
// - Controlar os motores via biblioteca da Vespa
// - Traduzir comandos do módulo de controle em movimento
//
// IMPORTANTE:
// A Vespa já gerencia PWM e direção internamente.
// NÃO usamos ledc diretamente aqui.
// =====================================================

#include "motores.h"
#include <Arduino.h>
#include "../controle/controle.h"
#include <RoboCore_Vespa.h>

// =====================================================
// OBJETO DE CONTROLE DOS MOTORES
// =====================================================

VespaMotors motores;

// =====================================================
// INICIALIZAÇÃO
// =====================================================

void initMotores() {
    // A inicialização já acontece no construtor
    motores.stop();
}

// =====================================================
// CONTROLE DE BAIXO NÍVEL
// =====================================================
// Converte velocidades diferenciais para a API da Vespa
//

void setVelocidade(int velEsq, int velDir) {
    // Limita valores
    velEsq = constrain(velEsq, -100, 100);
    velDir = constrain(velDir, -100, 100);
    // Aplica diretamente na Vespa
    motores.turn(velEsq, velDir);
}

// =====================================================
// CONTROLE DE ALTO NÍVEL
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
    Comando cmd = getComando();

    switch (cmd) {

        case FRENTE:
            moverFrente(100);
            break;

        case TRAS:
            moverTras(100);
            break;

        case ESQUERDA:
            virarEsquerda(100);
            break;

        case DIREITA:
            virarDireita(100);
            break;

        case PARAR:
        default:
            pararMotores();
            break;
    }
}