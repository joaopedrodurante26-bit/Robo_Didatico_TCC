#include "cmd_main.h"

#include <LittleFS.h>
#include <WiFi.h>
#include "../../diagnostico/diagnostico.h"
#include "../../robot/robot.h"
#include "../../config/configuracao.h"

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

static String upTrim(String s) {
    s.trim();
    s.toUpperCase();
    return s;
}

static String headToken(const String& text) {
    String s = text;
    s.trim();
    int p = s.indexOf(' ');
    if (p < 0) {
        return s;
    }
    return s.substring(0, p);
}

static String tailToken(const String& text) {
    String s = text;
    s.trim();
    int p = s.indexOf(' ');
    if (p < 0) {
        return "";
    }
    String rest = s.substring(p + 1);
    rest.trim();
    return rest;
}

static void printGeneralHelp() {
    console_println("Uso: HELP [OBJETO] [COMANDO]");
    console_println("");
    console_println("Exemplos:");
    console_println("HELP ULTRA");
    console_println("HELP MOTOR F");
    console_println("");
    console_println("Objetos disponíveis:");
    console_println("MAIN   - comandos gerais do sistema");
    console_println("MOTOR  - comandos de movimento");
    console_println("ULTRA  - comandos do ultrassônico");
    console_println("SENSOR - menu de sensores");
}

static void printMainHelp() {
    console_println("MAIN - Comandos gerais");
    console_println("");
    console_println("HELP - Mostra ajuda");
    console_println("STATUS - Estado do sistema");
    console_println("VERSION - Versao do firmware");
    console_println("MODE - Altera modo operacional");
    console_println("REBOOT - Reinicia o ESP32");
    console_println("CLEAR - Limpa o terminal");
    console_println("MOTOR - Entra no menu de motores");
    console_println("SENSOR - Entra no menu de sensores");
    console_println("WIFI - Informacoes do WiFi");
    console_println("FS - Informacoes do sistema de arquivos");
    console_println("DIAG - Executa diagnostico automatico");
    console_println("STOP - Interrompe streams");
    console_println("UI - Mostra/altera interface");
}

static void printUltraHelp() {
    console_println("ULTRA - Comandos do HC-SR04");
    console_println("");
    console_println("ULTRA READ");
    console_println("Mostra estado do driver e tempo de eco em us.");
    console_println("");
    console_println("ULTRA RAW");
    console_println("Valida trigger/echo e exibe o tempo bruto do eco.");
    console_println("");
    console_println("ULTRA STATUS");
    console_println("Exibe estado semântico, presença e contadores.");
    console_println("");
    console_println("ULTRA DIST");
    console_println("Converte tempo de eco para distância em cm (via SensorManager).");
    console_println("");
    console_println("ULTRA STREAM");
    console_println("Inicia leitura contínua da distância no console.");
    console_println("Pare com STOP ULTRA ou STOP.");
}

static void printMotorHelp() {
    console_println("MOTOR - Comandos de movimento");
    console_println("");
    console_println("MOTOR F <velocidade> [tempo_s]");
    console_println("MOTOR T <velocidade> [tempo_s]");
    console_println("MOTOR VE <velocidade> [tempo_s]");
    console_println("MOTOR VD <velocidade> [tempo_s]");
    console_println("MOTOR STOP");
    console_println("");
    console_println("Faixa de velocidade: " + String(PWM_MIN) + " até " + String(PWM_MAX));
    console_println("Se [tempo_s] for informado: executa por N segundos e para.");
    console_println("Sem [tempo_s]: movimento contínuo até novo comando (ex.: STOP).");
}

static void printMotorFHelp() {
    console_println("MOTOR F - Frente");
    console_println("");
    console_println("Sintaxe");
    console_println("MOTOR F <velocidade> [tempo_s]");
    console_println("");
    console_println("Parâmetros");
    console_println("<velocidade> : intensidade PWM dos dois motores.");
    console_println("[tempo_s]    : duração em segundos (opcional).");
    console_println("");
    console_println("Comportamento");
    console_println("Com tempo: mantém movimento por N segundos e finaliza comando.");
    console_println("Sem tempo: mantém movimento contínuo até STOP ou outro comando.");
    console_println("A segurança ultrassônica pode interromper o comando se houver obstáculo.");
    console_println("");
    console_println("Exemplos");
    console_println("MOTOR F 120");
    console_println("MOTOR F 180 2");
}

static void printMotorSubcommandHelp(const String& sub) {
    if (sub == "F" || sub == "FORWARD") {
        printMotorFHelp();
        return;
    }

    if (sub == "T" || sub == "BACKWARD") {
        console_println("MOTOR T - Ré");
        console_println("Sintaxe: MOTOR T <velocidade> [tempo_s]");
        console_println("Sem tempo: contínuo até STOP ou novo comando.");
        return;
    }

    if (sub == "VE" || sub == "TURN LEFT") {
        console_println("MOTOR VE - Curva à esquerda");
        console_println("Sintaxe: MOTOR VE <velocidade> [tempo_s]");
        return;
    }

    if (sub == "VD" || sub == "TURN RIGHT") {
        console_println("MOTOR VD - Curva à direita");
        console_println("Sintaxe: MOTOR VD <velocidade> [tempo_s]");
        return;
    }

    if (sub == "STOP") {
        console_println("MOTOR STOP");
        console_println("Interrompe o comando de movimento ativo e para os motores.");
        return;
    }

    console_println("Subcomando MOTOR desconhecido: " + sub);
    console_println("Use HELP MOTOR para listar a sintaxe disponível.");
}

Command* getMainCommands(size_t &count) {
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

    count = sizeof(comandosMain)/sizeof(Command);
    return comandosMain;
}

// ---------- implementações ----------
static void cmd_help_main(String args) {
    String entrada = upTrim(args);
    if (entrada.length() == 0) {
        printGeneralHelp();
        return;
    }

    String objeto = headToken(entrada);
    String restante = tailToken(entrada);

    if (objeto == "MAIN") {
        printMainHelp();
        return;
    }

    if (objeto == "ULTRA") {
        printUltraHelp();
        return;
    }

    if (objeto == "MOTOR") {
        if (restante.length() == 0) {
            printMotorHelp();
        } else {
            printMotorSubcommandHelp(restante);
        }
        return;
    }

    if (objeto == "SENSOR") {
        console_println("SENSOR");
        console_println("Use SENSOR para entrar no submenu.");
        console_println("Depois use HELP para comandos do submenu.");
        console_println("Atalho: HELP ULTRA para comandos do HC-SR04.");
        return;
    }

    console_println("Objeto de ajuda desconhecido: " + objeto);
    console_println("Use HELP para ver os objetos disponíveis.");
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
