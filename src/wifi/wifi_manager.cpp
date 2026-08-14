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

static const unsigned long WIFI_HEALTH_CHECK_MS = 2000;
static const unsigned long WIFI_RECOVERY_COOLDOWN_MS = 5000;
static const uint8_t WIFI_FAIL_STREAK_FOR_RECOVERY = 3;

static bool wifiApHealthy = false;
static bool wifiRecoveryInProgress = false;
static uint8_t wifiFailStreak = 0;
static unsigned long lastWifiHealthCheckMs = 0;
static unsigned long lastWifiRecoveryMs = 0;
static uint32_t wifiRecoveryCount = 0;
static uint32_t wifiRecoveryAttempts = 0;
static uint32_t wifiHealthChecks = 0;
static uint32_t wifiHealthCheckFailures = 0;
static unsigned long wifiLastHealthyMs = 0;
static unsigned long wifiLastUnhealthyMs = 0;
static unsigned long wifiLastRecoveryAttemptMs = 0;
static unsigned long wifiLastRecoverySuccessMs = 0;
static unsigned long wifiLastApStartAttemptMs = 0;
static unsigned long wifiLastApStartSuccessMs = 0;
static unsigned long wifiLastApStartFailureMs = 0;
static String wifiLastFault = "startup_pending";

// =====================================================
// SERVIDOR WEB
// =====================================================

static WebServer server(80);
static File fsUploadFile;
static String fsUploadTargetPath;
static bool fsUploadTargetsLogger = false;
static bool fsUploadHasError = false;
static String fsUploadErrorMessage;
static size_t fsUploadExpectedSize = 0;
static size_t fsUploadBytesWritten = 0;
static const size_t FS_RESERVED_FREE_BYTES = 1024;
static String normalizeFsPath(String path);
static bool openUploadTarget(File& outFile, const String& rawDestination, const String& uploadName, size_t uploadSize, String& resolvedPath, String& errorMessage);
static String escapeJson(const String& value);

static bool isForbiddenFsSegment(const String& segment) {
    return segment == "." || segment == "..";
}

static bool isProtectedFsPath(const String& path) {
    return path == "/boot_log.json"
        || path == "/ultra_config.json"
        || path.startsWith("/web")
        || path.startsWith("/help");
}

static bool validateFsPath(const String& rawPath, String& normalizedPath, String& errorMessage) {
    normalizedPath = normalizeFsPath(rawPath);

    if (normalizedPath.indexOf('\\') >= 0) {
        errorMessage = "Caminho invalido";
        return false;
    }

    int start = 1;
    while (start < normalizedPath.length()) {
        int slash = normalizedPath.indexOf('/', start);
        String segment = slash >= 0
            ? normalizedPath.substring(start, slash)
            : normalizedPath.substring(start);

        if (isForbiddenFsSegment(segment)) {
            errorMessage = "Caminho invalido";
            return false;
        }

        if (slash < 0) {
            break;
        }

        start = slash + 1;
    }

    return true;
}

static bool ensureWritableFsPath(const String& path, String& errorMessage) {
    if (isProtectedFsPath(path)) {
        errorMessage = "Caminho protegido contra escrita";
        return false;
    }

    return true;
}

static void resetUploadError() {
    fsUploadHasError = false;
    fsUploadErrorMessage = "";
    fsUploadExpectedSize = 0;
    fsUploadBytesWritten = 0;
}

static void failUpload(const String& errorMessage) {
    fsUploadHasError = true;
    fsUploadErrorMessage = errorMessage;
}

static void getFsUsage(size_t& totalBytes, size_t& usedBytes, size_t& freeBytes) {
    totalBytes = LittleFS.totalBytes();
    usedBytes = LittleFS.usedBytes();
    freeBytes = totalBytes > usedBytes ? (totalBytes - usedBytes) : 0;
}

static bool hasFsHeadroom(size_t bytesToWrite, size_t bytesFreedFirst, String& errorMessage) {
    size_t totalBytes = 0;
    size_t usedBytes = 0;
    size_t freeBytes = 0;
    getFsUsage(totalBytes, usedBytes, freeBytes);

    size_t effectiveFree = freeBytes + bytesFreedFirst;
    if (effectiveFree <= FS_RESERVED_FREE_BYTES || bytesToWrite > (effectiveFree - FS_RESERVED_FREE_BYTES)) {
        size_t safeWritable = effectiveFree > FS_RESERVED_FREE_BYTES ? (effectiveFree - FS_RESERVED_FREE_BYTES) : 0;
        errorMessage = "Espaco insuficiente no LittleFS. Livre=" + String((unsigned long)freeBytes)
            + " bytes, reserva=" + String((unsigned long)FS_RESERVED_FREE_BYTES)
            + " bytes, maximo seguro=" + String((unsigned long)safeWritable) + " bytes.";
        return false;
    }

    return true;
}

static size_t fileSizeIfRegular(const String& path) {
    File file = LittleFS.open(path.c_str(), "r");
    if (!file) {
        return 0;
    }

    if (file.isDirectory()) {
        file.close();
        return 0;
    }

    size_t size = file.size();
    file.close();
    return size;
}

static String buildFsStatsJson() {
    size_t totalBytes = 0;
    size_t usedBytes = 0;
    size_t freeBytes = 0;
    getFsUsage(totalBytes, usedBytes, freeBytes);

    size_t safeFreeBytes = freeBytes > FS_RESERVED_FREE_BYTES ? (freeBytes - FS_RESERVED_FREE_BYTES) : 0;

    String json = "\"stats\":{";
    json += "\"totalBytes\":" + String((unsigned long)totalBytes) + ",";
    json += "\"usedBytes\":" + String((unsigned long)usedBytes) + ",";
    json += "\"freeBytes\":" + String((unsigned long)freeBytes) + ",";
    json += "\"reservedBytes\":" + String((unsigned long)FS_RESERVED_FREE_BYTES) + ",";
    json += "\"safeFreeBytes\":" + String((unsigned long)safeFreeBytes);
    json += "}";
    return json;
}

static String buildFsProtectionJson(const String& path) {
    return String("\"protected\":") + (isProtectedFsPath(path) ? "true" : "false");
}

static const char* wifiModeToString(wifi_mode_t mode) {
    switch (mode) {
        case WIFI_MODE_NULL: return "null";
        case WIFI_MODE_STA: return "sta";
        case WIFI_MODE_AP: return "ap";
        case WIFI_MODE_APSTA: return "apsta";
        default: return "unknown";
    }
}

static const char* wifiHealthToString() {
    if (wifiRecoveryInProgress) {
        return "recovering";
    }

    if (wifiApHealthy) {
        return "healthy";
    }

    if (wifiFailStreak >= WIFI_FAIL_STREAK_FOR_RECOVERY) {
        return "down";
    }

    return "unstable";
}

static String buildWifiDiagnosticsJson() {
    unsigned long now = millis();
    IPAddress ip = WiFi.softAPIP();
    int clients = WiFi.softAPgetStationNum();

    String json = "{";
    json += "\"health\":\"" + String(wifiHealthToString()) + "\",";
    json += "\"apHealthy\":" + String(wifiApHealthy ? "true" : "false") + ",";
    json += "\"recoveryInProgress\":" + String(wifiRecoveryInProgress ? "true" : "false") + ",";
    json += "\"failStreak\":" + String(wifiFailStreak) + ",";
    json += "\"checks\":" + String(wifiHealthChecks) + ",";
    json += "\"checkFailures\":" + String(wifiHealthCheckFailures) + ",";
    json += "\"recoveryAttempts\":" + String(wifiRecoveryAttempts) + ",";
    json += "\"recoveries\":" + String(wifiRecoveryCount) + ",";
    json += "\"mode\":\"" + String(wifiModeToString(WiFi.getMode())) + "\",";
    json += "\"ip\":\"" + ip.toString() + "\",";
    json += "\"clients\":" + String(clients) + ",";
    json += "\"lastFault\":\"" + escapeJson(wifiLastFault) + "\",";
    json += "\"lastHealthyMs\":" + String(wifiLastHealthyMs) + ",";
    json += "\"lastUnhealthyMs\":" + String(wifiLastUnhealthyMs) + ",";
    json += "\"lastRecoveryAttemptMs\":" + String(wifiLastRecoveryAttemptMs) + ",";
    json += "\"lastRecoverySuccessMs\":" + String(wifiLastRecoverySuccessMs) + ",";
    json += "\"lastApStartAttemptMs\":" + String(wifiLastApStartAttemptMs) + ",";
    json += "\"lastApStartSuccessMs\":" + String(wifiLastApStartSuccessMs) + ",";
    json += "\"lastApStartFailureMs\":" + String(wifiLastApStartFailureMs) + ",";
    json += "\"msSinceLastHealthy\":" + String((wifiLastHealthyMs > 0) ? (now - wifiLastHealthyMs) : 0) + ",";
    json += "\"msSinceLastUnhealthy\":" + String((wifiLastUnhealthyMs > 0) ? (now - wifiLastUnhealthyMs) : 0);
    json += "}";

    return json;
}

static void resetWifiDiagnosticsState() {
    wifiFailStreak = 0;
    wifiRecoveryCount = 0;
    wifiRecoveryAttempts = 0;
    wifiHealthChecks = 0;
    wifiHealthCheckFailures = 0;
    wifiLastHealthyMs = 0;
    wifiLastUnhealthyMs = 0;
    wifiLastRecoveryAttemptMs = 0;
    wifiLastRecoverySuccessMs = 0;
    wifiLastApStartAttemptMs = 0;
    wifiLastApStartSuccessMs = 0;
    wifiLastApStartFailureMs = 0;
    wifiLastFault = "startup_pending";
}

static bool isAccessPointHealthy() {
    wifi_mode_t mode = WiFi.getMode();
    if (mode != WIFI_MODE_AP && mode != WIFI_MODE_APSTA) {
        return false;
    }

    IPAddress ip = WiFi.softAPIP();
    if (ip == IPAddress((uint32_t)0)) {
        return false;
    }

    String apSsid = WiFi.softAPSSID();
    apSsid.trim();
    if (apSsid.length() == 0) {
        return false;
    }

    return true;
}

static bool iniciarAccessPointComValidacao(uint8_t tentativas, bool logTentativas) {
    for (uint8_t i = 0; i < tentativas; ++i) {
        wifiLastApStartAttemptMs = millis();
        WiFi.softAPdisconnect(true);
        delay(120);

        WiFi.mode(WIFI_AP);
        delay(60);

        bool softApOk = WiFi.softAP(ssid, password);
        delay(300);

        bool apOk = softApOk && isAccessPointHealthy();
        if (apOk) {
            IPAddress ip = WiFi.softAPIP();
            robotStateSetWifi(ip.toString(), WiFi.softAPgetStationNum());
            wifiApHealthy = true;
            wifiFailStreak = 0;
            wifiLastHealthyMs = millis();
            wifiLastApStartSuccessMs = wifiLastHealthyMs;
            wifiLastFault = "none";
            logInfo("WIFI: AP ativo. SSID: " + String(ssid) + " | IP: " + ip.toString());
            return true;
        }

        wifiLastApStartFailureMs = millis();
        wifiLastUnhealthyMs = wifiLastApStartFailureMs;
        wifiLastFault = softApOk ? "ap_state_invalid_after_softap" : "softap_returned_false";

        if (logTentativas) {
            logWarn(
                "WIFI: Falha ao criar AP (tentativa " + String(i + 1) + "/" + String(tentativas) +
                "). softAP=" + String(softApOk ? "OK" : "FAIL") +
                " IP=" + WiFi.softAPIP().toString()
            );
        }
    }

    wifiApHealthy = false;
    robotStateSetWifi("0.0.0.0", 0);
    return false;
}

static void resetUploadState() {
    if (fsUploadFile) {
        fsUploadFile.close();
    }

    fsUploadFile = File();
    fsUploadTargetPath = "";
    fsUploadTargetsLogger = false;
    resetUploadError();
}

static void handleFsUpload() {
    HTTPUpload& upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        if (!LittleFS.begin()) {
            resetUploadState();
            failUpload("LittleFS indisponivel");
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
        String errorMessage;
        fsUploadExpectedSize = upload.totalSize;
        if (!openUploadTarget(fsUploadFile, rawDestination, uploadName, upload.totalSize, resolvedPath, errorMessage)) {
            if (fsUploadTargetsLogger) {
                reopenLogger();
            }
            if (errorMessage.length() == 0) {
                errorMessage = "Falha ao abrir destino do upload";
            }
            resetUploadState();
            failUpload(errorMessage);
            return;
        }

        fsUploadTargetPath = resolvedPath;
        return;
    }

    if (upload.status == UPLOAD_FILE_WRITE) {
        if (fsUploadFile) {
            size_t written = fsUploadFile.write(upload.buf, upload.currentSize);
            fsUploadBytesWritten += written;
            if (written != upload.currentSize) {
                fsUploadFile.close();
                if (fsUploadTargetPath.length() > 0) {
                    LittleFS.remove(fsUploadTargetPath.c_str());
                }
                failUpload("Falha ao gravar arquivo no LittleFS. Verifique o espaco disponivel.");
                fsUploadFile = File();
            }
        }
        return;
    }

    if (upload.status == UPLOAD_FILE_END) {
        if (fsUploadFile) {
            fsUploadFile.flush();
            fsUploadFile.close();
        }

        if (!fsUploadHasError && fsUploadExpectedSize > 0 && fsUploadBytesWritten != fsUploadExpectedSize) {
            if (fsUploadTargetPath.length() > 0) {
                LittleFS.remove(fsUploadTargetPath.c_str());
            }
            failUpload("Upload incompleto. O arquivo foi removido para evitar corrupcao.");
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
    String errorMessage;
    String normalized;
    if (!validateFsPath(trimmed, normalized, errorMessage)) {
        return "";
    }

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

    size_t destinationBytesFreed = 0;
    if (LittleFS.exists(destinationPath.c_str())) {
        File existing = LittleFS.open(destinationPath.c_str(), "r");
        if (existing && existing.isDirectory()) {
            existing.close();
            source.close();
            return false;
        }
        if (existing) {
            destinationBytesFreed = existing.size();
        }
        if (existing) {
            existing.close();
        }
        LittleFS.remove(destinationPath.c_str());
    }

    String capacityError;
    if (!hasFsHeadroom(source.size(), destinationBytesFreed, capacityError)) {
        source.close();
        return false;
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

static bool openUploadTarget(File& outFile, const String& rawDestination, const String& uploadName, size_t uploadSize, String& resolvedPath, String& errorMessage) {
    resolvedPath = resolveFsTargetPath(rawDestination, uploadName);

    if (resolvedPath.length() == 0) {
        errorMessage = "Destino invalido";
        return false;
    }

    if (resolvedPath == "/") {
        resolvedPath = buildChildPath("/", uploadName);
    }

    if (!ensureWritableFsPath(resolvedPath, errorMessage)) {
        return false;
    }

    size_t destinationBytesFreed = 0;
    File existing = LittleFS.open(resolvedPath.c_str(), "r");
    if (existing && existing.isDirectory()) {
        existing.close();
        errorMessage = "Destino aponta para um diretorio";
        return false;
    }
    if (existing) {
        destinationBytesFreed = existing.size();
        existing.close();
    }

    if (uploadSize > 0 && !hasFsHeadroom(uploadSize, destinationBytesFreed, errorMessage)) {
        return false;
    }

    outFile = LittleFS.open(resolvedPath.c_str(), "w");
    if (!outFile && errorMessage.length() == 0) {
        errorMessage = "Nao foi possivel abrir o arquivo de destino";
    }
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
    String path;
    String pathError;
    if (!validateFsPath(rawPath, path, pathError)) {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"" + escapeJson(pathError) + "\"}");
        return;
    }

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
            String json = "{\"ok\":true,\"path\":\"" + escapeJson(path) + "\",\"type\":\"file\",\"size\":" + String((unsigned long)node.size()) + ",\"items\":[]," + buildFsStatsJson() + "," + buildFsProtectionJson(path) + "}";
            node.close();
            sendJsonString(json);
            return;
        }

        String json = "{\"ok\":true,\"path\":\"" + escapeJson(path) + "\",\"type\":\"directory\"," + buildFsStatsJson() + ",\"items\":[";
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
            json += "\"size\":" + String((unsigned long)entry.size()) + ",";
            json += buildFsProtectionJson(childPath) + "}";

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

        String protectError;
        if (!ensureWritableFsPath(path, protectError)) {
            server.send(403, "application/json", "{\"ok\":false,\"error\":\"" + escapeJson(protectError) + "\"}");
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

        String sourcePath;
        String sourceError;
        if (!validateFsPath(rawSource, sourcePath, sourceError)) {
            server.send(400, "application/json", "{\"ok\":false,\"error\":\"" + escapeJson(sourceError) + "\"}");
            return;
        }

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

        size_t sourceSize = source.size();

        source.close();

        String destPath = resolveFsTargetPath(rawDest, baseNameFromPath(sourcePath));
        if (destPath.length() == 0 || destPath == "/") {
            server.send(400, "application/json", "{\"ok\":false,\"error\":\"Destino invalido\"}");
            return;
        }

        String protectError;
        if (!ensureWritableFsPath(destPath, protectError)) {
            server.send(403, "application/json", "{\"ok\":false,\"error\":\"" + escapeJson(protectError) + "\"}");
            return;
        }

        if (action == "move" && isProtectedFsPath(sourcePath)) {
            server.send(403, "application/json", "{\"ok\":false,\"error\":\"Caminho protegido contra escrita\"}");
            return;
        }

        size_t destinationBytesFreed = fileSizeIfRegular(destPath);
        String capacityError;
        if (sourcePath != destPath && !hasFsHeadroom(sourceSize, destinationBytesFreed, capacityError)) {
            server.send(400, "application/json", "{\"ok\":false,\"error\":\"" + escapeJson(capacityError) + "\"}");
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

            if (fsUploadHasError) {
                String errorResponse = "{\"ok\":false,\"error\":\"" + escapeJson(fsUploadErrorMessage) + "\"}";
                resetUploadState();
                server.send(400, "application/json", errorResponse);
                return;
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
        json += "\"wifi_diag\": " + buildWifiDiagnosticsJson() + ",";
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
        json += "\"wifiIp\": \"" + String(st.wifiIp) + "\",";
        json += "\"wifiDiag\": " + buildWifiDiagnosticsJson();
        json += "}";
        server.send(200, "application/json", json);
    });

    server.on("/wifi-diagnostics", []() {
        String json = "{";
        json += "\"ok\":true,";
        json += "\"wifiDiag\":" + buildWifiDiagnosticsJson();
        json += "}";
        server.send(200, "application/json", json);
    });
}

// =====================================================
// INICIALIZAÇÃO DO WIFI
// =====================================================

void initWiFi() {
    logInfo("WIFI: Iniciando modo Access Point...");

    resetWifiDiagnosticsState();

    bool apOk = iniciarAccessPointComValidacao(3, true);
    if (!apOk) {
        wifiLastFault = "init_ap_failed";
        logError("WIFI: AP nao iniciou com sucesso. Sistema seguira tentando recuperar no loop.");
    }

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
    unsigned long now = millis();

    if (now - lastWifiHealthCheckMs >= WIFI_HEALTH_CHECK_MS) {
        lastWifiHealthCheckMs = now;
        wifiHealthChecks++;

        bool apOk = isAccessPointHealthy();

        if (apOk) {
            if (!wifiApHealthy) {
                logInfo("WIFI: AP voltou a ficar estavel.");
            }

            wifiApHealthy = true;
            wifiFailStreak = 0;
            wifiLastHealthyMs = now;
            wifiLastFault = "none";
            robotStateSetWifi(WiFi.softAPIP().toString(), WiFi.softAPgetStationNum());
        } else {
            wifiApHealthy = false;
            wifiFailStreak++;
            wifiHealthCheckFailures++;
            wifiLastUnhealthyMs = now;
            wifiLastFault = "health_check_failed";
            robotStateSetWifi("0.0.0.0", 0);

            if (wifiFailStreak == 1 || (wifiFailStreak % 5) == 0) {
                logWarn(
                    "WIFI: AP instavel/indisponivel (falhas consecutivas=" + String(wifiFailStreak) +
                    "). IP atual=" + WiFi.softAPIP().toString()
                );
            }

            bool podeRecuperar = wifiFailStreak >= WIFI_FAIL_STREAK_FOR_RECOVERY;
            bool cooldownOk = (now - lastWifiRecoveryMs) >= WIFI_RECOVERY_COOLDOWN_MS;

            if (podeRecuperar && cooldownOk) {
                lastWifiRecoveryMs = now;
                wifiRecoveryAttempts++;
                wifiRecoveryInProgress = true;
                wifiLastRecoveryAttemptMs = now;
                logWarn("WIFI: Tentando recuperar AP...");

                bool recovered = iniciarAccessPointComValidacao(2, true);
                wifiRecoveryInProgress = false;
                if (recovered) {
                    wifiRecoveryCount++;
                    wifiLastRecoverySuccessMs = millis();
                    logInfo("WIFI: AP recuperado automaticamente (total de recuperacoes=" + String(wifiRecoveryCount) + ").");
                } else {
                    wifiLastFault = "recovery_failed";
                    logError("WIFI: Recuperacao do AP falhou. Nova tentativa apos cooldown.");
                }
            }
        }
    }

    server.handleClient();
}

bool wifiRecoverNow() {
    wifiRecoveryAttempts++;
    wifiRecoveryInProgress = true;
    wifiLastRecoveryAttemptMs = millis();

    bool recovered = iniciarAccessPointComValidacao(3, true);

    wifiRecoveryInProgress = false;
    lastWifiRecoveryMs = millis();

    if (recovered) {
        wifiRecoveryCount++;
        wifiLastRecoverySuccessMs = millis();
        wifiLastFault = "none";
        return true;
    }

    wifiLastFault = "manual_recovery_failed";
    return false;
}

void wifiResetDiagnostics() {
    bool keepHealthy = isAccessPointHealthy();
    resetWifiDiagnosticsState();
    wifiApHealthy = keepHealthy;
    wifiRecoveryInProgress = false;

    if (keepHealthy) {
        wifiLastHealthyMs = millis();
        wifiLastFault = "none";
    }
}

bool wifiIsApHealthy() {
    return isAccessPointHealthy();
}

String wifiGetHealthLabel() {
    return String(wifiHealthToString());
}

String wifiGetDiagnosticsJson() {
    return buildWifiDiagnosticsJson();
}