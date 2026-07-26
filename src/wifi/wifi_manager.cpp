// =====================================================
// MÓDULO WIFI MANAGER - IMPLEMENTAÇÃO
// =====================================================
// Responsável por:
// - Criar rede Wi-Fi (Access Point)
// - Servir interface web (HTML + JS)
// - Receber comandos do usuário (joystick)
// - Disponibilizar dados do robô via API (/status)
//
// Atua como ponte entre:
// Interface Web ↔ Controle ↔ Motores/Sensores
// =====================================================

#include "wifi_manager.h"
#include "controle/controle.h"
#include "motores/motores.h"
#include "sensores/sensores.h"
#include "../utils/logger.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

static bool montarLittleFS() {
    if (LittleFS.begin(true)) {
        logInfo("WIFI: LittleFS montado com sucesso.");
        return true;
    }

    logError("WIFI: Falha ao montar LittleFS. Tentando formatar...");

    if (LittleFS.format()) {
        logInfo("WIFI: LittleFS formatado com sucesso.");
        return LittleFS.begin();
    }

    logError("WIFI: Não foi possível formatar LittleFS.");
    return false;
}

// =====================================================
// CONFIGURAÇÕES DA REDE
// =====================================================

const char* ssid = "ROBO_VESPA";
const char* password = "12345678"; // mínimo 8 caracteres

// =====================================================
// SERVIDOR WEB
// =====================================================

static WebServer server(80);

static void enviarArquivoOuFallback(const char* path, const char* contentType, const String& fallback) {
    if (!LittleFS.exists(path)) {
        logWarn(String("Arquivo não encontrado em LittleFS: ") + path);
        server.send(200, contentType, fallback);
        return;
    }

    File file = LittleFS.open(path, "r");
    if (!file) {
        logWarn(String("Falha ao abrir arquivo em LittleFS: ") + path);
        server.send(200, contentType, fallback);
        return;
    }

    server.streamFile(file, contentType);
    file.close();
}

// =====================================================
// ROTAS HTTP
// =====================================================

static void configurarRotas() {
    // -------------------------------------------------
    // ROTA: Página principal
    // -------------------------------------------------
    server.on("/", []() {
        logInfo("[HTTP] Cliente acessou /");
        enviarArquivoOuFallback(
            "/index.html",
            "text/html",
            "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Robo Vespa</title><style>body{font-family:Arial,sans-serif;background:#111;color:#fff;padding:24px}h1{margin-bottom:8px}p{color:#ccc}</style></head><body><h1>Robô Vespa</h1><p>Interface web pronta no firmware.</p></body></html>"
        );
    });

    // -------------------------------------------------
    // ROTA: Script JS
    // -------------------------------------------------
    server.on("/script.js", []() {
        enviarArquivoOuFallback(
            "/script.js",
            "application/javascript",
            "console.log('Script fallback carregado');"
        );
    });

    // -------------------------------------------------
    // ROTA: Estilo CSS
    // -------------------------------------------------
    server.on("/style.css", []() {
        enviarArquivoOuFallback(
            "/style.css",
            "text/css",
            "body{font-family:Arial,sans-serif;text-align:center;}"
        );
    });


    // -------------------------------------------------
    // ROTA: CONTROLE (Joystick)
    // -------------------------------------------------
    server.on("/controle", []() {

        if (server.hasArg("x") && server.hasArg("y")) {

            float x = server.arg("x").toFloat();
            float y = server.arg("y").toFloat();

            Serial.print("[JOY] X: ");
            Serial.print(x);
            Serial.print(" | Y: ");
            Serial.println(y);

            // -------------------------------------------------
            // CONVERSÃO: Joystick → Velocidade diferencial
            // -------------------------------------------------
            setJoystick(x, y);
        }

        server.send(200, "text/plain", "OK");
    });


    // -------------------------------------------------
    // ROTA: STATUS DO ROBÔ (API)
    // -------------------------------------------------
    server.on("/status", []() {

        String json = "{";

        // =========================
        // ULTRASSÔNICO
        // =========================
        json += "\"distancia\": " + String(getDistancia()) + ",";

        // =========================
        // ENCODERS
        // =========================
        json += "\"encoder_esq\": " + String(getPulsosEsq()) + ",";
        json += "\"encoder_dir\": " + String(getPulsosDir()) + ",";

        // =========================
        // ACELERÔMETRO
        // =========================
        json += "\"accel\": {";
        json += "\"x\": " + String(getAccelX(), 3) + ",";
        json += "\"y\": " + String(getAccelY(), 3) + ",";
        json += "\"z\": " + String(getAccelZ(), 3);
        json += "},";

        // =========================
        // GIROSCÓPIO
        // =========================
        json += "\"gyro\": {";
        json += "\"x\": " + String(getGyroX()) + ",";
        json += "\"y\": " + String(getGyroY()) + ",";
        json += "\"z\": " + String(getGyroZ());
        json += "}";

        json += "}";

        server.send(200, "application/json", json);
    });
}

// =====================================================
// INICIALIZAÇÃO DO WIFI
// =====================================================

void initWiFi() {
    logInfo("WIFI: Iniciando modo Access Point...");

    // Define modo AP
    WiFi.mode(WIFI_AP);

    // Cria rede
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();

    logInfo("WIFI: Rede criada!");
    logInfo("WIFI: SSID: " + String(ssid));
    logInfo("WIFI: IP: " + String(IP));
    // -------------------------------------------------
    // Inicializa sistema de arquivos
    // -------------------------------------------------

    if (!montarLittleFS()) {
        logError("WIFI: Falha ao inicializar LittleFS");
        return;
    }

    logInfo("PS: Sistema de arquivos montado");

    // -------------------------------------------------
    // Configura servidor
    // -------------------------------------------------

    configurarRotas();
    delay(100); // garante estabilidade inicial
    server.begin();
    logInfo("SERVER: Servidor iniciado");
}

// =====================================================
// LOOP WIFI
// =====================================================

void atualizarWiFi() {
    server.handleClient();
}