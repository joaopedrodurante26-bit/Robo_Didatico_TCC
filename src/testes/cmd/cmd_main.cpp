#include "cmd_main.h"

#include <LittleFS.h>
#include <WiFi.h>
#include "../../diagnostico/diagnostico.h"
#include "../../robot/robot.h"

// Protótipos locais das funções de comando
static void cmd_help_main(String args);
static void cmd_status(String args);
static void cmd_version(String args);
static void cmd_reboot(String args);
static void cmd_clear(String args);
static void cmd_mode(String args);
static void cmd_enter_motor(String args);
static void cmd_enter_sensor(String args);
static void cmd_wifi(String args);
static void cmd_fs(String args);
static void cmd_diag(String args);
static void cmd_stop(String args);
static void cmd_ui(String args);

static Command comandosMain[] = {
    {"HELP", cmd_help_main, "Mostra ajuda"},
    {"STATUS", cmd_status, "Estado do sistema"},
    {"SYSTEM STATUS", cmd_status, "Estado do sistema (alias)"},
    {"VERSION", cmd_version, "Versão do firmware"},
    {"SYSTEM VERSION", cmd_version, "Versão do firmware (alias)"},
    {"MODE", cmd_mode, "Altera o modo operacional"},
    {"REBOOT", cmd_reboot, "Reinicia o ESP32"},
    {"SYSTEM REBOOT", cmd_reboot, "Reinicia o ESP32 (alias)"},
    {"CLEAR", cmd_clear, "Limpa o terminal"},
    {"SYSTEM CLEAR", cmd_clear, "Limpa o terminal (alias)"},
    {"MOTOR", cmd_enter_motor, "Menu dos motores"},
    {"SENSOR", cmd_enter_sensor, "Menu dos sensores"},
    {"WIFI", cmd_wifi, "Informações do WiFi"},
    {"FS", cmd_fs, "Informações do sistema de arquivos"},
    {"DIAG", cmd_diag, "Executa diagnóstico automático"},
    {"STOP", cmd_stop, "Interrompe streams de sensores"},
    {"UI", cmd_ui, "Mostra/altera interface: CONSOLE, CONTROL, MONITOR, CONFIG"},
};

Command* getMainCommands(size_t &count) {
    count = sizeof(comandosMain)/sizeof(Command);
    return comandosMain;
}

// ---------- implementações ----------
static void cmd_help_main(String args) {
    console_println("Comandos disponíveis:");
    for (size_t i = 0; i < sizeof(comandosMain)/sizeof(Command); ++i) {
        console_println(String(comandosMain[i].nome) + " - " + String(comandosMain[i].descricao));
    }
}

static void cmd_status(String args) {
    console_println("Sistema: OK");
    console_println("Modo atual: " + String(robotModeToString(getCurrentMode())));
    console_println("Interface atual: " + String(interfaceModeToString(getInterfaceMode())));
    console_println("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
}

static void cmd_version(String args) {
    console_println("Firmware 2.0");
}

static void cmd_reboot(String args) {
    console_println("Reiniciando...");
    delay(200);
    ESP.restart();
}

static void cmd_clear(String args) {
    console_clear();
}

static void cmd_mode(String args) {
    String entrada = args;
    entrada.trim();
    entrada.toUpperCase();

    if (entrada.length() == 0) {
        console_println("Modo atual: " + String(robotModeToString(getCurrentMode())));
        return;
    }

    if (entrada == "IDLE") {
        setRobotMode(MODE_IDLE);
    } else if (entrada == "REMOTE" || entrada == "MANUAL") {
        setRobotMode(MODE_MANUAL);
    } else if (entrada == "AUTO" || entrada == "AUTONOMOUS") {
        setRobotMode(MODE_AUTONOMOUS);
    } else if (entrada == "CAL" || entrada == "CALIBRATION") {
        setRobotMode(MODE_CALIBRATION);
    } else if (entrada == "TEST") {
        setRobotMode(MODE_CALIBRATION);
    } else if (entrada == "DIAG" || entrada == "DIAGNOSTIC") {
        console_println("MODE DIAG foi descontinuado. Use o comando DIAG.");
        return;
    } else {
        console_println("Modo inválido. Use IDLE, MANUAL, AUTO ou CAL.");
        return;
    }

    console_println("[OK] Comando MODE executado: " + String(robotModeToString(getCurrentMode())));
}

static void cmd_enter_motor(String args) {
    console_setState(STATE_MOTOR);
    console_println("Entrando no menu de motores...");
}

static void cmd_enter_sensor(String args) {
    console_setState(STATE_SENSOR);
    console_println("Entrando no menu de sensores...");
}

static void cmd_wifi(String args) {
    console_println("WIFI STATUS");
    console_println("");
    console_println("SSID");
    console_println(WiFi.softAPgetStationNum() ? String("ROBO_VESPA") : String("ROBO_VESPA"));
    console_println("");
    console_println("IP");
    console_println(WiFi.softAPIP().toString());
    console_println("");
    console_println("Clientes");
    console_println("");
    console_println(String(WiFi.softAPgetStationNum()) + " conectado(s)");
}

static void cmd_fs(String args) {
    console_println("FS INFO");

    if (!LittleFS.begin(true)) {
        console_println("LittleFS não montado");
        return;
    }

    console_println("LittleFS");
    console_println("");
    console_println("Montado");
    console_println("Arquivos:");

    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) {
        console_println("(nenhum arquivo encontrado)");
        return;
    }

    File entry = root.openNextFile();
    if (!entry) {
        console_println("(diretório vazio)");
        return;
    }

    do {
        console_println(String(entry.name()));
    } while ((entry = root.openNextFile()));
}

static void cmd_diag(String args) {
    executarDiagnostico();
    console_println("Diagnóstico concluído.");
}

static void cmd_stop(String args) {
    console_stopStreams();
    console_println("[OK] STREAM interrompido.");
}

static void cmd_ui(String args) {
    String entrada = args;
    entrada.trim();
    entrada.toUpperCase();

    if (entrada.length() == 0) {
        console_println("Interface atual: " + String(interfaceModeToString(getInterfaceMode())));
        return;
    }

    if (entrada == "CONSOLE") {
        setInterfaceMode(UI_CONSOLE);
    } else if (entrada == "CONTROL") {
        setInterfaceMode(UI_CONTROL);
    } else if (entrada == "MONITOR") {
        setInterfaceMode(UI_MONITOR);
    } else if (entrada == "CONFIG" || entrada == "CONFIGURATION") {
        setInterfaceMode(UI_CONFIGURATION);
    } else {
        console_println("UI inválida. Use CONSOLE, CONTROL, MONITOR ou CONFIG.");
        return;
    }

    console_println("[OK] Interface selecionada: " + String(interfaceModeToString(getInterfaceMode())));
}
