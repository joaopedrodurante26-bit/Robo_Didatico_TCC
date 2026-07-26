#include "robot.h"

#include <Arduino.h>

#include "controle/controle.h"
#include "motores/motores.h"
#include "sensores/sensores.h"
#include "utils/logger.h"

static RobotMode currentMode = MODE_IDLE;
static InterfaceMode currentInterfaceMode = UI_CONSOLE;

void initRobot() {
    currentMode = MODE_IDLE;
    currentInterfaceMode = UI_CONSOLE;
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
        case MODE_MANUAL: return "MODE_MANUAL";
        case MODE_AUTONOMOUS: return "MODE_AUTONOMOUS";
        case MODE_CALIBRATION: return "MODE_CALIBRATION";
        default: return "MODE_UNKNOWN";
    }
}

void setInterfaceMode(InterfaceMode mode) {
    currentInterfaceMode = mode;
}

InterfaceMode getInterfaceMode() {
    return currentInterfaceMode;
}

const char* interfaceModeToString(InterfaceMode mode) {
    switch (mode) {
        case UI_CONSOLE: return "UI_CONSOLE";
        case UI_CONTROL: return "UI_CONTROL";
        case UI_MONITOR: return "UI_MONITOR";
        case UI_CONFIGURATION: return "UI_CONFIGURATION";
        default: return "UI_UNKNOWN";
    }
}

void robot_update() {
    switch (currentMode) {
        case MODE_IDLE:
            pararMotores();
            break;
        case MODE_MANUAL:
            atualizarControle();
            break;
        case MODE_AUTONOMOUS:
            atualizarControle();
            break;
        case MODE_CALIBRATION:
            break;
    }
}
