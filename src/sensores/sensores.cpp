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
#include <math.h>

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
static unsigned long ultraEchoTimeUs = 0;
static UltrasonicStatus ultraStatus = ULTRA_SENSOR_ERROR;
static bool ultraTrigOk = false;
static bool ultraEchoPinOk = false;
static bool ultraPresente = false;
static unsigned long ultraReadCount = 0;
static unsigned long ultraTimeoutCount = 0;
static uint8_t ultraTimeoutConsecutivo = 0;

static const unsigned long ULTRA_TIMEOUT_US = 30000UL;
static const uint8_t ULTRA_TIMEOUT_PARA_AUSENCIA = 3;
static const float ULTRA_MIN_DISTANCE_CM = 2.0f;
static const float ULTRA_MAX_DISTANCE_CM = 400.0f;
static const unsigned long ULTRA_MIN_ECHO_US = 115UL;
static const unsigned long ULTRA_MAX_ECHO_US = 23500UL;

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

static void ultra_initDriver() {
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    digitalWrite(PIN_TRIG, LOW);

    ultraTrigOk = true;
    ultraEchoPinOk = true;
    ultraPresente = false;
    ultraStatus = ULTRA_SENSOR_ERROR;
    ultraEchoTimeUs = 0;
    ultraReadCount = 0;
    ultraTimeoutCount = 0;
    ultraTimeoutConsecutivo = 0;
}

static void ultra_updateDriver() {
    ultraReadCount++;

    // Rejeita condição elétrica inválida do ECHO travado em HIGH.
    if (digitalRead(PIN_ECHO) == HIGH) {
        ultraStatus = ULTRA_SENSOR_ERROR;
        ultraEchoPinOk = false;
        distancia = -1.0f;
        ultraTimeoutConsecutivo = ULTRA_TIMEOUT_PARA_AUSENCIA;
        ultraPresente = false;
        return;
    }

    ultraEchoPinOk = true;

    // Pulso de disparo de 10us para o HC-SR04.
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    unsigned long duracao = pulseIn(PIN_ECHO, HIGH, ULTRA_TIMEOUT_US);
    ultraEchoTimeUs = duracao;

    if (duracao == 0) {
        ultraStatus = ULTRA_TIMEOUT;
        ultraTimeoutCount++;
        if (ultraTimeoutConsecutivo < 255) {
            ultraTimeoutConsecutivo++;
        }
        ultraPresente = (ultraTimeoutConsecutivo < ULTRA_TIMEOUT_PARA_AUSENCIA);
        distancia = -1.0f;
        return;
    }

    if (duracao < ULTRA_MIN_ECHO_US) {
        ultraStatus = ULTRA_ECHO_TOO_SHORT;
        ultraTimeoutConsecutivo = 0;
        ultraPresente = true;
        distancia = -1.0f;
        return;
    }

    if (duracao > ULTRA_MAX_ECHO_US) {
        ultraStatus = ULTRA_ECHO_TOO_LONG;
        ultraTimeoutConsecutivo = 0;
        ultraPresente = true;
        distancia = -1.0f;
        return;
    }

    float distanciaCalculada = static_cast<float>(duracao) * 0.0343f / 2.0f;
    if (!isfinite(distanciaCalculada) || distanciaCalculada <= 0.0f) {
        ultraStatus = ULTRA_INVALID_READING;
        ultraTimeoutConsecutivo = 0;
        ultraPresente = true;
        distancia = -1.0f;
        return;
    }

    if (distanciaCalculada < ULTRA_MIN_DISTANCE_CM || distanciaCalculada > ULTRA_MAX_DISTANCE_CM) {
        ultraStatus = ULTRA_OUT_OF_RANGE;
        ultraTimeoutConsecutivo = 0;
        ultraPresente = true;
        distancia = distanciaCalculada;
        return;
    }

    ultraStatus = ULTRA_OK;
    ultraTimeoutConsecutivo = 0;
    ultraPresente = true;
    distancia = distanciaCalculada;
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
    ultra_initDriver();

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

static void imu_updateDriver() {
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
// ATUALIZAÇÃO (PROCESSAMENTO FUTURO)
// =====================================================
// Aqui futuramente serão calculados:
//
// - velocidade (RPM)
// - distância percorrida
// - filtros de ruído
//

void atualizarSensores() {
    atualizarUltrassonico();
    atualizarIMU();
}

void atualizarUltrassonico() {
    // =========================
    // ULTRASSÔNICO
    // =========================
    ultra_updateDriver();
}

void atualizarIMU() {
    // =========================
    // IMU
    // =========================
    imu_updateDriver();
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

unsigned long getUltraEchoTimeUs() {
    return ultraEchoTimeUs;
}

UltrasonicStatus getUltraStatus() {
    return ultraStatus;
}

const char* ultraStatusToString(UltrasonicStatus status) {
    switch (status) {
        case ULTRA_OK: return "OK";
        case ULTRA_TIMEOUT: return "TIMEOUT";
        case ULTRA_ECHO_TOO_SHORT: return "ECHO TOO SHORT";
        case ULTRA_ECHO_TOO_LONG: return "ECHO TOO LONG";
        case ULTRA_OUT_OF_RANGE: return "OUT OF RANGE";
        case ULTRA_INVALID_READING: return "INVALID READING";
        case ULTRA_SENSOR_ERROR: return "SENSOR ERROR";
        default: return "UNKNOWN";
    }
}

bool ultraSensorPresente() {
    return ultraPresente;
}

bool ultraTriggerOk() {
    return ultraTrigOk;
}

bool ultraEchoOk() {
    return ultraEchoPinOk;
}

unsigned long getUltraReadCount() {
    return ultraReadCount;
}

unsigned long getUltraTimeoutCount() {
    return ultraTimeoutCount;
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