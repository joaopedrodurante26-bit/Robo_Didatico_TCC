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

#endif
