#include "cmd_main.h"

#include <LittleFS.h>
#include <WiFi.h>
#include "../../diagnostico/diagnostico.h"

// Protótipos locais das funções de comando
static void cmd_help_main(String args);
static void cmd_status(String args);
static void cmd_version(String args);
static void cmd_reboot(String args);
static void cmd_clear(String args);
static void cmd_enter_motor(String args);
static void cmd_enter_sensor(String args);
static void cmd_wifi(String args);
static void cmd_fs(String args);
static void cmd_diag(String args);

static Command comandosMain[] = {
    {"HELP", cmd_help_main, "Mostra ajuda"},
    {"STATUS", cmd_status, "Estado do sistema"},
    {"VERSION", cmd_version, "Versão do firmware"},
    {"REBOOT", cmd_reboot, "Reinicia o ESP32"},
    {"CLEAR", cmd_clear, "Limpa o terminal"},
    {"MOTOR", cmd_enter_motor, "Menu dos motores"},
    {"SENSOR", cmd_enter_sensor, "Menu dos sensores"},
    {"WIFI", cmd_wifi, "Informações do WiFi"},
    {"FS", cmd_fs, "Informações do sistema de arquivos"},
    {"DIAG", cmd_diag, "Executa diagnóstico automático"},
};

Command* getMainCommands(size_t &count) {
    count = sizeof(comandosMain)/sizeof(Command);
    return comandosMain;
}

// ---------- implementações ----------
static void cmd_help_main(String args) {
    Serial.println("Comandos disponíveis:");
    for (size_t i = 0; i < sizeof(comandosMain)/sizeof(Command); ++i) {
        Serial.print(comandosMain[i].nome);
        Serial.print(" - ");
        Serial.println(comandosMain[i].descricao);
    }
}

static void cmd_status(String args) {
    Serial.println("Sistema: OK");
    Serial.println("RAM Livre: ? kB");
}

static void cmd_version(String args) {
    Serial.println("Firmware 1.0");
}

static void cmd_reboot(String args) {
    Serial.println("Reiniciando...");
    delay(200);
    ESP.restart();
}

static void cmd_clear(String args) {
    for (int i=0;i<30;i++) Serial.println();
}

static void cmd_enter_motor(String args) {
    console_setState(STATE_MOTOR);
    Serial.println("Entrando no menu de motores...");
}

static void cmd_enter_sensor(String args) {
    console_setState(STATE_SENSOR);
    Serial.println("Entrando no menu de sensores...");
}

static void cmd_wifi(String args) {
    Serial.println("WIFI STATUS");
    Serial.println();
    Serial.println("SSID");
    Serial.println(WiFi.softAPgetStationNum() ? String("ROBO_VESPA") : String("ROBO_VESPA"));
    Serial.println();
    Serial.println("IP");
    Serial.println(WiFi.softAPIP().toString());
    Serial.println();
    Serial.print("Clientes\n\n");
    Serial.println(String(WiFi.softAPgetStationNum()) + " conectado(s)");
}

static void cmd_fs(String args) {
    Serial.println("FS INFO");
    if (LittleFS.begin()) {
        Serial.println("LittleFS\n\nMontado");
        Serial.println("Arquivos:");
        if (LittleFS.exists("/index.html")) Serial.println("index.html");
        if (LittleFS.exists("/config.json")) Serial.println("config.json");
        if (LittleFS.exists("/logo.bmp")) Serial.println("logo.bmp");
    } else {
        Serial.println("LittleFS não montado");
    }
}

static void cmd_diag(String args) {
    executarDiagnostico();
    Serial.println("Diagnóstico concluído.");
}
