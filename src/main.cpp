// =====================================================
// ARQUIVO PRINCIPAL (main.cpp)
// =====================================================
// Responsável por:
// - Inicializar todos os módulos do sistema
// - Executar o loop principal do robô
//
// Arquitetura geral:
//
//        WiFi (entrada)
//             ↓
//         Controle
//             ↓
//         Motores (saída)
//
//        Sensores (feedback)
//             ↑
//
// Este arquivo NÃO deve conter lógica específica,
// apenas orquestração dos módulos.
// =====================================================

#include <Arduino.h>

#include "wifi/wifi_manager.h"
#include "motores/motores.h"
#include "sensores/sensores.h"
#include "sensores/sensor_manager.h"
#include "controle/controle.h"
#include "utils/logger.h"
#include "diagnostico/diagnostico.h"
#include "robot/robot.h"
#include "testes/testes.h"

static unsigned long lastSensorsMs = 0;
static unsigned long lastControlMs = 0;
static unsigned long lastWifiMs = 0;

static const unsigned long SENSOR_PERIOD_MS = 5;   // ~200 Hz
static const unsigned long CONTROL_PERIOD_MS = 20; // ~50 Hz
static const unsigned long WIFI_PERIOD_MS = 100;   // ~10 Hz

// =====================================================
// SETUP
// =====================================================
// Executado uma única vez na inicialização
//

void setup() {
    // Comunicação serial
    Serial.begin(115200);
    delay(1000);

    // Logger
    initLogger();

    logInfo("BOOT: Sistema do robô iniciando...");

    // Hardware
    initMotores();
    initSensores();

    // Gerenciador central
    initRobot();

    initSensorManager();

    // Comunicação
    initWiFi();

    // Console administrativo
    testes_iniciar();

    logInfo("BOOT: Sistema inicializado!");
}

// =====================================================
// LOOP PRINCIPAL
// =====================================================
// Executado continuamente
//

void loop() {
    unsigned long now = millis();

    // -------------------------------------------------
    // ATUALIZAÇÃO DOS MÓDULOS
    // -------------------------------------------------
    // Ordem pensada como pipeline:
    //
    // 1. Sensores → leitura do mundo
    // 2. Controle → decisão
    // 3. Motores  → ação
    // 4. WiFi     → comunicação
    //

    // Interface
    atualizarTestes();

    if (now - lastSensorsMs >= SENSOR_PERIOD_MS) {
        lastSensorsMs = now;
        updateSensorManager();
    }

    if (now - lastControlMs >= CONTROL_PERIOD_MS) {
        lastControlMs = now;
        robot_update();
        atualizarMotores();
    }

    if (now - lastWifiMs >= WIFI_PERIOD_MS) {
        lastWifiMs = now;
        atualizarWiFi();
    }


    // -------------------------------------------------
    // CONTROLE DE LOOP
    // -------------------------------------------------
    // Pequeno delay para evitar uso excessivo da CPU
    // e manter estabilidade do sistema
    //
    delay(1);
}