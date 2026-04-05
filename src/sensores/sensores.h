// =====================================================
// MÓDULO DE SENSORES - INTERFACE
// =====================================================
// Responsável por:
// - Inicializar e gerenciar todos os sensores do robô
// - Fornecer dados organizados para outros módulos
//
// Este módulo centraliza a leitura de sensores como:
// - Encoders (movimento)
// - Ultrassônico (distância) [futuro]
// - IMU (aceleração/rotação) [futuro]
//
// Ele NÃO deve conter lógica de decisão,
// apenas aquisição e processamento de dados.
// =====================================================

#ifndef SENSORES_H
#define SENSORES_H

#include <Arduino.h>

// =====================================================
// INICIALIZAÇÃO E ATUALIZAÇÃO
// =====================================================

// -----------------------------------------------------
// Inicializa todos os sensores
// -----------------------------------------------------
// Deve configurar pinos, interrupções e comunicação.
//
void initSensores();

// -----------------------------------------------------
// Atualiza leituras dos sensores
// -----------------------------------------------------
// Deve ser chamada no loop()
// Responsável por cálculos derivados (ex: velocidade).
//
void atualizarSensores();

// =====================================================
// ENCODERS (MOVIMENTO)
// =====================================================
// Responsáveis por medir rotação das rodas,
// permitindo cálculo de:
// - velocidade
// - distância
// - odometria (futuro)
// =====================================================

// -----------------------------------------------------
// Retorna número de pulsos do encoder esquerdo
// -----------------------------------------------------
long getPulsosEsq();

// -----------------------------------------------------
// Retorna número de pulsos do encoder direito
// -----------------------------------------------------
long getPulsosDir();

// -----------------------------------------------------
// Zera contadores de pulsos
// -----------------------------------------------------
// Útil para:
// - medir deslocamento em trechos
// - reinicializar odometria
//
void resetEncoders();

// =====================================================
// ULTRASSÔNICO
// =====================================================
// Mede distância em centímetros
//

float getDistancia();

// =====================================================
// IMU (ACELERÔMETRO + GIROSCÓPIO)
// =====================================================

float getAccelX();
float getAccelY();
float getAccelZ();

float getGyroX();
float getGyroY();
float getGyroZ();

#endif