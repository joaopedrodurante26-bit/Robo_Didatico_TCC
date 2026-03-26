#ifndef SENSORES_H
#define SENSORES_H

#include <Arduino.h>

// Inicialização
void initSensores();

// Atualização geral
void atualizarSensores();

// =========================
// ENCODERS
// =========================

long getPulsosEsq();
long getPulsosDir();

void resetEncoders();

#endif