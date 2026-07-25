#ifndef ROBOT_H
#define ROBOT_H

// Modo operacional central do robô.
// O gerenciador de modos é a única entidade que decide quem manda.
enum RobotMode {
    MODE_IDLE,
    MODE_REMOTE,
    MODE_AUTONOMOUS,
    MODE_TEST,
    MODE_DIAGNOSTIC
};

void initRobot();
void setRobotMode(RobotMode mode);
RobotMode getCurrentMode();
const char* robotModeToString(RobotMode mode);
void robot_update();

#endif
