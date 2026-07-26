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
    initSensorManager();

    // Comunicação
    initWiFi();

    // Gerenciador central
    initRobot();

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

    // Gerenciador central
    robot_update();

    // Hardware
    updateSensorManager();
    atualizarMotores();
    atualizarWiFi();


    // -------------------------------------------------
    // CONTROLE DE LOOP
    // -------------------------------------------------
    // Pequeno delay para evitar uso excessivo da CPU
    // e manter estabilidade do sistema
    //
    delay(10);
}