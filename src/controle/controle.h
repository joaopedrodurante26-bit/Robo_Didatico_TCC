#ifndef CONTROLE_H
#define CONTROLE_H

#include <Arduino.h>

// =====================================================
// MÓDULO DE CONTROLE DO ROBÔ
// =====================================================
// Responsável por:
// - Definir os comandos possíveis do robô
// - Gerenciar o estado atual de controle
// - Servir como interface entre entrada (Wi-Fi, joystick)
//   e atuação (motores)
//
// Este módulo atua como "cérebro lógico" do robô.
// =====================================================

// =====================================================
// ENUM: COMANDOS DO ROBÔ
// =====================================================
// Define os movimentos básicos possíveis.
// Esses comandos são independentes do hardware,
// permitindo reutilização em simulação ou controle real.
//
enum Comando {
PARAR,     // Robô parado
FRENTE,    // Movimento para frente
TRAS,      // Movimento para trás
ESQUERDA,  // Rotação à esquerda
DIREITA    // Rotação à direita
};

// =====================================================
// ENUM: ESTADO DO SISTEMA
// =====================================================
// Define o modo de operação do robô.
//
enum EstadoSistema {
MANUAL,     // Controle externo (Wi-Fi, joystick)
AUTOMATICO  // Controle autônomo (sensores/IA futura)
};

// =====================================================
// INTERFACE PÚBLICA
// =====================================================

// -----------------------------------------------------
// Retorna o comando atual do robô
// -----------------------------------------------------
// Usado por outros módulos (ex: motores)
// para saber qual ação executar.
//
Comando getComando();

// -----------------------------------------------------
// Inicializa o sistema de controle
// -----------------------------------------------------
// Deve ser chamado no setup()
// Responsável por definir estado inicial.
//
void initControle();

// -----------------------------------------------------
// Define um novo comando para o robô
// -----------------------------------------------------
// Usado principalmente pelo módulo Wi-Fi
// ou qualquer outra interface de entrada.
//
void setComando(Comando cmd);

// -----------------------------------------------------
// Atualiza o estado do controle
// -----------------------------------------------------
// Deve ser chamado no loop()
// Responsável por aplicar lógica de controle,
// como priorização, filtros ou estados futuros.
//
void atualizarControle();

#endif