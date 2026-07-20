// Testes: registra módulos de comando e delega ao núcleo do console
#include "testes.h"

#include "console/console.h"
#include "cmd/cmd_main.h"
#include "cmd/cmd_motores.h"
#include "cmd/cmd_sensores.h"

void testes_iniciar() {
	size_t n;

	Command* main = getMainCommands(n);
	console_setMainCommands(main, n);

	Command* mot = getMotorCommands(n);
	console_setMotorCommands(mot, n);

	Command* sen = getSensorCommands(n);
	console_setSensorCommands(sen, n);

	console_init();
}

void atualizarTestes() {
	console_loop();
}

