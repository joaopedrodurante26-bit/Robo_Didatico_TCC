#include "sensor_manager.h"

#include <Arduino.h>

#include "sensores.h"
#include "../controle/controle.h"
#include "../robot/robot_state.h"

void initSensorManager() {
    // Mantém o estado consistente no boot com uma leitura inicial.
    updateSensorManager();
}

void updateSensorManager() {
    atualizarSensores();

    robotStateSetSensors(
        getPulsosEsq(),
        getPulsosDir(),
        getDistancia(),
        getAccelX(),
        getAccelY(),
        getAccelZ(),
        getGyroX(),
        getGyroY(),
        getGyroZ());

    robotStateSetMotorCommand(getVelEsq(), getVelDir());
    robotStateSetUptime(millis());
}
