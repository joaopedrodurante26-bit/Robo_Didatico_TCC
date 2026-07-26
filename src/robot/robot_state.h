#ifndef ROBOT_STATE_H
#define ROBOT_STATE_H

#include <Arduino.h>

#include "robot.h"

struct RobotState {
    RobotMode mode;
    InterfaceMode interfaceMode;

    long encoderEsq;
    long encoderDir;

    float distanciaCm;

    float accelX;
    float accelY;
    float accelZ;

    float gyroX;
    float gyroY;
    float gyroZ;

    int velEsqCmd;
    int velDirCmd;

    unsigned long uptimeMs;

    int wifiClients;
    char wifiIp[16];
};

void initRobotState();

void robotStateSetMode(RobotMode mode);
void robotStateSetInterfaceMode(InterfaceMode mode);

void robotStateSetSensors(
    long encoderEsq,
    long encoderDir,
    float distanciaCm,
    float accelX,
    float accelY,
    float accelZ,
    float gyroX,
    float gyroY,
    float gyroZ);

void robotStateSetMotorCommand(int velEsqCmd, int velDirCmd);
void robotStateSetWifi(const String& ip, int clients);
void robotStateSetUptime(unsigned long uptimeMs);

RobotState getRobotStateSnapshot();

#endif
