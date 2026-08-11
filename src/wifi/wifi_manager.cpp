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
static File fsUploadFile;
static String fsUploadTargetPath;
static bool fsUploadTargetsLogger = false;
static bool openUploadTarget(File& outFile, const String& rawDestination, const String& uploadName, String& resolvedPath);

static void resetUploadState() {
    if (fsUploadFile) {
        fsUploadFile.close();
    }

    fsUploadFile = File();
    fsUploadTargetPath = "";
    fsUploadTargetsLogger = false;
}

static void handleFsUpload() {
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        if (!LittleFS.begin()) {
            resetUploadState();
            return;
        }

        resetUploadState();

        String rawDestination = server.hasArg("path") ? server.arg("path") : "/";
        fsUploadTargetsLogger = rawDestination == "/log.txt";
        if (fsUploadTargetsLogger) {
            closeLogger();
        }

        String uploadName = String(upload.filename);
        if (uploadName.length() == 0) {
            uploadName = "upload.bin";
        }

        String resolvedPath;
        if (!openUploadTarget(fsUploadFile, rawDestination, uploadName, resolvedPath)) {
            if (fsUploadTargetsLogger) {
                reopenLogger();
            }
            resetUploadState();
            return;
        }

        fsUploadTargetPath = resolvedPath;
        return;
    }

    if (upload.status == UPLOAD_FILE_WRITE) {
        if (fsUploadFile) {
            fsUploadFile.write(upload.buf, upload.currentSize);
        }
        return;
    }

    if (upload.status == UPLOAD_FILE_END) {
        if (fsUploadFile) {
            fsUploadFile.flush();
            fsUploadFile.close();
        }

        if (fsUploadTargetsLogger) {
            reopenLogger();
        }
        return;
    }

    if (upload.status == UPLOAD_FILE_ABORTED) {
        if (fsUploadFile) {
            fsUploadFile.close();
        }

        if (fsUploadTargetPath.length() > 0) {
            LittleFS.remove(fsUploadTargetPath.c_str());
        }

        if (fsUploadTargetsLogger) {
            reopenLogger();
        }
        resetUploadState();
    }
}

static String normalizeFsPath(String path) {
    path.trim();

    if (path.length() == 0) {
        return "/";
    }

    if (path[0] != '/') {
        path = "/" + path;
    }

    while (path.endsWith("/") && path.length() > 1) {
        path.remove(path.length() - 1);
    }

    return path;
}

static String buildChildPath(const String& parentPath, const String& childName) {
    String child = childName;
    child.trim();

    if (child.length() == 0) {
        return normalizeFsPath(parentPath);
    }

    if (child[0] == '/') {
        return normalizeFsPath(child);
    }

    String parent = normalizeFsPath(parentPath);
    if (parent == "/") {
        return normalizeFsPath("/" + child);
    }

    return normalizeFsPath(parent + "/" + child);
}

static String baseNameFromPath(const String& path) {
    String normalized = normalizeFsPath(path);
    int slash = normalized.lastIndexOf('/');
    if (slash < 0 || slash == normalized.length() - 1) {
        return normalized.length() == 0 ? String("file") : normalized;
    }

    return normalized.substring(slash + 1);
}

static String resolveFsTargetPath(const String& rawDestination, const String& fallbackName) {
    String raw = rawDestination;
    String trimmed = raw;
    trimmed.trim();

    if (trimmed.length() == 0) {
        return normalizeFsPath("/" + fallbackName);
    }

    bool wantsDirectory = raw.endsWith("/");
    String normalized = normalizeFsPath(trimmed);

    File node = LittleFS.open(normalized.c_str(), "r");
    bool isDirectory = node && node.isDirectory();
    if (node) {
        node.close();
    }

    if (wantsDirectory || isDirectory) {
        return buildChildPath(normalized, fallbackName);
    }

    return normalized;
}

static bool copyFsFile(const String& sourcePath, const String& destinationPath) {
    File source = LittleFS.open(sourcePath.c_str(), "r");
    if (!source || source.isDirectory()) {
        if (source) {
            source.close();
        }
        return false;
    }

    if (LittleFS.exists(destinationPath.c_str())) {
        File existing = LittleFS.open(destinationPath.c_str(), "r");
        if (existing && existing.isDirectory()) {
            existing.close();
            source.close();
            return false;
        }
        if (existing) {
            existing.close();
        }
        LittleFS.remove(destinationPath.c_str());
    }

    File destination = LittleFS.open(destinationPath.c_str(), "w");
    if (!destination) {
        source.close();
        return false;
    }

    uint8_t buffer[256];
    while (source.available()) {
        size_t readBytes = source.read(buffer, sizeof(buffer));
        if (readBytes == 0) {
            break;
        }

        size_t written = destination.write(buffer, readBytes);
        if (written != readBytes) {
            destination.close();
            source.close();
            LittleFS.remove(destinationPath.c_str());
            return false;
        }
    }

    destination.flush();
    destination.close();
    source.close();
    return true;
}

static bool moveFsFile(const String& sourcePath, const String& destinationPath) {
    if (sourcePath == destinationPath) {
        return true;
    }

    if (!copyFsFile(sourcePath, destinationPath)) {
        return false;
    }

    return LittleFS.remove(sourcePath.c_str());
}

static void streamFsDownload(const String& path) {
    File file = LittleFS.open(path.c_str(), "r");
    if (!file) {
        server.send(404, "text/plain", "Arquivo nao encontrado");
        return;
    }

    if (file.isDirectory()) {
        file.close();
        server.send(400, "text/plain", "Nao e possivel baixar diretorios");
        return;
    }

    String filename = baseNameFromPath(path);
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    server.sendHeader("Cache-Control", "no-store");
    server.streamFile(file, "application/octet-stream");
    file.close();
}

static bool openUploadTarget(File& outFile, const String& rawDestination, const String& uploadName, String& resolvedPath) {
    resolvedPath = resolveFsTargetPath(rawDestination, uploadName);

    if (resolvedPath == "/") {
        resolvedPath = buildChildPath("/", uploadName);
    }

    File existing = LittleFS.open(resolvedPath.c_str(), "r");
    if (existing && existing.isDirectory()) {
        existing.close();
        return false;
    }
    if (existing) {
        existing.close();
    }

    outFile = LittleFS.open(resolvedPath.c_str(), "w");
    return (bool)outFile;
}

static void sendJsonString(const String& json) {
    server.send(200, "application/json", json);
}

static String escapeJson(const String& value) {
    String out;
    out.reserve(value.length() + 8);

    for (size_t i = 0; i < value.length(); ++i) {
        char c = value[i];
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }

    return out;
}

static void responderFsApi() {
    String action = server.hasArg("action") ? server.arg("action") : "";
    action.trim();
    action.toLowerCase();

    String rawPath = server.hasArg("path") ? server.arg("path") : "/";
    String path = normalizeFsPath(rawPath);

    if (!LittleFS.begin()) {
        server.send(500, "application/json", "{\"ok\":false,\"error\":\"LittleFS indisponivel\"}");
        return;
    }

    if (action == "list") {
        File node = LittleFS.open(path.c_str(), "r");
        if (!node) {
            server.send(404, "application/json", "{\"ok\":false,\"error\":\"Caminho nao encontrado\"}");
            return;
        }

        if (!node.isDirectory()) {
            String json = "{\"ok\":true,\"path\":\"" + escapeJson(path) + "\",\"type\":\"file\",\"size\":" + String((unsigned long)node.size()) + ",\"items\":[]}";
            node.close();
            sendJsonString(json);
            return;
        }

        String json = "{\"ok\":true,\"path\":\"" + escapeJson(path) + "\",\"type\":\"directory\",\"items\":[";
        bool first = true;
        File entry = node.openNextFile();

        while (entry) {
            if (!first) {
                json += ",";
            }

            String childPath = buildChildPath(path, String(entry.name()));
            json += "{\"name\":\"" + escapeJson(String(entry.name())) + "\",";
            json += "\"path\":\"" + escapeJson(childPath) + "\",";
            json += "\"type\":\"" + String(entry.isDirectory() ? "directory" : "file") + "\",";
            json += "\"size\":" + String((unsigned long)entry.size()) + "}";

            first = false;
            entry.close();
            entry = node.openNextFile();
        }

        json += "]}";
        node.close();
        sendJsonString(json);
        return;
    }

    if (action == "read") {
        File file = LittleFS.open(path.c_str(), "r");
        if (!file) {
            server.send(404, "text/plain", "Arquivo nao encontrado");
            return;
        }

        if (file.isDirectory()) {
            file.close();
            server.send(400, "text/plain", "O caminho aponta para um diretorio");
            return;
        }

        server.streamFile(file, "text/plain");
        file.close();
        return;
    }

    if (action == "download") {
        streamFsDownload(path);
        return;
    }

    if (action == "delete") {
        bool confirm = server.hasArg("confirm") && server.arg("confirm") == "1";
        if (!confirm) {
            server.send(400, "application/json", "{\"ok\":false,\"error\":\"Confirme com confirm=1\"}");
            return;
        }

        File file = LittleFS.open(path.c_str(), "r");
        if (!file) {
            server.send(404, "application/json", "{\"ok\":false,\"error\":\"Arquivo nao encontrado\"}");
            return;
        }

        if (file.isDirectory()) {
            file.close();
            server.send(400, "application/json", "{\"ok\":false,\"error\":\"Nao e permitido remover diretorios\"}");
            return;
        }

        file.close();

        if (path == "/log.txt") {
            closeLogger();
        }

        if (!LittleFS.remove(path.c_str())) {
            if (path == "/log.txt") {
                reopenLogger();
            }
            server.send(500, "application/json", "{\"ok\":false,\"error\":\"Falha ao remover\"}");
            return;
        }

        if (path == "/log.txt") {
            reopenLogger();
        }

        server.send(200, "application/json", "{\"ok\":true}");
        return;
    }

    if (action == "copy" || action == "move") {
        String rawSource = server.hasArg("source") ? server.arg("source") : "";
        String rawDest = server.hasArg("dest") ? server.arg("dest") : "";

        String sourcePath = normalizeFsPath(rawSource);
        if (sourcePath.length() == 0 || sourcePath == "/") {
            server.send(400, "application/json", "{\"ok\":false,\"error\":\"Fonte invalida\"}");
            return;
        }

        File source = LittleFS.open(sourcePath.c_str(), "r");
        if (!source) {
            server.send(404, "application/json", "{\"ok\":false,\"error\":\"Arquivo de origem nao encontrado\"}");
            return;
        }

        if (source.isDirectory()) {
            source.close();
            server.send(400, "application/json", "{\"ok\":false,\"error\":\"Copiar/mover diretorios nao esta suportado\"}");
            return;
        }

        source.close();

        String destPath = resolveFsTargetPath(rawDest, baseNameFromPath(sourcePath));
        if (destPath.length() == 0 || destPath == "/") {
            server.send(400, "application/json", "{\"ok\":false,\"error\":\"Destino invalido\"}");
            return;
        }

        bool sourceIsLogger = sourcePath == "/log.txt";
        bool destIsLogger = destPath == "/log.txt";
        if (sourceIsLogger || destIsLogger) {
            closeLogger();
        }

        bool ok = (action == "copy")
            ? copyFsFile(sourcePath, destPath)
            : moveFsFile(sourcePath, destPath);

        if (sourceIsLogger || destIsLogger) {
            reopenLogger();
        }

        if (!ok) {
            server.send(500, "application/json", "{\"ok\":false,\"error\":\"Falha ao executar operacao\"}");
            return;
        }

        String json = "{\"ok\":true,\"source\":\"" + escapeJson(sourcePath) + "\",\"dest\":\"" + escapeJson(destPath) + "\"}";
        sendJsonString(json);
        return;
    }

    server.send(400, "application/json", "{\"ok\":false,\"error\":\"action invalida\"}");
}

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

    server.on("/fs", []() {
        logInfo("[HTTP] Cliente acessou /fs");
        enviarArquivoOuFallback(
            "/web/fs.html",
            "text/html",
            "<!DOCTYPE html><html><body><h1>FS</h1><p>fs.html não encontrado.</p></body></html>"
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

    server.on("/fs.js", []() {
        enviarArquivoOuFallback(
            "/web/fs.js",
            "application/javascript",
            "console.log('fs.js não encontrado');"
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

    server.on("/fs-api", []() {
        responderFsApi();
    });

    server.on(
        "/fs-upload",
        HTTP_POST,
        []() {
            if (fsUploadFile) {
                fsUploadFile.flush();
                fsUploadFile.close();
            }

            if (fsUploadTargetsLogger) {
                reopenLogger();
            }

            String response = "{\"ok\":true,\"path\":\"" + escapeJson(fsUploadTargetPath) + "\"}";
            resetUploadState();
            server.send(200, "application/json", response);
        },
        handleFsUpload
    );


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