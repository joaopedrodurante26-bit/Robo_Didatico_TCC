// Implementação do núcleo do console
#include "console.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "../../sensores/sensores.h"
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
static unsigned long lastStreamMillis = 0;

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
void console_stopStreams() { mpuStream = false; ultraStream = false; }

void console_printPrompt() { printPrompt(); }

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
    switch (state) {
        case STATE_MAIN: Serial.print("ROBO> "); break;
        case STATE_MOTOR: Serial.print("MOTOR> "); break;
        case STATE_SENSOR: Serial.print("SENSOR> "); break;
    }
}

static void printBootStatus() {
    Serial.println("=================================================");
    Serial.println(" ROBÔ EDUCACIONAL");
    Serial.println(" Console de Diagnóstico e Manutenção");
    Serial.println(" Firmware 1.0");
    Serial.println("=================================================");
    Serial.println();

    Serial.println("Inicializando sistema...");
    Serial.print("[    ] Logger "); Serial.println("[ OK ]");
    Serial.print("[    ] Motores "); Serial.println("[ OK ]");
    Serial.print("[    ] Controle "); Serial.println("[ OK ]");
    Serial.print("[    ] Encoders "); Serial.println("[ OK ]");

    Serial.print("[    ] MPU6050 ");
    if (getAccelX() == 0 && getAccelY() == 0 && getAccelZ() == 0) Serial.println("[FAIL]"); else Serial.println("[ OK ]");

    Serial.print("[    ] WiFi "); IPAddress ip = WiFi.softAPIP(); if (ip != IPAddress(0,0,0,0)) Serial.println("[ OK ]"); else Serial.println("[FAIL]");

    Serial.print("[    ] LittleFS "); if (LittleFS.begin()) Serial.println("[ OK ]"); else Serial.println("[FAIL]");

    Serial.println();
    Serial.println("Sistema inicializado.");
    Serial.println();
    Serial.println("Digite HELP para listar os comandos.");
    Serial.println();
}

void console_init() {
    printBootStatus();
    printPrompt();
}

void console_loop() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            Serial.println();
            String cmd = lineBuffer;
            lineBuffer = "";
            if (cmd.length() > 0) {
                switch (state) {
                    case STATE_MAIN:
                        if (!executarComandoDaTabela(mainTable, mainTableSize, cmd)) Serial.println("Comando não reconhecido. Digite HELP.");
                        break;
                    case STATE_MOTOR:
                        if (!executarComandoDaTabela(motorTable, motorTableSize, cmd)) Serial.println("Comando MOTOR não reconhecido. Digite HELP.");
                        break;
                    case STATE_SENSOR:
                        if (!executarComandoDaTabela(sensorTable, sensorTableSize, cmd)) Serial.println("Comando SENSOR não reconhecido. Digite HELP.");
                        break;
                }
            }
            printPrompt();
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

    if (mpuStream && millis() - lastStreamMillis > 200) {
        lastStreamMillis = millis();
        Serial.println("Accel");
        Serial.println("X = " + String(getAccelX(), 3) + " g");
        Serial.println("Y = " + String(getAccelY(), 3) + " g");
        Serial.println("Z = " + String(getAccelZ(), 3) + " g");
        Serial.println("Gyro");
        Serial.println("X = " + String(getGyroX(), 3) + " °/s");
        Serial.println("Y = " + String(getGyroY(), 3) + " °/s");
        Serial.println("Z = " + String(getGyroZ(), 3) + " °/s");
    }

    if (ultraStream && millis() - lastStreamMillis > 200) {
        lastStreamMillis = millis();
        Serial.println("Distância");
        Serial.println(String(getDistancia()) + " cm");
    }
}
