#include "cmd_motores.h"

#include <Arduino.h>
#include "../../motores/motores.h"

static void executarComMotorTemporizado(void (*acao)(int), int velocidade, int tempoMs) {
    if (tempoMs > 0) {
        acao(velocidade);
        delay(tempoMs);
        pararMotores();
        return;
    }

    acao(velocidade);
}

static void cmd_motor_f(String args) {
    int primeira = args.toInt();
    int segunda = 0;
    int espaco = args.indexOf(' ');

    if (espaco != -1) {
        segunda = args.substring(espaco + 1).toInt();
    }

    executarComMotorTemporizado(moverFrente, primeira, segunda * 1000);
    Serial.println("[OK] MOTOR F " + String(primeira) + " " + String(segunda));
}

static void cmd_motor_t(String args) {
    int primeira = args.toInt();
    int segunda = 0;
    int espaco = args.indexOf(' ');

    if (espaco != -1) {
        segunda = args.substring(espaco + 1).toInt();
    }

    executarComMotorTemporizado(moverTras, primeira, segunda * 1000);
    Serial.println("[OK] MOTOR T " + String(primeira) + " " + String(segunda));
}

static void cmd_motor_ve(String args) {
    int primeira = args.toInt();
    int segunda = 0;
    int espaco = args.indexOf(' ');

    if (espaco != -1) {
        segunda = args.substring(espaco + 1).toInt();
    }

    executarComMotorTemporizado(virarEsquerda, primeira, segunda * 1000);
    Serial.println("[OK] MOTOR VE " + String(primeira) + " " + String(segunda));
}

static void cmd_motor_vd(String args) {
    int primeira = args.toInt();
    int segunda = 0;
    int espaco = args.indexOf(' ');

    if (espaco != -1) {
        segunda = args.substring(espaco + 1).toInt();
    }

    executarComMotorTemporizado(virarDireita, primeira, segunda * 1000);
    Serial.println("[OK] MOTOR VD " + String(primeira) + " " + String(segunda));
}

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
