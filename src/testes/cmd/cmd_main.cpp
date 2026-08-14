#include "cmd_main.h"

#include <LittleFS.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "../../diagnostico/diagnostico.h"
#include "../../robot/robot.h"
#include "../../config/configuracao.h"
#include "../../sensores/sensores.h"
#include "../../sensores/sensor_manager.h"
#include "../../motores/motores.h"
#include "../../wifi/wifi_manager.h"
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
static void cmd_enter_wifi(String args);
static void cmd_fs(String args);
static void cmd_diag(String args);
static void cmd_stop(String args);
static void cmd_ui(String args);
static void cmd_log(String args);
static void cmd_watch(String args);
static void cmd_config(String args);
static void cmd_run(String args);
static const char* LOG_FILE_PATH = "/log.txt";
static const char* CONFIG_FILE_PATH = "/robot_config.json";
static const uint16_t LOG_TAIL_MAX_LINES = 40;

struct RuntimeConfig {
    String wifiSsid;
    String wifiPass;
    int motorPwmMin;
    int motorPwmMax;
    float ultraCalibrationFactor;
};

static RuntimeConfig systemConfig = {
    "ROBO_TCC",
    "12345678",
    PWM_MIN,
    PWM_MAX,
    1.0f
};

static void printStatusFull(bool detailed);
static void printSensorStatus(void);
static void printMotorStatus(void);
static void printWifiStatus(void);
static void printConfigUsage(void);
static void printConfigValues(void);
static bool loadSystemConfigFromFile();
static bool saveSystemConfigToFile();
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
        {"WIFI", cmd_enter_wifi, "Menu do WiFi"},
        {"FS", cmd_fs, "Navega no LittleFS"},
        {"LOG", cmd_log, "Atalhos do arquivo de log"},
        {"DIAG", cmd_diag, "Executa diagnóstico automático"},
        {"STOP", cmd_stop, "Interrompe streams de sensores"},
        {"WATCH", cmd_watch, "Ativa monitoramento em tempo real"},
        {"RUN", cmd_run, "Executa rotina de manutenção ou diagnóstico"},
        {"CONFIG", cmd_config, "Exibe e ajusta configuração do sistema"},
        {"SYSTEM CONFIG", cmd_config, "Configuração do sistema (alias)"},
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

    if (objeto == "WIFI") {
        printHelpFile("/help/wifi.txt");
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

static void printStatusFull(bool detailed) {
    console_println("=== STATUS DO SISTEMA ===");
    console_println("Modo..............: " + String(robotModeToString(getCurrentMode())));
    console_println("Interface.........: " + String(interfaceModeToString(getInterfaceMode())));
    console_println("Heap livre........: " + String(ESP.getFreeHeap()) + " bytes");
    console_println("Uptime............: " + String(millis() / 1000) + " s");

    if (detailed) {
        console_println("WiFi health.......: " + wifiGetHealthLabel());
        console_println("Clientes WiFi.....: " + String(WiFi.softAPgetStationNum()));
        console_println("MPU accel.........: " + String(getAccelX(), 2) + ", " + String(getAccelY(), 2) + ", " + String(getAccelZ(), 2) + " g");
        console_println("MPU gyro..........: " + String(getGyroX(), 2) + ", " + String(getGyroY(), 2) + ", " + String(getGyroZ(), 2) + " deg/s");
        console_println("Encoders..........: E=" + String(getPulsosEsq()) + " D=" + String(getPulsosDir()));
        console_println("Ultrassonico......: " + String(isUltraDistanceValid() ? getUltraDistanceCm() : 0.0f, 1) + " cm");
    }

    console_println("========================");
}

static void printSensorStatus() {
    console_println("=== SENSOR STATUS ===");
    console_println("MPU accel.........: " + String(getAccelX(), 2) + ", " + String(getAccelY(), 2) + ", " + String(getAccelZ(), 2) + " g");
    console_println("MPU gyro..........: " + String(getGyroX(), 2) + ", " + String(getGyroY(), 2) + ", " + String(getGyroZ(), 2) + " deg/s");
    console_println("Encoders..........: E=" + String(getPulsosEsq()) + " D=" + String(getPulsosDir()));
    console_println("Ultra.............: " + String(isUltraDistanceValid() ? getUltraDistanceCm() : 0.0f, 1) + " cm");
    console_println("====================");
}

static void printMotorStatus() {
    console_println("=== MOTOR STATUS ===");
    console_println("PWM min...........: " + String(PWM_MIN));
    console_println("PWM max...........: " + String(PWM_MAX));
    console_println("Modo atual........: " + String(robotModeToString(getCurrentMode())));
    console_println("====================");
}

static void printWifiStatus() {
    console_println("=== WIFI STATUS ===");
    console_println("SSID..............: " + WiFi.softAPSSID());
    console_println("IP................: " + WiFi.softAPIP().toString());
    console_println("Clientes..........: " + String(WiFi.softAPgetStationNum()));
    console_println("Saude.............: " + wifiGetHealthLabel());
    console_println("====================");
}

static void cmd_status(String args) {
    String entrada = args;
    entrada.trim();
    entrada.toUpperCase();

    if (entrada.length() == 0) {
        console_println("Sistema: OK");
        console_println("Modo atual: " + String(robotModeToString(getCurrentMode())));
        console_println("Interface atual: " + String(interfaceModeToString(getInterfaceMode())));
        console_println("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
        return;
    }

    if (entrada == "FULL" || entrada == "DETAIL" || entrada == "ALL") {
        printStatusFull(true);
        return;
    }

    if (entrada == "SENSOR") {
        printSensorStatus();
        return;
    }

    if (entrada == "MOTOR") {
        printMotorStatus();
        return;
    }

    if (entrada == "WIFI") {
        printWifiStatus();
        return;
    }

    console_println("STATUS inválido. Use: STATUS, STATUS FULL, STATUS SENSOR, STATUS MOTOR ou STATUS WIFI.");
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

static void cmd_enter_wifi(String args) {
    console_setState(STATE_WIFI);
    console_println("Entrando no menu WiFi...");
    console_println("Use HELP para listar os comandos de rede.");
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

    if (comando == "FILTER" || comando == "SEARCH") {
        if (restante.length() == 0) {
            console_println("Para filtrar use: LOG FILTER <texto>");
            return;
        }

        if (!ensureLittleFsMounted()) {
            return;
        }

        File file = LittleFS.open(LOG_FILE_PATH, "r");
        if (!file) {
            console_println("Log nao encontrado em /log.txt");
            return;
        }

        String texto = restante;
        texto.trim();
        texto.toUpperCase();

        console_println("Resultados do filtro: " + restante);
        bool found = false;
        String line = "";
        while (file.available()) {
            char c = (char)file.read();
            if (c == '\r') continue;
            if (c == '\n') {
                String upperLine = line;
                upperLine.toUpperCase();
                if (upperLine.indexOf(texto) >= 0) {
                    console_println(line);
                    found = true;
                }
                line = "";
                continue;
            }
            line += c;
        }

        if (line.length() > 0) {
            String upperLine = line;
            upperLine.toUpperCase();
            if (upperLine.indexOf(texto) >= 0) {
                console_println(line);
                found = true;
            }
        }

        if (!found) {
            console_println("Nenhuma linha corresponde ao filtro.");
        }

        file.close();
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

static void cmd_watch(String args) {
    String entrada = args;
    entrada.trim();
    entrada.toUpperCase();

    if (entrada.length() == 0) {
        entrada = "ALL";
    }

    if (entrada == "STOP" || entrada == "OFF" || entrada == "DISABLE") {
        console_stopWatch();
        console_println("[OK] WATCH desativado.");
        return;
    }

    if (entrada == "ALL" || entrada == "SYSTEM" || entrada == "SENSOR" || entrada == "MOTOR" || entrada == "WIFI") {
        console_startWatch(entrada);
        console_println("[OK] WATCH ativo para " + entrada + ".");
        return;
    }

    console_println("Uso: WATCH [ALL|SYSTEM|SENSOR|MOTOR|WIFI|STOP]");
}

static void printConfigUsage() {
    console_println("Uso do CONFIG:");
    console_println("CONFIG SHOW");
    console_println("CONFIG SAVE");
    console_println("CONFIG LOAD");
    console_println("CONFIG GET <GRUPO> <CHAVE>");
    console_println("CONFIG SET <GRUPO> <CHAVE> <VALOR>");
    console_println("CONFIG RESET");
}

static bool loadSystemConfigFromFile() {
    if (!ensureLittleFsMounted()) {
        return false;
    }

    File file = LittleFS.open(CONFIG_FILE_PATH, "r");
    if (!file) {
        return false;
    }

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (err) {
        return false;
    }

    if (doc.containsKey("wifi_ssid")) {
        systemConfig.wifiSsid = doc["wifi_ssid"].as<String>();
    }

    if (doc.containsKey("wifi_password")) {
        systemConfig.wifiPass = doc["wifi_password"].as<String>();
    }

    if (doc.containsKey("motor_pwm_min")) {
        systemConfig.motorPwmMin = doc["motor_pwm_min"].as<int>();
    }

    if (doc.containsKey("motor_pwm_max")) {
        systemConfig.motorPwmMax = doc["motor_pwm_max"].as<int>();
    }

    if (doc.containsKey("ultra_factor")) {
        systemConfig.ultraCalibrationFactor = doc["ultra_factor"].as<float>();
    }

    return true;
}

static bool saveSystemConfigToFile() {
    if (!ensureLittleFsMounted()) {
        return false;
    }

    StaticJsonDocument<256> doc;
    doc["wifi_ssid"] = systemConfig.wifiSsid;
    doc["wifi_password"] = systemConfig.wifiPass;
    doc["motor_pwm_min"] = systemConfig.motorPwmMin;
    doc["motor_pwm_max"] = systemConfig.motorPwmMax;
    doc["ultra_factor"] = systemConfig.ultraCalibrationFactor;

    File file = LittleFS.open(CONFIG_FILE_PATH, "w");
    if (!file) {
        return false;
    }

    bool ok = serializeJson(doc, file) > 0;
    file.close();
    return ok;
}

static void printConfigValues() {
    console_println("=== CONFIGURAÇÃO ATUAL ===");
    console_println("WiFi SSID.........: " + systemConfig.wifiSsid);
    console_println("WiFi Password.....: " + systemConfig.wifiPass);
    console_println("PWM Min...........: " + String(systemConfig.motorPwmMin));
    console_println("PWM Max...........: " + String(systemConfig.motorPwmMax));
    console_println("Ultra factor......: " + String(systemConfig.ultraCalibrationFactor, 4));
    console_println("Arquivo...........: " + String(CONFIG_FILE_PATH));
    console_println("=========================");
}

static void cmd_config(String args) {
    String entrada = args;
    entrada.trim();

    if (entrada.length() == 0 || entrada.equalsIgnoreCase("SHOW") || entrada.equalsIgnoreCase("INFO")) {
        printConfigValues();
        return;
    }

    String verb;
    String restante;
    splitCommandVerbAndArgs(entrada, verb, restante);
    verb.toUpperCase();

    if (verb == "LOAD") {
        if (loadSystemConfigFromFile()) {
            console_println("[OK] Configuração carregada de " + String(CONFIG_FILE_PATH));
        } else {
            console_println("[WARN] Arquivo de configuração não encontrado ou inválido.");
        }
        return;
    }

    if (verb == "SAVE") {
        if (saveSystemConfigToFile()) {
            console_println("[OK] Configuração salva em " + String(CONFIG_FILE_PATH));
        } else {
            console_println("[ERROR] Falha ao salvar configuração.");
        }
        return;
    }

    if (verb == "RESET" || verb == "DEFAULT") {
        systemConfig.wifiSsid = "ROBO_TCC";
        systemConfig.wifiPass = "12345678";
        systemConfig.motorPwmMin = PWM_MIN;
        systemConfig.motorPwmMax = PWM_MAX;
        systemConfig.ultraCalibrationFactor = 1.0f;
        if (saveSystemConfigToFile()) {
            console_println("[OK] Configuração resetada para defaults e salva.");
        } else {
            console_println("[OK] Configuração resetada para defaults.");
        }
        return;
    }

    if (verb == "GET") {
        String grupo = headToken(restante);
        String chave = tailToken(restante);
        grupo.toUpperCase();
        chave.toUpperCase();

        if (grupo == "WIFI" && chave == "SSID") {
            console_println("WIFI SSID = " + systemConfig.wifiSsid);
            return;
        }

        if (grupo == "WIFI" && chave == "PASSWORD") {
            console_println("WIFI PASSWORD = " + systemConfig.wifiPass);
            return;
        }

        if (grupo == "MOTOR" && chave == "PWMMIN") {
            console_println("MOTOR PWM_MIN = " + String(systemConfig.motorPwmMin));
            return;
        }

        if (grupo == "MOTOR" && chave == "PWMMAX") {
            console_println("MOTOR PWM_MAX = " + String(systemConfig.motorPwmMax));
            return;
        }

        if (grupo == "ULTRA" && chave == "FACTOR") {
            console_println("ULTRA FACTOR = " + String(systemConfig.ultraCalibrationFactor, 4));
            return;
        }

        console_println("Chave de configuração não reconhecida.");
        printConfigUsage();
        return;
    }

    if (verb == "SET") {
        String grupo = headToken(restante);
        String rest2 = tailToken(restante);
        String chave = headToken(rest2);
        String valor = tailToken(rest2);
        grupo.toUpperCase();
        chave.toUpperCase();

        if (grupo == "WIFI" && chave == "SSID") {
            systemConfig.wifiSsid = valor;
            saveSystemConfigToFile();
            console_println("[OK] WIFI SSID atualizado para: " + systemConfig.wifiSsid);
            return;
        }

        if (grupo == "WIFI" && chave == "PASSWORD") {
            systemConfig.wifiPass = valor;
            saveSystemConfigToFile();
            console_println("[OK] WIFI PASSWORD atualizado.");
            return;
        }

        if (grupo == "MOTOR" && chave == "PWMMIN") {
            systemConfig.motorPwmMin = valor.toInt();
            saveSystemConfigToFile();
            console_println("[OK] PWM_MIN atualizado para: " + String(systemConfig.motorPwmMin));
            return;
        }

        if (grupo == "MOTOR" && chave == "PWMMAX") {
            systemConfig.motorPwmMax = valor.toInt();
            saveSystemConfigToFile();
            console_println("[OK] PWM_MAX atualizado para: " + String(systemConfig.motorPwmMax));
            return;
        }

        if (grupo == "ULTRA" && chave == "FACTOR") {
            systemConfig.ultraCalibrationFactor = valor.toFloat();
            saveSystemConfigToFile();
            console_println("[OK] ULTRA FACTOR atualizado para: " + String(systemConfig.ultraCalibrationFactor, 4));
            return;
        }

        console_println("Grupo/chave de configuração não reconhecidos.");
        printConfigUsage();
        return;
    }

    console_println("Comando CONFIG inválido.");
    printConfigUsage();
}

static void cmd_run(String args) {
    String entrada = args;
    entrada.trim();
    entrada.toUpperCase();

    if (entrada.length() == 0) {
        console_println("Uso: RUN [DIAG|STATUS|SENSOR|MOTOR|WIFI|BOOT]");
        return;
    }

    if (entrada == "DIAG" || entrada == "DIAGNOSTIC") {
        executarDiagnostico();
        console_println("[OK] Rotina DIAG concluída.");
        return;
    }

    if (entrada == "STATUS" || entrada == "SYS") {
        printStatusFull(true);
        return;
    }

    if (entrada == "SENSOR") {
        printSensorStatus();
        return;
    }

    if (entrada == "MOTOR") {
        printMotorStatus();
        return;
    }

    if (entrada == "WIFI") {
        printWifiStatus();
        return;
    }

    if (entrada == "BOOT" || entrada == "CHECK") {
        printStatusFull(true);
        executarDiagnostico();
        console_println("[OK] Rotina BOOT concluída.");
        return;
    }

    if (entrada == "TEST_MOTOR" || entrada == "TESTMOTOR") {
        console_println("[RUN] Iniciando teste de motores...");
        motores_iniciarComando();
        moverFrente(180);
        delay(500);
        pararMotores();
        delay(200);
        moverTras(160);
        delay(500);
        pararMotores();
        delay(200);
        virarEsquerda(150);
        delay(450);
        pararMotores();
        delay(200);
        virarDireita(150);
        delay(450);
        pararMotores();
        motores_finalizarComando();
        console_println("[OK] Teste de motores concluído.");
        return;
    }

    console_println("Rotina RUN desconhecida. Use: DIAG, STATUS, SENSOR, MOTOR, WIFI, BOOT ou TEST_MOTOR.");
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
    console_println("LOG FILTER <texto>");
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
