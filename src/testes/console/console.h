// Núcleo do console: parser, máquina de estados, I/O serial e streaming
#ifndef CONSOLE_H
#define CONSOLE_H

#include <Arduino.h>

typedef void (*CmdFn)(String args);

struct Command {
    const char* nome;
    CmdFn func;
    const char* descricao;
};

// Estados públicos (usados por módulos de comando)
enum ConsoleState {
    STATE_MAIN,
    STATE_MOTOR,
    STATE_SENSOR
};

// Inicializa o console (imprime boot/status)
void console_init();

// Deve ser chamado no loop() para processar entrada serial
void console_loop();

// Registro de tabelas de comando por estado
void console_setMainCommands(Command* tabela, size_t tamanho);
void console_setMotorCommands(Command* tabela, size_t tamanho);
void console_setSensorCommands(Command* tabela, size_t tamanho);

// Funções auxiliares utilizadas por módulos de comando
void console_setState(ConsoleState s);
void console_startMpuStream();
void console_startUltraStream();
void console_stopStreams();

// Força reimpressão do prompt (útil após saída das rotinas)
void console_printPrompt();

// Saída unificada para serial + console web
void console_println(const String& line);

// Limpa terminal serial e sinaliza limpeza para o console web
void console_clear();

// Envia uma linha para o console web, se disponível
void console_appendWebLine(const String& line);

// Lê a próxima linha do console web, se houver
String console_readWebCommand();

// Enfileira um comando vindo da interface web
void console_queueWebCommand(const String& cmd);

// Submete comando para o interpretador central
void console_submitCommand(const String& cmd);

// Retorna o conteúdo atual do console web e limpa o buffer
String console_getAndClearWebBuffer();

#endif
