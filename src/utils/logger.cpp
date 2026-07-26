#include "logger.h"
#include <LittleFS.h>

static File logFile;

// =========================
// INICIALIZAÇÃO
// =========================

void initLogger() {

    if (!LittleFS.begin()) {
        Serial.println("[LOGGER] Falha no LittleFS");
        return;
    }

    logFile = LittleFS.open("/log.txt", "a");

    if (!logFile) {
        Serial.println("[LOGGER] Falha ao abrir log");
    }
}

// =========================
// FUNÇÃO INTERNA
// =========================

void escrever(String nivel, String msg) {
    String ts = String(millis() / 1000.0f, 3);
    String linha = "[" + ts + "] [" + nivel + "] " + msg;

    // Serial
    Serial.println(linha);

    // Arquivo
    if (logFile) {
        logFile.println(linha);
        logFile.flush(); // garante escrita
    }
}

// =========================
// NÍVEIS
// =========================

void logDebug(String msg) {
    escrever("DEBUG", msg);
}

void logInfo(String msg) {
    escrever("INFO", msg);
}

void logWarn(String msg) {
    escrever("WARN", msg);
}

void logError(String msg) {
    escrever("ERROR", msg);
}

void logFatal(String msg) {
    escrever("FATAL", msg);
}