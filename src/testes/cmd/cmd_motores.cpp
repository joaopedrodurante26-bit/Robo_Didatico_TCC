#include "cmd_motores.h"

#include <Arduino.h>
#include "../../motores/motores.h"
#include "../../sensores/sensor_manager.h"

static void executarComMotor(void (*acao)(int), int velocidade, bool usarTempo, int tempoMs) {
    motores_iniciarComando();

    if (usarTempo && tempoMs > 0) {
        unsigned long inicio = millis();

        while (millis() - inicio < (unsigned long)tempoMs) {
            acao(velocidade);
            updateSensorManager();
            atualizarMotores();

            if (motoresSegurancaAtiva()) {
                break;
            }

            delay(1);
        }

        motores_finalizarComando();
        return;
    }

    // Sem tempo: mantém movimento até novo comando (ex.: STOP).
    acao(velocidade);
}

static void cmd_motor_f(String args) {
    int primeira = args.toInt();
    int segunda = 0;
    bool temTempo = false;
    int espaco = args.indexOf(' ');

    if (espaco != -1) {
        String resto = args.substring(espaco + 1);
        resto.trim();
        if (resto.length() > 0) {
            temTempo = true;
            segunda = resto.toInt();
        }
    }

    executarComMotor(moverFrente, primeira, temTempo, segunda * 1000);
    if (temTempo) {
        console_println("[OK] MOTOR F " + String(primeira) + " " + String(segunda));
    } else {
        console_println("[OK] MOTOR F " + String(primeira));
    }
}

static void cmd_motor_t(String args) {
    int primeira = args.toInt();
    int segunda = 0;
    bool temTempo = false;
    int espaco = args.indexOf(' ');

    if (espaco != -1) {
        String resto = args.substring(espaco + 1);
        resto.trim();
        if (resto.length() > 0) {
            temTempo = true;
            segunda = resto.toInt();
        }
    }

    executarComMotor(moverTras, primeira, temTempo, segunda * 1000);
    if (temTempo) {
        console_println("[OK] MOTOR T " + String(primeira) + " " + String(segunda));
    } else {
        console_println("[OK] MOTOR T " + String(primeira));
    }
}

static void cmd_motor_ve(String args) {
    int primeira = args.toInt();
    int segunda = 0;
    bool temTempo = false;
    int espaco = args.indexOf(' ');

    if (espaco != -1) {
        String resto = args.substring(espaco + 1);
        resto.trim();
        if (resto.length() > 0) {
            temTempo = true;
            segunda = resto.toInt();
        }
    }

    executarComMotor(virarEsquerda, primeira, temTempo, segunda * 1000);
    if (temTempo) {
        console_println("[OK] MOTOR VE " + String(primeira) + " " + String(segunda));
    } else {
        console_println("[OK] MOTOR VE " + String(primeira));
    }
}

static void cmd_motor_vd(String args) {
    int primeira = args.toInt();
    int segunda = 0;
    bool temTempo = false;
    int espaco = args.indexOf(' ');

    if (espaco != -1) {
        String resto = args.substring(espaco + 1);
        resto.trim();
        if (resto.length() > 0) {
            temTempo = true;
            segunda = resto.toInt();
        }
    }

    executarComMotor(virarDireita, primeira, temTempo, segunda * 1000);
    if (temTempo) {
        console_println("[OK] MOTOR VD " + String(primeira) + " " + String(segunda));
    } else {
        console_println("[OK] MOTOR VD " + String(primeira));
    }
}

static void cmd_motor_e(String args) { int v = args.toInt(); setVelocidade(v, 0); console_println("[OK] MOTOR E " + String(v)); }
static void cmd_motor_d(String args) { int v = args.toInt(); setVelocidade(0, v); console_println("[OK] MOTOR D " + String(v)); }
static void cmd_motor_stop(String args) { motores_finalizarComando(); console_println("[OK] MOTOR STOP"); }
static void cmd_motor_exit(String args) { motores_finalizarComando(); console_setState(STATE_MAIN); console_println("[OK] MOTOR EXIT"); }
static void cmd_motor_help(String args) { console_println("Comandos Motores: F T VE VD E D STOP EXIT"); }

static Command comandosMotor[] = {
    {"F", cmd_motor_f, "Move ambos para frente"},
    {"FORWARD", cmd_motor_f, "Move ambos para frente (alias)"},
    {"T", cmd_motor_t, "Move ambos para trás"},
    {"BACKWARD", cmd_motor_t, "Move ambos para trás (alias)"},
    {"VE", cmd_motor_ve, "Vira à esquerda"},
    {"TURN LEFT", cmd_motor_ve, "Vira à esquerda (alias)"},
    {"VD", cmd_motor_vd, "Vira à direita"},
    {"TURN RIGHT", cmd_motor_vd, "Vira à direita (alias)"},
    {"E", cmd_motor_e, "Controla lado esquerdo"},
    {"LEFT", cmd_motor_e, "Controla lado esquerdo (alias)"},
    {"D", cmd_motor_d, "Controla lado direito"},
    {"RIGHT", cmd_motor_d, "Controla lado direito (alias)"},
    {"STOP", cmd_motor_stop, "Interrompe motores"},
    {"EXIT", cmd_motor_exit, "Volta ao menu principal"},
    {"HELP", cmd_motor_help, "Ajuda do menu motores"},
};

Command* getMotorCommands(size_t &count) {
    count = sizeof(comandosMotor)/sizeof(Command);
    return comandosMotor;
}
