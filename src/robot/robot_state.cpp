#include "robot_state.h"

#include <cstring>

static RobotState g_state;

void initRobotState() {
    g_state.mode = MODE_IDLE;
    g_state.interfaceMode = UI_CONSOLE;

    g_state.encoderEsq = 0;
    g_state.encoderDir = 0;

    g_state.distanciaCm = -1.0f;

    g_state.accelX = 0.0f;
    g_state.accelY = 0.0f;
    g_state.accelZ = 0.0f;

    g_state.gyroX = 0.0f;
    g_state.gyroY = 0.0f;
    g_state.gyroZ = 0.0f;

    g_state.velEsqCmd = 0;
    g_state.velDirCmd = 0;

    g_state.uptimeMs = 0;

    g_state.wifiClients = 0;
    std::strncpy(g_state.wifiIp, "0.0.0.0", sizeof(g_state.wifiIp));
    g_state.wifiIp[sizeof(g_state.wifiIp) - 1] = '\0';
}

void robotStateSetMode(RobotMode mode) {
    g_state.mode = mode;
}

void robotStateSetInterfaceMode(InterfaceMode mode) {
    g_state.interfaceMode = mode;
}

void robotStateSetSensors(
    long encoderEsq,
    long encoderDir,
    float distanciaCm,
    float accelX,
    float accelY,
    float accelZ,
    float gyroX,
    float gyroY,
    float gyroZ) {
    g_state.encoderEsq = encoderEsq;
    g_state.encoderDir = encoderDir;
    g_state.distanciaCm = distanciaCm;

    g_state.accelX = accelX;
    g_state.accelY = accelY;
    g_state.accelZ = accelZ;

    g_state.gyroX = gyroX;
    g_state.gyroY = gyroY;
    g_state.gyroZ = gyroZ;
}

void robotStateSetMotorCommand(int velEsqCmd, int velDirCmd) {
    g_state.velEsqCmd = velEsqCmd;
    g_state.velDirCmd = velDirCmd;
}

void robotStateSetWifi(const String& ip, int clients) {
    g_state.wifiClients = clients;
    std::strncpy(g_state.wifiIp, ip.c_str(), sizeof(g_state.wifiIp));
    g_state.wifiIp[sizeof(g_state.wifiIp) - 1] = '\0';
}

void robotStateSetUptime(unsigned long uptimeMs) {
    g_state.uptimeMs = uptimeMs;
}

RobotState getRobotStateSnapshot() {
    return g_state;
}
