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

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "sensores.h"
#include "../config/pinos.h"
#include "../utils/logger.h"

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
static Adafruit_MPU6050 mpu;

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
    pinMode(PIN_ENCODER_E, INPUT_PULLUP);
    pinMode(PIN_ENCODER_D, INPUT_PULLUP);

    // Associa interrupções aos pinos
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_E), contarPulsoEsq, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_D), contarPulsoDir, RISING);

    // =========================
    // ULTRASSÔNICO
    // =========================
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);

    // =========================
    // IMU (I2C)
    // =========================
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    if (!mpu.begin())
    {
        logError("IMU: MPU6050 não encontrado!");
    }
    else
    {
        logInfo("IMU: MPU6050 conectado com sucesso.");

        // Configuração padrão
        mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
        mpu.setGyroRange(MPU6050_RANGE_250_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
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
    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;

    mpu.getEvent(&accel, &gyro, &temp);

    accelX = accel.acceleration.x;
    accelY = accel.acceleration.y;
    accelZ = accel.acceleration.z;

    gyroX = gyro.gyro.x;
    gyroY = gyro.gyro.y;
    gyroZ = gyro.gyro.z;
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