#include "cmd_main.h"

#include <LittleFS.h>
#include <WiFi.h>
#include "../../diagnostico/diagnostico.h"
#include "../../robot/robot.h"
#include "../../config/configuracao.h"
#include "../../utils/logger.h"

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
static void cmd_log(String args);
static const char* LOG_FILE_PATH = "/log.txt";
static const uint16_t LOG_TAIL_MAX_LINES = 40;
static bool ensureLittleFsMounted();
static String normalizeFsPath(String path);
static uint16_t parseLineCount(const String& value, uint16_t defaultValue);
static void printFsUsage();
static void printLogUsage();
static void listFsPath(const String& rawPath);
static void printFileContents(const String& rawPath);
static void printLogTail(uint16_t requestedLines);

static String upTrim(String s) {
    s.trim();
    s.toUpperCase();
    return s;
}

static void splitCommandVerbAndArgs(const String& text, String& verb, String& args) {
    String input = text;
    input.trim();

    int separator = input.indexOf(' ');
    if (separator < 0) {
        verb = input;
        args = "";
        verb.toUpperCase();
        return;
    }

    verb = input.substring(0, separator);
    args = input.substring(separator + 1);
    args.trim();
    verb.toUpperCase();
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

static void printHelpFile(const char* path) {
    if (!console_printTextFile(path)) {
        console_println("Ajuda indisponivel no momento.");
    }
}

static void printMotorSubcommandHelp(const String& sub) {
    if (sub == "F" || sub == "FORWARD") {
        printHelpFile("/help/motor_f.txt");
        return;
    }

    if (sub == "T" || sub == "BACKWARD") {
        printHelpFile("/help/motor_t.txt");
        return;
    }

    if (sub == "VE" || sub == "TURN LEFT") {
        printHelpFile("/help/motor_ve.txt");
        return;
    }

    if (sub == "VD" || sub == "TURN RIGHT") {
        printHelpFile("/help/motor_vd.txt");
        return;
    }

    if (sub == "STOP") {
        printHelpFile("/help/motor_stop.txt");
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
        {"FS", cmd_fs, "Navega no LittleFS"},
        {"LOG", cmd_log, "Atalhos do arquivo de log"},
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
        printHelpFile("/help/general.txt");
        return;
    }

    String objeto = headToken(entrada);
    String restante = tailToken(entrada);

    if (objeto == "MAIN") {
        printHelpFile("/help/main.txt");
        return;
    }

    if (objeto == "ULTRA") {
        printHelpFile("/help/ultra.txt");
        return;
    }

    if (objeto == "MOTOR") {
        if (restante.length() == 0) {
            printHelpFile("/help/motor.txt");
            console_println("Faixa de velocidade: " + String(PWM_MIN) + " ate " + String(PWM_MAX));
        } else {
            printMotorSubcommandHelp(restante);
        }
        return;
    }

    if (objeto == "SENSOR") {
        printHelpFile("/help/sensor.txt");
        return;
    }

    if (objeto == "FS") {
        printHelpFile("/help/fs.txt");
        return;
    }

    if (objeto == "LOG") {
        printHelpFile("/help/log.txt");
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
    String entrada = args;
    entrada.trim();

    if (entrada.length() == 0 || entrada == "INFO") {
        if (!ensureLittleFsMounted()) {
            return;
        }

        size_t total = LittleFS.totalBytes();
        size_t used = LittleFS.usedBytes();
        size_t freeBytes = (total >= used) ? (total - used) : 0;

        console_println("FS INFO");
        console_println("Montado: sim");
        console_println("Total: " + String((unsigned long)total) + " bytes");
        console_println("Usado: " + String((unsigned long)used) + " bytes");
        console_println("Livre: " + String((unsigned long)freeBytes) + " bytes");
        console_println("");
        printFsUsage();
        return;
    }

    String comando;
    String restante;
    splitCommandVerbAndArgs(entrada, comando, restante);

    if (comando == "LS" || comando == "LIST") {
        listFsPath(restante);
        return;
    }

    if (comando == "CAT" || comando == "READ" || comando == "OPEN") {
        if (restante.length() == 0) {
            printFsUsage();
            return;
        }

        printFileContents(restante);
        return;
    }

    if (comando == "RM" || comando == "DEL" || comando == "DELETE") {
        String target = headToken(restante);
        String confirm = tailToken(restante);
        confirm.toUpperCase();

        if (target.length() == 0 || confirm != "CONFIRM") {
            console_println("Para apagar um arquivo use: FS RM <arquivo> CONFIRM");
            return;
        }

        if (!ensureLittleFsMounted()) {
            return;
        }

        String path = normalizeFsPath(target);
        File file = LittleFS.open(path.c_str(), "r");
        if (!file) {
            console_println("Arquivo nao encontrado: " + path);
            return;
        }

        if (file.isDirectory()) {
            console_println("FS RM remove apenas arquivos, nao diretorios: " + path);
            file.close();
            return;
        }

        file.close();

        if (path == LOG_FILE_PATH) {
            closeLogger();
        }

        if (!LittleFS.remove(path.c_str())) {
            console_println("Falha ao remover: " + path);
            if (path == LOG_FILE_PATH) {
                reopenLogger();
            }
            return;
        }

        console_println("[OK] Removido: " + path);

        if (path == LOG_FILE_PATH) {
            if (reopenLogger()) {
                console_println("[OK] /log.txt recriado para continuar registrando.");
            } else {
                console_println("[AVISO] /log.txt removido, mas nao foi possivel reabrir.");
            }
        }

        return;
    }

    console_println("Uso invalido de FS.");
    printFsUsage();
}

static void cmd_log(String args) {
    String entrada = args;
    entrada.trim();

    if (entrada.length() == 0 || entrada == "INFO") {
        if (!ensureLittleFsMounted()) {
            return;
        }

        File file = LittleFS.open(LOG_FILE_PATH, "r");
        if (!file) {
            console_println("Log nao encontrado em /log.txt");
            printLogUsage();
            return;
        }

        console_println("LOG INFO");
        console_println("Arquivo: /log.txt");
        console_println("Tamanho: " + String((unsigned long)file.size()) + " bytes");
        console_println("");
        printLogUsage();
        file.close();
        return;
    }

    String comando;
    String restante;
    splitCommandVerbAndArgs(entrada, comando, restante);

    if (comando == "SHOW" || comando == "CAT" || comando == "OPEN") {
        printFileContents(LOG_FILE_PATH);
        return;
    }

    if (comando == "TAIL") {
        uint16_t linhas = parseLineCount(restante, 20);
        printLogTail(linhas);
        return;
    }

    if (comando == "CLEAR" || comando == "DEL" || comando == "DELETE") {
        String confirm = restante;
        confirm.toUpperCase();

        if (confirm != "CONFIRM") {
            console_println("Para limpar o log use: LOG CLEAR CONFIRM");
            return;
        }

        if (!ensureLittleFsMounted()) {
            return;
        }

        closeLogger();

        File file = LittleFS.open(LOG_FILE_PATH, "w");
        if (!file) {
            console_println("Falha ao limpar /log.txt");
            reopenLogger();
            return;
        }

        file.close();

        if (reopenLogger()) {
            console_println("[OK] /log.txt limpo.");
        } else {
            console_println("[AVISO] /log.txt limpo, mas nao foi possivel reabrir.");
        }

        return;
    }

    console_println("Uso invalido de LOG.");
    printLogUsage();
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

static bool ensureLittleFsMounted() {
    if (LittleFS.begin()) {
        return true;
    }

    console_println("LittleFS nao montado.");
    return false;
}

static String normalizeFsPath(String path) {
    path.trim();

    if (path.length() == 0) {
        return "/";
    }

    if (path[0] != '/') {
        path = "/" + path;
    }

    return path;
}

static uint16_t parseLineCount(const String& value, uint16_t defaultValue) {
    String text = value;
    text.trim();

    if (text.length() == 0) {
        return defaultValue;
    }

    long parsed = text.toInt();
    if (parsed <= 0) {
        return defaultValue;
    }

    if (parsed > LOG_TAIL_MAX_LINES) {
        parsed = LOG_TAIL_MAX_LINES;
    }

    return (uint16_t)parsed;
}

static void printFsUsage() {
    console_println("Uso do FS:");
    console_println("FS INFO");
    console_println("FS LS [caminho]");
    console_println("FS CAT <arquivo>");
    console_println("FS RM <arquivo> CONFIRM");
}

static void printLogUsage() {
    console_println("Uso do LOG:");
    console_println("LOG INFO");
    console_println("LOG SHOW");
    console_println("LOG TAIL [linhas]");
    console_println("LOG CLEAR CONFIRM");
}

static void listFsPath(const String& rawPath) {
    if (!ensureLittleFsMounted()) {
        return;
    }

    String path = normalizeFsPath(rawPath);
    File node = LittleFS.open(path.c_str(), "r");

    if (!node) {
        console_println("Caminho nao encontrado: " + path);
        return;
    }

    if (!node.isDirectory()) {
        console_println("[FILE] " + path + " (" + String((unsigned long)node.size()) + " bytes)");
        node.close();
        return;
    }

    console_println("Conteudo de " + path + ":");

    File entry = node.openNextFile();
    if (!entry) {
        console_println("(diretorio vazio)");
        node.close();
        return;
    }

    while (entry) {
        String prefix = entry.isDirectory() ? "[DIR ] " : "[FILE] ";
        String line = prefix + String(entry.name());

        if (!entry.isDirectory()) {
            line += " (" + String((unsigned long)entry.size()) + " bytes)";
        }

        console_println(line);
        entry.close();
        entry = node.openNextFile();
    }

    node.close();
}

static void printFileContents(const String& rawPath) {
    if (!ensureLittleFsMounted()) {
        return;
    }

    String path = normalizeFsPath(rawPath);
    File file = LittleFS.open(path.c_str(), "r");

    if (!file) {
        console_println("Arquivo nao encontrado: " + path);
        return;
    }

    if (file.isDirectory()) {
        console_println("O caminho aponta para um diretorio: " + path);
        file.close();
        return;
    }

    console_println("Conteudo de " + path + ":");

    String line = "";
    while (file.available()) {
        char c = (char)file.read();

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            console_println(line);
            line = "";
            continue;
        }

        line += c;
    }

    if (line.length() > 0) {
        console_println(line);
    }

    file.close();
}

static void printLogTail(uint16_t requestedLines) {
    if (!ensureLittleFsMounted()) {
        return;
    }

    File file = LittleFS.open(LOG_FILE_PATH, "r");
    if (!file) {
        console_println("Log nao encontrado em /log.txt");
        return;
    }

    if (file.isDirectory()) {
        console_println("/log.txt aponta para um diretorio inesperadamente.");
        file.close();
        return;
    }

    uint16_t limit = requestedLines == 0 ? 20 : requestedLines;
    if (limit > LOG_TAIL_MAX_LINES) {
        limit = LOG_TAIL_MAX_LINES;
    }

    String lines[LOG_TAIL_MAX_LINES];
    uint16_t count = 0;
    uint16_t index = 0;
    String current = "";

    while (file.available()) {
        char c = (char)file.read();

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            lines[index] = current;
            index = (index + 1) % limit;
            if (count < limit) {
                count++;
            }
            current = "";
            continue;
        }

        current += c;
    }

    if (current.length() > 0) {
        lines[index] = current;
        index = (index + 1) % limit;
        if (count < limit) {
            count++;
        }
    }

    console_println("Ultimas " + String(count) + " linhas de /log.txt:");

    uint16_t start = (count < limit) ? 0 : index;
    for (uint16_t i = 0; i < count; ++i) {
        console_println(lines[(start + i) % limit]);
    }

    file.close();
}
