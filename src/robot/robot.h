#ifndef ROBOT_H
#define ROBOT_H

// Modo operacional central do robô.
// O gerenciador de modos é a única entidade que decide quem manda.
enum RobotMode {
    MODE_IDLE,
    MODE_MANUAL,
    MODE_AUTONOMOUS,
    MODE_CALIBRATION
};

// Interface ativa para o usuário.
// Não altera o comportamento central do robô.
enum InterfaceMode {
    UI_CONSOLE,
    UI_CONTROL,
    UI_MONITOR,
    UI_CONFIGURATION
};

void initRobot();
void setRobotMode(RobotMode mode);
RobotMode getCurrentMode();
const char* robotModeToString(RobotMode mode);

void setInterfaceMode(InterfaceMode mode);
InterfaceMode getInterfaceMode();
const char* interfaceModeToString(InterfaceMode mode);

void robot_update();

#endif
