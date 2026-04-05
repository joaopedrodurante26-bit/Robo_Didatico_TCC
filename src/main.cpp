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
#include "controle/controle.h"

// =====================================================
// SETUP
// =====================================================
// Executado uma única vez na inicialização
//

void setup() {
    // Inicializa comunicação serial para debug
    Serial.begin(115200);
    Serial.println("\n[BOOT] Iniciando sistema do robô...");

    // -------------------------------------------------
    // INICIALIZAÇÃO DOS MÓDULOS
    // -------------------------------------------------
    // Ordem importante:
    // - Controle antes de motores (boa prática futura)
    // - Sensores antes de uso
    // - WiFi por último (depende do sistema já estável)
    //

    initControle();
    initMotores();
    initSensores();
    initWiFi();

    Serial.println("[BOOT] Sistema inicializado com sucesso!");
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

    atualizarSensores();
    atualizarControle();
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