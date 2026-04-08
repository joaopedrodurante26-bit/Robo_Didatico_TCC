#include "diagnostico.h"
#include "../sensores/sensores.h"
#include <LittleFS.h>
#include "../utils/logger.h"

void executarDiagnostico() {
    logInfo("Iniciando diagnóstico...");

    String log = "{";

    // =========================
    // MPU6050
    // =========================
    bool imu_ok = (getAccelX() != 0 || getAccelY() != 0 || getAccelZ() != 0);
    if (imu_ok) {
        logInfo("MPU6050: OK");
    } else {
        logError("MPU6050: FALHA");
    }
    log += "\"imu\": " + String(imu_ok ? "true" : "false") + ",";

    // =========================
    // Ultrassônico
    // =========================
    float dist = getDistancia();
    bool ultra_ok = (dist > 0);
    if (ultra_ok) {
        logInfo("Sensor Ultrassônico: OK (Distância: " + String(dist) + " cm)");
    } else {
        logWarn("Sensor Ultrassônico: FALHA (Distância inválida: " + String(dist) + " cm)");
    }
    log += "\"ultrassonico\": " + String(ultra_ok ? "true" : "false") + ",";

    // =========================
    // Encoders
    // =========================
    bool enc_ok = true;
    if (enc_ok) {
        logInfo("Encoders: OK");
    } else {
        logError("Encoders: FALHA");
    }
    log += "\"encoders\": " + String(enc_ok ? "true" : "false");

    log += "}";

    // =========================
    // Salvar arquivo
    // =========================
    File file = LittleFS.open("/boot_log.json", "w");

    if (file) {
        file.print(log);
        file.close();
    }
}