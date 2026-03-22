#include "wifi_manager.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

// =========================
// CONFIGURAÇÕES DA REDE
// =========================
const char* ssid = "ROBO_VESPA";
const char* password = "12345678"; // mínimo 8 caracteres

// =========================
// SERVIDOR WEB
// =========================
WebServer server(80);

// =========================
// CONFIGURA ROTAS
// =========================
void configurarRotas() {

    server.on("/", []() {
        File file = LittleFS.open("/index.html", "r");

        if (!file) {
            server.send(500, "text/plain", "Erro ao abrir HTML");
            return;
        }

        server.on("/", []() {
            Serial.println("[HTTP] Cliente acessou /");

            File file = LittleFS.open("/index.html", "r");

            if (!file) {
                Serial.println("[ERRO] Arquivo não encontrado!");
                server.send(500, "text/plain", "Erro ao abrir HTML");
                return;
            }

            server.streamFile(file, "text/html");
            file.close();
        });

        server.streamFile(file, "text/html");
        file.close();
    });

    server.on("/frente", []() {
        Serial.println("[CMD] Frente");
        server.send(200, "text/plain", "OK");
    });

    server.on("/parar", []() {
        Serial.println("[CMD] Parar");
        server.send(200, "text/plain", "OK");
    });
}

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

    // Inicializa sistema de arquivos
    if (!LittleFS.begin()) {
        Serial.println("[ERRO] Falha ao montar LittleFS");
        return;
    }

    Serial.println("[FS] Sistema de arquivos montado");

    configurarRotas();
    server.begin();

    Serial.println("[SERVER] Servidor iniciado");
}

// =========================
// LOOP WIFI (futuro)
// =========================
void atualizarWiFi() {
    server.handleClient();
}