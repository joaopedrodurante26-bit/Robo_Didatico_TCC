#ifndef CONTROLE_H
#define CONTROLE_H
#include <Arduino.h>

// Estados possíveis do robô
enum Comando {
    PARAR,
    FRENTE,
    TRAS,
    ESQUERDA,
    DIREITA
};

enum EstadoSistema {
    MANUAL,
    AUTOMATICO
};

void initControle();
void setComando(Comando cmd);
void atualizarControle();

#endif