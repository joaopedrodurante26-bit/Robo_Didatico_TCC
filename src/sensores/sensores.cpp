// =====================================================
// MÓDULO DE SENSORES - IMPLEMENTAÇÃO
// =====================================================
// Responsável por:
// - Leitura de encoders via interrupção
// - Armazenamento de pulsos
// - Fornecimento de dados para outros módulos
//
// IMPORTANTE:
// Este módulo trabalha com interrupções (ISR),
// portanto cuidados com concorrência são essenciais.
// =====================================================

#include "sensores.h"
#include <Wire.h>
#include <MPU6050.h>

// =====================================================
// DEFINIÇÃO DE PINOS
// =====================================================
// Escolhidos com base na disponibilidade da Vespa
// e compatibilidade com interrupções.
//

#define PIN_ENCODER_ESQ 5
#define PIN_ENCODER_DIR 18

#define PIN_TRIG 23
#define PIN_ECHO 19

// =====================================================
// VARIÁVEIS GLOBAIS (COMPARTILHADAS COM ISR)
// =====================================================
// 'volatile' é obrigatório pois são alteradas
// dentro de interrupções.
//

// Encoders
static volatile long pulsosEsq = 0;
static volatile long pulsosDir = 0;

// Ultrassônico
static float distancia = 0;

// IMU
static MPU6050 mpu;

static float accelX = 0, accelY = 0, accelZ = 0;
static float gyroX = 0, gyroY = 0, gyroZ = 0;

// =====================================================
// ROTINAS DE INTERRUPÇÃO (ISR)
// =====================================================
// IRAM_ATTR → garante execução rápida no ESP32
// Deve conter código mínimo (apenas incremento)
//

void IRAM_ATTR contarPulsoEsq() {
    pulsosEsq++;
}

void IRAM_ATTR contarPulsoDir() {
    pulsosDir++;
}

// =====================================================
// INICIALIZAÇÃO
// =====================================================

void initSensores() {
    // =========================
    // ENCODERS
    // =========================
    pinMode(PIN_ENCODER_ESQ, INPUT_PULLUP);
    pinMode(PIN_ENCODER_DIR, INPUT_PULLUP);

    // Associa interrupções aos pinos
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_ESQ), contarPulsoEsq, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_DIR), contarPulsoDir, RISING);

    // =========================
    // ULTRASSÔNICO
    // =========================
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);

    // =========================
    // IMU (I2C)
    // =========================
    Wire.begin(21, 22); // SDA, SCL

    mpu.initialize();

    if (!mpu.testConnection()) {
        Serial.println("[ERRO] MPU6050 não conectado!");
    } else {
        Serial.println("[IMU] MPU6050 conectado!");
    }
}

// =====================================================
// ATUALIZAÇÃO (PROCESSAMENTO FUTURO)
// =====================================================
// Aqui futuramente serão calculados:
//
// - velocidade (RPM)
// - distância percorrida
// - filtros de ruído
//

void atualizarSensores() {
    // =========================
    // ULTRASSÔNICO
    // =========================
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);

    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    long duracao = pulseIn(PIN_ECHO, HIGH, 20000); // timeout 20ms

    if (duracao == 0) {
        distancia = -1; // indica erro / fora de alcance
    } else {
        distancia = duracao * 0.0343 / 2;
    }

    // =========================
    // IMU
    // =========================
    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    if (mpu.testConnection()) {
        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    } else {
        Serial.println("[ERRO] Falha ao ler MPU6050!");
        return;
    }
    
    // Conversão simples (escala padrão)
    accelX = ax / 16384.0;
    accelY = ay / 16384.0;
    accelZ = az / 16384.0;

    gyroX = gx / 131.0;
    gyroY = gy / 131.0;
    gyroZ = gz / 131.0;
}

// =====================================================
// GETTERS (ACESSO SEGURO)
// =====================================================
// IMPORTANTE:
// Como as variáveis são alteradas por interrupção,
// precisamos garantir leitura consistente.
//

long getPulsosEsq() {
    noInterrupts();
    long valor = pulsosEsq;
    interrupts();
    return valor;
}

long getPulsosDir() {
    noInterrupts();
    long valor = pulsosDir;
    interrupts();
    return valor;
}

float getDistancia() {
    return distancia;
}

float getAccelX() { return accelX; }
float getAccelY() { return accelY; }
float getAccelZ() { return accelZ; }

float getGyroX() { return gyroX; }
float getGyroY() { return gyroY; }
float getGyroZ() { return gyroZ; }

// =====================================================
// RESET DOS ENCODERS
// =====================================================
// Também precisa ser protegido contra interrupções
//

void resetEncoders() {
    noInterrupts();
    pulsosEsq = 0;
    pulsosDir = 0;
    interrupts();
}