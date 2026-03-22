#include "wifi_manager.h"
#include <WiFi.h>

// =========================
// CONFIGURAÇÕES DA REDE
// =========================
const char* ssid = "ROBO_VESPA";
const char* password = "12345678"; // mínimo 8 caracteres

// =========================
// INICIALIZAÇÃO DO WIFI
// =========================
void initWiFi() {
    Serial.println("[WIFI] Iniciando modo Access Point...");

    // Inicia como ponto de acesso
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();

    Serial.print("[WIFI] Rede criada!");
    Serial.print("\n[WIFI] SSID: ");
    Serial.print(ssid);
    Serial.print("\n[WIFI] IP: ");
    Serial.println(IP);
}

// =========================
// LOOP WIFI (futuro)
// =========================
void atualizarWiFi() {
    // Aqui vamos futuramente:
    // - tratar requisições web
    // - atualizar dados
}