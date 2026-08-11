// Implementação do núcleo do console
#include "console.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "../../sensores/sensores.h"
#include "../../sensores/sensor_manager.h"
#include "../../utils/logger.h"

// Tabelas registradas
static Command* mainTable = nullptr;
static size_t mainTableSize = 0;

static Command* motorTable = nullptr;
static size_t motorTableSize = 0;

static Command* sensorTable = nullptr;
static size_t sensorTableSize = 0;

// Estado e buffers
static ConsoleState state = STATE_MAIN;
static String lineBuffer = "";

// Streams
static bool mpuStream = false;
static bool ultraStream = false;
static bool encoderStream = false;
static unsigned long lastStreamMillis = 0;

// Console web
static String webConsoleBuffer = "";
static String webCommandBuffer = "";

// Forward
static void printBootStatus();
static void printPrompt();
static String toUpper(String s) { s.toUpperCase(); return s; }

void console_setMainCommands(Command* tabela, size_t tamanho) {
    mainTable = tabela;
    mainTableSize = tamanho;
}

void console_setMotorCommands(Command* tabela, size_t tamanho) {
    motorTable = tabela;
    motorTableSize = tamanho;
}

void console_setSensorCommands(Command* tabela, size_t tamanho) {
    sensorTable = tabela;
    sensorTableSize = tamanho;
}

void console_setState(ConsoleState s) {
    state = s;
}

void console_startMpuStream() { mpuStream = true; lastStreamMillis = millis(); }
void console_startUltraStream() { ultraStream = true; lastStreamMillis = millis(); }
void console_startEncoderStream() { encoderStream = true; lastStreamMillis = millis(); }
void console_stopStreams() { mpuStream = false; ultraStream = false; encoderStream = false; }

void console_printPrompt() { printPrompt(); }

void console_println(const String& line) {
    Serial.println(line);
    console_appendWebLine(line);
}

void console_clear() {
    for (int i = 0; i < 30; i++) {
        Serial.println();
    }
    webConsoleBuffer = "__CLEAR__\n";
}

void console_appendWebLine(const String& line) {
    if (line.length() == 0) return;
    webConsoleBuffer += line;
    webConsoleBuffer += "\n";
}

bool console_printTextFile(const char* path) {
    if (!path || path[0] == '\0') {
        return false;
    }

    if (!LittleFS.begin()) {
        console_println("Falha ao montar LittleFS para exibir ajuda.");
        return false;
    }

    File file = LittleFS.open(path, "r");
    if (!file) {
        console_println("Arquivo de ajuda nao encontrado.");
        return false;
    }

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
    return true;
}

String console_readWebCommand() {
    if (webCommandBuffer.length() == 0) return "";
    String cmd = webCommandBuffer;
    int idx = cmd.indexOf('\n');
    if (idx >= 0) {
        cmd = cmd.substring(0, idx);
        webCommandBuffer = webCommandBuffer.substring(idx + 1);
    } else {
        webCommandBuffer = "";
    }
    return cmd;
}

void console_queueWebCommand(const String& cmd) {
    if (cmd.length() == 0) return;
    webCommandBuffer += cmd;
    webCommandBuffer += "\n";
}

void console_submitCommand(const String& cmd) {
    console_queueWebCommand(cmd);
}

String console_getAndClearWebBuffer() {
    String out = webConsoleBuffer;
    webConsoleBuffer = "";
    return out;
}

// Executa um comando contra uma tabela específica
static bool executarComandoDaTabela(Command* tabela, size_t tamanho, String entrada) {
    if (!tabela || tamanho == 0) return false;

    entrada.trim();
    if (entrada.length() == 0) return false;

    int esp = entrada.indexOf(' ');
    String token;
    String args;

    if (esp == -1) {
        token = toUpper(entrada);
        args = "";
    } else {
        token = toUpper(entrada.substring(0, esp));
        args = entrada.substring(esp + 1);
        args.trim();
    }

    for (size_t i = 0; i < tamanho; ++i) {
        String nome(tabela[i].nome);
        if (nome.indexOf(' ') != -1) {
            String entUp = toUpper(entrada);
            if (entUp.startsWith(nome)) {
                String rest = entrada.substring(nome.length());
                rest.trim();
                tabela[i].func(rest);
                return true;
            }
        } else {
            if (token == nome) {
                tabela[i].func(args);
                return true;
            }
        }
    }

    return false;
}

// Imprime prompt e cabeçalho
static void printPrompt() {
    String prompt;
    switch (state) {
        case STATE_MAIN: prompt = "ROBO> "; break;
        case STATE_MOTOR: prompt = "MOTOR> "; break;
        case STATE_SENSOR: prompt = "SENSOR> "; break;
    }
    Serial.print(prompt);
    console_appendWebLine(prompt);
}

static void printBootStatus() {
    console_println("=================================================");
    console_println(" ROBÔ EDUCACIONAL");
    console_println(" Console de Diagnóstico e Manutenção");
    console_println(" Firmware 2.0");
    console_println("=================================================");
    console_println("");

    console_println("Inicializando sistema...");
    console_println("[    ] Logger [ OK ]");
    console_println("[    ] Motores [ OK ]");
    console_println("[    ] Controle [ OK ]");
    console_println("[    ] Encoders [ OK ]");

    if (getAccelX() == 0 && getAccelY() == 0 && getAccelZ() == 0) {
        console_println("[    ] MPU6050 [FAIL]");
    } else {
        console_println("[    ] MPU6050 [ OK ]");
    }

    IPAddress ip = WiFi.softAPIP();
    if (ip != IPAddress(0,0,0,0)) {
        console_println("[    ] WiFi [ OK ]");
    } else {
        console_println("[    ] WiFi [FAIL]");
    }

    if (LittleFS.begin()) {
        console_println("[    ] LittleFS [ OK ]");
    } else {
        console_println("[    ] LittleFS [FAIL]");
    }

    console_println("");
    console_println("Sistema inicializado.");
    console_println("");
    console_println("Digite HELP para listar os comandos.");
    console_println("");
}

void console_init() {
    printBootStatus();
    printPrompt();
}

static void executarComando(String cmd) {
    if (cmd.length() == 0) return;

    Serial.println();
    console_appendWebLine(cmd);
    String linha = cmd;
    linha.trim();
    String up = toUpper(linha);

    // Interrupção global de stream em qualquer estado.
    if (up == "STOP STREAM" || up == "STREAM STOP") {
        console_stopStreams();
        console_println("[OK] STREAM interrompido.");
        printPrompt();
        return;
    }

    // Comandos qualificados funcionam em qualquer estado.
    if (up.startsWith("MOTOR ")) {
        String sub = linha.substring(6);
        sub.trim();
        if (sub.length() > 0 && executarComandoDaTabela(motorTable, motorTableSize, sub)) {
            printPrompt();
            return;
        }
    }

    if (up.startsWith("SENSOR ")) {
        String sub = linha.substring(7);
        sub.trim();
        if (sub.length() > 0 && executarComandoDaTabela(sensorTable, sensorTableSize, sub)) {
            printPrompt();
            return;
        }
    }

    if (up.startsWith("SYSTEM ")) {
        if (executarComandoDaTabela(mainTable, mainTableSize, linha)) {
            printPrompt();
            return;
        }
    }

    bool executado = false;

    switch (state) {
        case STATE_MAIN:
            executado = executarComandoDaTabela(mainTable, mainTableSize, linha);
            break;
        case STATE_MOTOR:
            executado = executarComandoDaTabela(motorTable, motorTableSize, linha);
            break;
        case STATE_SENSOR:
            executado = executarComandoDaTabela(sensorTable, sensorTableSize, linha);
            break;
    }

    // Fallback global: se não reconhecer no estado atual, tenta nas outras tabelas.
    if (!executado) {
        executado = executarComandoDaTabela(mainTable, mainTableSize, linha)
            || executarComandoDaTabela(motorTable, motorTableSize, linha)
            || executarComandoDaTabela(sensorTable, sensorTableSize, linha);
    }

    if (!executado) {
        console_println("Comando não reconhecido. Digite HELP.");
    }

    printPrompt();
}

void console_loop() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            String cmd = lineBuffer;
            lineBuffer = "";
            executarComando(cmd);
        }
        else if (c == 8 || c == 127) {
            if (lineBuffer.length() > 0) {
                lineBuffer.remove(lineBuffer.length() - 1);
                Serial.print("\b \b");
            }
        }
        else {
            if (c >= 32 && c <= 126) {
                lineBuffer += c;
                Serial.print(c);
            }
        }
    }

    String webCmd = console_readWebCommand();
    while (webCmd.length() > 0) {
        executarComando(webCmd);
        webCmd = console_readWebCommand();
    }

    if (mpuStream && millis() - lastStreamMillis > 200) {
        lastStreamMillis = millis();
        console_println("Accel");
        console_println("X = " + String(getAccelX(), 3) + " g");
        console_println("Y = " + String(getAccelY(), 3) + " g");
        console_println("Z = " + String(getAccelZ(), 3) + " g");
        console_println("Gyro");
        console_println("X = " + String(getGyroX(), 3) + " °/s");
        console_println("Y = " + String(getGyroY(), 3) + " °/s");
        console_println("Z = " + String(getGyroZ(), 3) + " °/s");
    }

    if (ultraStream && millis() - lastStreamMillis > 200) {
        lastStreamMillis = millis();
        console_println("Distância");
        if (isUltraDistanceValid()) {
            console_println(String(getUltraDistanceCm(), 1) + " cm");
        } else {
            console_println("N/A");
        }
    }

    if (encoderStream && millis() - lastStreamMillis > 200) {
        lastStreamMillis = millis();
        console_println("Encoder");
        console_println("Esquerdo = " + String(getPulsosEsq()));
        console_println("Direito = " + String(getPulsosDir()));
    }
}
