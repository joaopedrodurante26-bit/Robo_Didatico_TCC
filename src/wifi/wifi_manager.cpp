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

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

// =====================================================
// CONFIGURAÇÕES DA REDE
// =====================================================

const char* ssid = "ROBO_VESPA";
const char* password = "12345678"; // mínimo 8 caracteres

// =====================================================
// SERVIDOR WEB
// =====================================================

static WebServer server(80);

// =====================================================
// ROTAS HTTP
// =====================================================

static void configurarRotas() {
    // -------------------------------------------------
    // ROTA: Página principal
    // -------------------------------------------------
    server.on("/", []() {
        Serial.println("[HTTP] Cliente acessou /");

        File file = LittleFS.open("/index.html", "r");

        if (!file) {
            Serial.println("[ERRO] index.html não encontrado!");
            server.send(500, "text/plain", "Erro ao abrir HTML");
            return;
        }

        server.streamFile(file, "text/html");
        file.close();
    });


    // -------------------------------------------------
    // ROTA: Script JS
    // -------------------------------------------------
    server.on("/script.js", []() {
        File file = LittleFS.open("/script.js", "r");

        if (!file) {
            Serial.println("[ERRO] script.js não encontrado!");
            server.send(500, "text/plain", "Erro ao abrir JS");
            return;
        }

        server.streamFile(file, "application/javascript");
        file.close();
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
    Serial.println("[WIFI] Iniciando modo Access Point...");

    // Define modo AP
    WiFi.mode(WIFI_AP);

    // Cria rede
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();

    Serial.println("[WIFI] Rede criada!");
    Serial.print("[WIFI] SSID: ");
    Serial.println(ssid);
    Serial.print("[WIFI] IP: ");
    Serial.println(IP);

    // -------------------------------------------------
    // Inicializa sistema de arquivos
    // -------------------------------------------------

    if (!LittleFS.begin()) {
        Serial.println("[ERRO] Falha ao montar LittleFS");
        return;
    }

    Serial.println("[FS] Sistema de arquivos montado");

    // -------------------------------------------------
    // Configura servidor
    // -------------------------------------------------

    configurarRotas();
    delay(100); // garante estabilidade inicial
    server.begin();
    Serial.println("[SERVER] Servidor iniciado");
}

// =====================================================
// LOOP WIFI
// =====================================================

void atualizarWiFi() {
    server.handleClient();
}