#include "logger.h"
#include <LittleFS.h>

static File logFile;
static const char* LOG_PATH = "/log.txt";

static String formatTimestamp(unsigned long ms) {
    unsigned long totalSeconds = ms / 1000UL;
    unsigned int hours = (unsigned int)(totalSeconds / 3600UL);
    unsigned int minutes = (unsigned int)((totalSeconds % 3600UL) / 60UL);
    unsigned int seconds = (unsigned int)(totalSeconds % 60UL);
    unsigned int millisPart = (unsigned int)(ms % 1000UL);

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u.%03u", hours, minutes, seconds, millisPart);
    return String(buffer);
}

static bool openLogFile() {
    if (logFile) {
        logFile.close();
    }

    logFile = LittleFS.open(LOG_PATH, "a");
    return (bool)logFile;
}

// =========================
// INICIALIZAÇÃO
// =========================

void initLogger() {

    if (!LittleFS.begin()) {
        Serial.println("[LOGGER] Falha no LittleFS");
        return;
    }

    if (!openLogFile()) {
        Serial.println("[LOGGER] Falha ao abrir log");
    }
}

void closeLogger() {
    if (logFile) {
        logFile.close();
    }

    logFile = File();
}

bool reopenLogger() {
    if (!LittleFS.begin()) {
        return false;
    }

    return openLogFile();
}

// =========================
// FUNÇÃO INTERNA
// =========================

void escrever(String nivel, String msg) {
    String ts = formatTimestamp(millis());
    String linha = "[" + ts + "] [" + nivel + "] " + msg;

    // Serial
    Serial.println(linha);

    // Arquivo
    if (logFile) {
        logFile.println(linha);
        logFile.flush(); // garante escrita
    } else if (reopenLogger()) {
        logFile.println(linha);
        logFile.flush();
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