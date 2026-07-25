#include "robot.h"

#include <Arduino.h>

#include "controle/controle.h"
#include "motores/motores.h"
#include "sensores/sensores.h"
#include "utils/logger.h"

static RobotMode currentMode = MODE_IDLE;

void initRobot() {
    currentMode = MODE_IDLE;
    initControle();
}

void setRobotMode(RobotMode mode) {
    currentMode = mode;
}

RobotMode getCurrentMode() {
    return currentMode;
}

const char* robotModeToString(RobotMode mode) {
    switch (mode) {
        case MODE_IDLE: return "MODE_IDLE";
        case MODE_REMOTE: return "MODE_REMOTE";
        case MODE_AUTONOMOUS: return "MODE_AUTONOMOUS";
        case MODE_TEST: return "MODE_TEST";
        case MODE_DIAGNOSTIC: return "MODE_DIAGNOSTIC";
        default: return "MODE_UNKNOWN";
    }
}

void robot_update() {
    switch (currentMode) {
        case MODE_IDLE:
            pararMotores();
            break;
        case MODE_REMOTE:
            atualizarControle();
            break;
        case MODE_AUTONOMOUS:
            atualizarControle();
            break;
        case MODE_TEST:
            break;
        case MODE_DIAGNOSTIC:
            break;
    }
}
