#include "sensor_manager.h"

#include <Arduino.h>

#include "sensores.h"
#include "../controle/controle.h"
#include "../robot/robot_state.h"

static float g_ultraDistanceCm = -1.0f;
static bool g_ultraDistanceValid = false;

static float echoUsToCm(unsigned long echoUs) {
    // Distância (cm) = tempo(us) * velocidadeSom(cm/us) / 2.
    return static_cast<float>(echoUs) * 0.0343f / 2.0f;
}

void initSensorManager() {
    // Mantém o estado consistente no boot com uma leitura inicial.
    updateSensorManager();
}

void updateSensorManager() {
    atualizarSensores();

    UltrasonicStatus ultraStatus = getUltraStatus();
    unsigned long ultraEchoUs = getUltraEchoTimeUs();
    g_ultraDistanceCm = (ultraEchoUs > 0) ? echoUsToCm(ultraEchoUs) : -1.0f;
    g_ultraDistanceValid = (ultraStatus == ULTRA_OK);

    robotStateSetSensors(
        getPulsosEsq(),
        getPulsosDir(),
        g_ultraDistanceCm,
        getAccelX(),
        getAccelY(),
        getAccelZ(),
        getGyroX(),
        getGyroY(),
        getGyroZ());

    robotStateSetMotorCommand(getVelEsq(), getVelDir());
    robotStateSetUptime(millis());
}

float getUltraDistanceCm() {
    return g_ultraDistanceCm;
}

bool isUltraDistanceValid() {
    return g_ultraDistanceValid;
}
