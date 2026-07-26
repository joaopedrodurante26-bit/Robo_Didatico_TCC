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

enum UltrasonicStatus {
	ULTRA_OK = 0,
	ULTRA_TIMEOUT,
	ULTRA_ECHO_TOO_SHORT,
	ULTRA_ECHO_TOO_LONG,
	ULTRA_OUT_OF_RANGE,
	ULTRA_INVALID_READING,
	ULTRA_SENSOR_ERROR
};

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

// Leitura crua em microssegundos da última amostra válida.
unsigned long getUltraEchoTimeUs();

// Estado do último ciclo de leitura do HC-SR04.
UltrasonicStatus getUltraStatus();

// Sinaliza presença provável do sensor com base nas últimas leituras.
bool ultraSensorPresente();

// Diagnóstico básico de pinos e leitura.
bool ultraTriggerOk();
bool ultraEchoOk();

// Estatísticas para diagnóstico.
unsigned long getUltraReadCount();
unsigned long getUltraTimeoutCount();

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