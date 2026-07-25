#include "cmd_motores.h"

#include "../../motores/motores.h"

static void cmd_motor_f(String args) { int v = args.toInt(); moverFrente(v); Serial.println("[OK] MOTOR F " + String(v)); }
static void cmd_motor_t(String args) { int v = args.toInt(); moverTras(v); Serial.println("[OK] MOTOR T " + String(v)); }
static void cmd_motor_ve(String args) { int v = args.toInt(); virarEsquerda(v); Serial.println("[OK] MOTOR VE " + String(v)); }
static void cmd_motor_vd(String args) { int v = args.toInt(); virarDireita(v); Serial.println("[OK] MOTOR VD " + String(v)); }
static void cmd_motor_e(String args) { int v = args.toInt(); setVelocidade(v, 0); Serial.println("[OK] MOTOR E " + String(v)); }
static void cmd_motor_d(String args) { int v = args.toInt(); setVelocidade(0, v); Serial.println("[OK] MOTOR D " + String(v)); }
static void cmd_motor_stop(String args) { pararMotores(); Serial.println("[OK] MOTOR STOP"); }
static void cmd_motor_exit(String args) { pararMotores(); console_setState(STATE_MAIN); Serial.println("[OK] MOTOR EXIT"); }
static void cmd_motor_help(String args) { Serial.println("Comandos Motores: F T VE VD E D STOP EXIT"); }

static Command comandosMotor[] = {
    {"F", cmd_motor_f, "Move ambos para frente"},
    {"T", cmd_motor_t, "Move ambos para trás"},
    {"VE", cmd_motor_ve, "Vira à esquerda"},
    {"VD", cmd_motor_vd, "Vira à direita"},
    {"E", cmd_motor_e, "Controla lado esquerdo"},
    {"D", cmd_motor_d, "Controla lado direito"},
    {"STOP", cmd_motor_stop, "Interrompe motores"},
    {"EXIT", cmd_motor_exit, "Volta ao menu principal"},
    {"HELP", cmd_motor_help, "Ajuda do menu motores"},
};

Command* getMotorCommands(size_t &count) {
    count = sizeof(comandosMotor)/sizeof(Command);
    return comandosMotor;
}
