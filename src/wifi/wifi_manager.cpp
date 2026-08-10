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
#include "sensores/sensor_manager.h"
#include "../robot/robot.h"
#include "../robot/robot_state.h"
#include "../testes/console/console.h"
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
        setInterfaceMode(UI_CONSOLE);
        enviarArquivoOuFallback(
            "/web/index.html",
            "text/html",
            "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Robo Vespa</title><style>body{font-family:Arial,sans-serif;background:#111;color:#fff;padding:24px}h1{margin-bottom:8px}p{color:#ccc}</style></head><body><h1>Robô Vespa</h1><p>Interface web pronta no firmware.</p></body></html>"
        );
    });

    server.on("/control", []() {
        logInfo("[HTTP] Cliente acessou /control");
        setInterfaceMode(UI_CONTROL);
        enviarArquivoOuFallback(
            "/web/control.html",
            "text/html",
            "<!DOCTYPE html><html><body><h1>Control</h1><p>control.html não encontrado.</p></body></html>"
        );
    });

    server.on("/monitor", []() {
        logInfo("[HTTP] Cliente acessou /monitor");
        setInterfaceMode(UI_MONITOR);
        enviarArquivoOuFallback(
            "/web/monitor.html",
            "text/html",
            "<!DOCTYPE html><html><body><h1>Monitor</h1><p>monitor.html não encontrado.</p></body></html>"
        );
    });

    server.on("/config", []() {
        logInfo("[HTTP] Cliente acessou /config");
        setInterfaceMode(UI_CONFIGURATION);
        enviarArquivoOuFallback(
            "/web/config.html",
            "text/html",
            "<!DOCTYPE html><html><body><h1>Config</h1><p>config.html não encontrado.</p></body></html>"
        );
    });

    // -------------------------------------------------
    // ROTA: Script JS
    // -------------------------------------------------
    server.on("/script.js", []() {
        enviarArquivoOuFallback(
            "/web/script.js",
            "application/javascript",
            "console.log('Script fallback carregado');"
        );
    });

    server.on("/control.js", []() {
        enviarArquivoOuFallback(
            "/web/control.js",
            "application/javascript",
            "console.log('control.js não encontrado');"
        );
    });

    server.on("/monitor.js", []() {
        enviarArquivoOuFallback(
            "/web/monitor.js",
            "application/javascript",
            "console.log('monitor.js não encontrado');"
        );
    });

    server.on("/config.js", []() {
        enviarArquivoOuFallback(
            "/web/config.js",
            "application/javascript",
            "console.log('config.js não encontrado');"
        );
    });

    // -------------------------------------------------
    // ROTA: Estilo CSS
    // -------------------------------------------------
    server.on("/style.css", []() {
        enviarArquivoOuFallback(
            "/web/style.css",
            "text/css",
            "body{font-family:Arial,sans-serif;text-align:center;}"
        );
    });


    // -------------------------------------------------
    // ROTA: CONTROLE (Joystick)
    // -------------------------------------------------
    server.on("/controle", []() {
        if (server.hasArg("cmd")) {
            String cmd = server.arg("cmd");
            cmd.trim();

            if (cmd.length() > 0) {
                console_submitCommand(cmd);
                Serial.print("[CMD] ");
                Serial.println(cmd);
            }
        }

        if (server.hasArg("x") && server.hasArg("y")) {
            float x = server.arg("x").toFloat();
            float y = server.arg("y").toFloat();

            Serial.print("[JOY] X: ");
            Serial.print(x);
            Serial.print(" | Y: ");
            Serial.println(y);

            setJoystick(x, y);
        }

        server.send(200, "text/plain", "OK");
    });

    server.on("/console-output", []() {
        String out = console_getAndClearWebBuffer();
        server.send(200, "text/plain", out);
    });


    // -------------------------------------------------
    // ROTA: STATUS DO ROBÔ (API)
    // -------------------------------------------------
    server.on("/status", []() {
        RobotState st = getRobotStateSnapshot();
        String estado = "Aguardando";
        float distancia = st.distance;
        UltraStats ultraStats = getUltraStats();

        if (distancia < 20.0f) {
            estado = "Obstáculo";
        } else if (st.mode == MODE_MANUAL) {
            estado = "Manual";
        } else if (st.mode == MODE_AUTONOMOUS) {
            estado = "Autônomo";
        } else if (st.mode == MODE_CALIBRATION) {
            estado = "Calibração";
        }

        String json = "{";
        json += "\"distancia\": " + String((int)distancia) + ",";
        json += "\"distance\": " + String(distancia, 2) + ",";
        json += "\"estado\": \"" + estado + "\",";
        json += "\"modo\": \"" + String(robotModeToString(st.mode)) + "\",";
        json += "\"ui\": \"" + String(interfaceModeToString(st.interfaceMode)) + "\",";
        json += "\"wifi\": \"" + String(ssid) + "\",";
        json += "\"encoder_esq\": " + String(st.encoderEsq) + ",";
        json += "\"encoder_dir\": " + String(st.encoderDir) + ",";
        json += "\"encoderLeft\": " + String(st.encoderLeft) + ",";
        json += "\"encoderRight\": " + String(st.encoderRight) + ",";
        json += "\"accel\": {";
        json += "\"x\": " + String(st.accelX, 3) + ",";
        json += "\"y\": " + String(st.accelY, 3) + ",";
        json += "\"z\": " + String(st.accelZ, 3);
        json += "},";
        json += "\"gyro\": {";
        json += "\"x\": " + String(st.gyroX, 3) + ",";
        json += "\"y\": " + String(st.gyroY, 3) + ",";
        json += "\"z\": " + String(st.gyroZ, 3);
        json += "},";
        json += "\"accelX\": " + String(st.accel[0], 3) + ",";
        json += "\"accelY\": " + String(st.accel[1], 3) + ",";
        json += "\"accelZ\": " + String(st.accel[2], 3) + ",";
        json += "\"gyroX\": " + String(st.gyro[0], 3) + ",";
        json += "\"gyroY\": " + String(st.gyro[1], 3) + ",";
        json += "\"gyroZ\": " + String(st.gyro[2], 3) + ",";
        json += "\"pwmLeft\": " + String(st.pwmLeft) + ",";
        json += "\"pwmRight\": " + String(st.pwmRight) + ",";
        json += "\"wifiConnected\": " + String(st.wifiConnected ? "true" : "false") + ",";
        json += "\"uptime\": " + String(st.uptime) + ",";
        json += "\"vel_esq\": " + String(st.velEsqCmd) + ",";
        json += "\"vel_dir\": " + String(st.velDirCmd) + ",";
        json += "\"uptime_ms\": " + String(st.uptimeMs) + ",";
        json += "\"wifi_clients\": " + String(st.wifiClients) + ",";
        json += "\"wifi_ip\": \"" + String(st.wifiIp) + "\",";
        json += "\"ultra\": {";
        json += "\"rawDistance\": " + String(getUltraRawDistanceCm(), 2) + ",";
        json += "\"filteredDistance\": " + String(getUltraFilteredDistanceCm(), 2) + ",";
        json += "\"valid\": " + String(isUltraDistanceValid() ? "true" : "false") + ",";
        json += "\"status\": \"" + String(ultraStatusToString(getUltraStatus())) + "\",";
        json += "\"filter\": \"" + String(ultraFilterModeToString(getUltraFilterMode())) + "\",";
        json += "\"calibration\": " + String(getUltraCalibrationFactor(), 6) + ",";
        json += "\"ageMs\": " + String(getUltraLastAgeMs()) + ",";
        json += "\"hz\": " + String(getUltraUpdateHz(), 1) + ",";
        json += "\"timestampMs\": " + String(getUltraLastUpdateMs()) + ",";
        json += "\"stats\": {";
        json += "\"reads\": " + String(ultraStats.reads) + ",";
        json += "\"validReads\": " + String(ultraStats.validReads) + ",";
        json += "\"timeouts\": " + String(ultraStats.timeouts) + ",";
        json += "\"outOfRange\": " + String(ultraStats.outOfRange) + ",";
        json += "\"echoShort\": " + String(ultraStats.echoShort) + ",";
        json += "\"echoLong\": " + String(ultraStats.echoLong) + ",";
        json += "\"sensorError\": " + String(ultraStats.sensorError) + ",";
        json += "\"invalidRead\": " + String(ultraStats.invalidRead);
        json += "}";
        json += "}";

        server.send(200, "application/json", json);
    });

    server.on("/state", []() {
        RobotState st = getRobotStateSnapshot();
        UltraStats ultraStats = getUltraStats();
        String json = "{";
        json += "\"mode\": \"" + String(robotModeToString(st.mode)) + "\",";
        json += "\"interfaceMode\": \"" + String(interfaceModeToString(st.interfaceMode)) + "\",";
        json += "\"pwmLeft\": " + String(st.pwmLeft) + ",";
        json += "\"pwmRight\": " + String(st.pwmRight) + ",";
        json += "\"encoderLeft\": " + String(st.encoderLeft) + ",";
        json += "\"encoderRight\": " + String(st.encoderRight) + ",";
        json += "\"distance\": " + String(st.distance, 2) + ",";
        json += "\"ultra\": {";
        json += "\"rawDistance\": " + String(getUltraRawDistanceCm(), 2) + ",";
        json += "\"filteredDistance\": " + String(getUltraFilteredDistanceCm(), 2) + ",";
        json += "\"valid\": " + String(isUltraDistanceValid() ? "true" : "false") + ",";
        json += "\"status\": \"" + String(ultraStatusToString(getUltraStatus())) + "\",";
        json += "\"filter\": \"" + String(ultraFilterModeToString(getUltraFilterMode())) + "\",";
        json += "\"calibration\": " + String(getUltraCalibrationFactor(), 6) + ",";
        json += "\"ageMs\": " + String(getUltraLastAgeMs()) + ",";
        json += "\"hz\": " + String(getUltraUpdateHz(), 1) + ",";
        json += "\"timestampMs\": " + String(getUltraLastUpdateMs()) + ",";
        json += "\"stats\": {";
        json += "\"reads\": " + String(ultraStats.reads) + ",";
        json += "\"validReads\": " + String(ultraStats.validReads) + ",";
        json += "\"timeouts\": " + String(ultraStats.timeouts) + ",";
        json += "\"outOfRange\": " + String(ultraStats.outOfRange) + ",";
        json += "\"echoShort\": " + String(ultraStats.echoShort) + ",";
        json += "\"echoLong\": " + String(ultraStats.echoLong) + ",";
        json += "\"sensorError\": " + String(ultraStats.sensorError) + ",";
        json += "\"invalidRead\": " + String(ultraStats.invalidRead);
        json += "}";
        json += "},";
        json += "\"accelX\": " + String(st.accel[0], 3) + ",";
        json += "\"accelY\": " + String(st.accel[1], 3) + ",";
        json += "\"accelZ\": " + String(st.accel[2], 3) + ",";
        json += "\"gyroX\": " + String(st.gyro[0], 3) + ",";
        json += "\"gyroY\": " + String(st.gyro[1], 3) + ",";
        json += "\"gyroZ\": " + String(st.gyro[2], 3) + ",";
        json += "\"wifiConnected\": " + String(st.wifiConnected ? "true" : "false") + ",";
        json += "\"uptime\": " + String(st.uptime) + ",";
        json += "\"wifiClients\": " + String(st.wifiClients) + ",";
        json += "\"wifiIp\": \"" + String(st.wifiIp) + "\"";
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
    robotStateSetWifi(IP.toString(), WiFi.softAPgetStationNum());

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
    robotStateSetWifi(WiFi.softAPIP().toString(), WiFi.softAPgetStationNum());
    server.handleClient();
}