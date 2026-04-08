#include "diagnostico.h"
#include "../sensores/sensores.h"
#include <LittleFS.h>
#include "../utils/logger.h"

void executarDiagnostico() {

    String log = "{";

    // =========================
    // MPU6050
    // =========================
    bool imu_ok = (getAccelX() != 0 || getAccelY() != 0 || getAccelZ() != 0);
    log += "\"imu\": " + String(imu_ok ? "true" : "false") + ",";

    // =========================
    // Ultrassônico
    // =========================
    float dist = getDistancia();
    bool ultra_ok = (dist > 0);
    log += "\"ultrassonico\": " + String(ultra_ok ? "true" : "false") + ",";

    // =========================
    // Encoders
    // =========================
    bool enc_ok = true; // (pode melhorar depois)
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