#ifndef CONTROLE_H
#define CONTROLE_H

#include <Arduino.h>

// =========================
// INICIALIZAÇÃO
// =========================

void initControle();
void atualizarControle();

// =========================
// ENTRADA (COMANDOS)
// =========================

// Joystick (-1 a 1)
void setJoystick(float x, float y);

// =========================
// SAÍDA (PARA MOTORES)
// =========================

int getVelEsq();
int getVelDir();

#endif