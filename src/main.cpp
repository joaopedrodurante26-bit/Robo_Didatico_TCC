#include <Arduino.h>
#include "wifi/wifi_manager.h"
#include "motores/motores.h"
#include "sensores/sensores.h"
#include "controle/controle.h"

// =========================
// SETUP
// =========================
void setup() {
    Serial.begin(115200);
    Serial.println("\n[BOOT] Iniciando sistema do robô...");

    // Inicialização dos módulos
    initMotores();
    initSensores();
    initWiFi();
    initControle();

    Serial.println("[BOOT] Sistema inicializado com sucesso!");
}

// =========================
// LOOP PRINCIPAL
// =========================
void loop() {
    // Atualização contínua dos módulos
    atualizarSensores();
    atualizarControle();
    atualizarMotores();
    atualizarWiFi();
    // Pequeno delay para estabilidade (evitar loop agressivo)
    delay(10);
}