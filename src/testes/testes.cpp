// Console de diagnóstico e manutenção (implementação)
// Baseado nas especificações do arquivo prompt.txt em src/testes/
// Fornece um prompt via Serial, submenus por subsistema e comandos de teste.

#include "testes.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "../motores/motores.h"
#include "../sensores/sensores.h"
#include "../diagnostico/diagnostico.h"
#include "../utils/logger.h"

// =====================================================
// Máquina de estados do console
// =====================================================
enum ConsoleState {
	STATE_MAIN,
	STATE_MOTOR,
	STATE_SENSOR
};

static ConsoleState state = STATE_MAIN;

// Buffer de entrada serial
static String lineBuffer = "";

// Flags de stream (por exemplo: MPU, ULTRA)
static bool mpuStream = false;
static bool ultraStream = false;
static unsigned long lastStreamMillis = 0;

// ==================================
// Helpers
// ==================================

// Converte string para maiúsculas (in-place)
static String toUpper(String s) {
	s.toUpperCase();
	return s;
}

// Imprime o prompt atual
static void printPrompt() {
	switch (state) {
		case STATE_MAIN:
			Serial.print("ROBO> ");
			break;
		case STATE_MOTOR:
			Serial.print("MOTOR> ");
			break;
		case STATE_SENSOR:
			Serial.print("SENSOR> ");
			break;
	}
}

// Imprime cabeçalho inicial com status dos módulos (boot)
static void printBootStatus() {
	Serial.println("=========================================================");
	Serial.println(" ROBÔ EDUCACIONAL");
	Serial.println(" Console de Diagnóstico e Manutenção");
	Serial.println(" Firmware 1.0");
	Serial.println("=========================================================");
	Serial.println();

	Serial.println("Inicializando sistema...");

	// Logger
	Serial.print("[    ] Logger ");
	// assumimos que initLogger foi chamado antes
	Serial.println("[ OK ]");

	// Motores
	Serial.print("[    ] Motores ");
	Serial.println("[ OK ]");

	// Controle
	Serial.print("[    ] Controle ");
	Serial.println("[ OK ]");

	// Encoders
	Serial.print("[    ] Encoders ");
	Serial.println("[ OK ]");

	// IMU (MPU6050)
	Serial.print("[    ] MPU6050 ");
	// checagem simples: valores diferentes de zero indicam leitura
	if (getAccelX() == 0 && getAccelY() == 0 && getAccelZ() == 0) {
		Serial.println("[FAIL]");
	} else {
		Serial.println("[ OK ]");
	}

	// WiFi
	Serial.print("[    ] WiFi ");
	IPAddress ip = WiFi.softAPIP();
	if (ip != IPAddress(0,0,0,0)) Serial.println("[ OK ]"); else Serial.println("[FAIL]");

	// LittleFS
	Serial.print("[    ] LittleFS ");
	if (LittleFS.begin()) {
		Serial.println("[ OK ]");
	} else {
		Serial.println("[FAIL]");
	}

	Serial.println();
	Serial.println("Sistema inicializado.");
	Serial.println();
	Serial.println("Digite HELP para listar os comandos.");
	Serial.println();
}

// ==================================
// Abordagem baseada em tabela de comandos
// Definimos uma estrutura simples para mapear nomes a funções
// Isso evita cadeias longas de if/else e facilita extensão.
// ==================================

typedef void (*CmdFn)(String args);

struct Command {
	const char* nome; // comando em MAIÚSCULAS
	CmdFn func;
	const char* descricao;
};

// ---------- PROTÓTIPOS DAS FUNÇÕES DE COMANDO ----------
// Principais
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

// Motor
static void cmd_motor_f(String args);
static void cmd_motor_t(String args);
static void cmd_motor_ve(String args);
static void cmd_motor_vd(String args);
static void cmd_motor_e(String args);
static void cmd_motor_d(String args);
static void cmd_motor_stop(String args);
static void cmd_motor_exit(String args);
static void cmd_motor_help(String args);

// Sensor
static void cmd_sensor_mpu_read(String args);
static void cmd_sensor_mpu_stream(String args);
static void cmd_sensor_encoder_read(String args);
static void cmd_sensor_encoder_reset(String args);
static void cmd_sensor_ultra_read(String args);
static void cmd_sensor_ultra_stream(String args);
static void cmd_sensor_stop(String args);
static void cmd_sensor_exit(String args);
static void cmd_sensor_help(String args);

// ---------- TABELAS DE COMANDOS ----------
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

static Command comandosMotor[] = {
	{"F", cmd_motor_f, "Move ambos para frente"},
	{"T", cmd_motor_t, "Move ambos para trás"},
	{"VE", cmd_motor_ve, "Vira à esquerda"},
	{"VD", cmd_motor_vd, "Vira à direita"},
	{"E", cmd_motor_e, "Controla lado esquerdo"},
	{"D", cmd_motor_d, "Controla lado direito"},
	{"STOP", cmd_motor_stop, "Interrompe motores"},
	{"EXIT", cmd_motor_exit, "Volta ao menu principal"},
	{"HELP", cmd_motor_help, "Ajuda do menu motores"},
};

static Command comandosSensor[] = {
	{"MPU READ", cmd_sensor_mpu_read, "Leituras IMU"},
	{"MPU STREAM", cmd_sensor_mpu_stream, "Fluxo contínuo IMU"},
	{"ENCODER READ", cmd_sensor_encoder_read, "Pulsos dos encoders"},
	{"ENCODER RESET", cmd_sensor_encoder_reset, "Zerar encoders"},
	{"ULTRA READ", cmd_sensor_ultra_read, "Leitura única ultrassônico"},
	{"ULTRA STREAM", cmd_sensor_ultra_stream, "Fluxo contínuo ultrassônico"},
	{"STOP", cmd_sensor_stop, "Para fluxos"},
	{"EXIT", cmd_sensor_exit, "Volta ao menu principal"},
	{"HELP", cmd_sensor_help, "Ajuda do menu sensores"},
};

// ---------- FUNÇÃO AUXILIAR: procura comando na tabela ----------
static bool executarComandoDaTabela(Command* tabela, size_t tamanho, String entrada) {
	entrada.trim();
	if (entrada.length() == 0) return false;

	// separa token (até primeiro espaço) e args (resto)
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

	// Procura na tabela (caso exato ou prefixo para comandos com espaço)
	for (size_t i = 0; i < tamanho; ++i) {
		String nome(tabela[i].nome);
		// para comandos que incluem espaço no nome (ex: "MPU READ") comparamos a entrada inteira em maiúsculas
		if (nome.indexOf(' ') != -1) {
			// compara início da entrada com nome completo
			String entUp = toUpper(entrada);
			if (entUp.startsWith(nome)) {
				// args serão o que vem após o nome
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

// ---------- HANDLE MAIN: usa tabela ----------
static void handleMainCommand(String cmd) {
	bool ok = executarComandoDaTabela(comandosMain, sizeof(comandosMain)/sizeof(Command), cmd);
	if (!ok) {
		Serial.println("Comando não reconhecido. Digite HELP.");
	}
}

// ==================================
// Implementações das funções de comando
// ==================================

// ---------- MAIN ----------
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
	state = STATE_MOTOR;
	Serial.println("====================================");
	Serial.println(" TESTE DOS MOTORES");
	Serial.println("====================================");
	Serial.println();
	Serial.println("Comandos disponíveis");
	Serial.println();
	Serial.println("F <velocidade>");
	Serial.println("T <velocidade>");
	Serial.println("VE <velocidade>");
	Serial.println("VD <velocidade>");
	Serial.println("E <velocidade>");
	Serial.println("D <velocidade>");
	Serial.println("STOP");
	Serial.println("EXIT");
}

static void cmd_enter_sensor(String args) {
	state = STATE_SENSOR;
	Serial.println("Entrando no menu de sensores. Digite HELP para ver comandos.");
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

// ---------- MOTOR ----------
static void cmd_motor_f(String args) {
	int v = args.toInt();
	moverFrente(v);
}

static void cmd_motor_t(String args) {
	int v = args.toInt();
	moverTras(v);
}

static void cmd_motor_ve(String args) {
	int v = args.toInt();
	virarEsquerda(v);
}

static void cmd_motor_vd(String args) {
	int v = args.toInt();
	virarDireita(v);
}

static void cmd_motor_e(String args) {
	int v = args.toInt();
	setVelocidade(v, 0);
}

static void cmd_motor_d(String args) {
	int v = args.toInt();
	setVelocidade(0, v);
}

static void cmd_motor_stop(String args) {
	pararMotores();
}

static void cmd_motor_exit(String args) {
	pararMotores();
	state = STATE_MAIN;
}

static void cmd_motor_help(String args) {
	Serial.println("Comandos Motores: F T VE VD E D STOP EXIT");
}

// ---------- SENSOR ----------
static void cmd_sensor_mpu_read(String args) {
	Serial.println("Accel\n");
	Serial.println("X = " + String(getAccelX(), 3) + " g");
	Serial.println("Y = " + String(getAccelY(), 3) + " g");
	Serial.println("Z = " + String(getAccelZ(), 3) + " g");
	Serial.println();
	Serial.println("Gyro\n");
	Serial.println("X = " + String(getGyroX(), 3) + " °/s");
	Serial.println("Y = " + String(getGyroY(), 3) + " °/s");
	Serial.println("Z = " + String(getGyroZ(), 3) + " °/s");
}

static void cmd_sensor_mpu_stream(String args) {
	mpuStream = true;
	lastStreamMillis = millis();
}

static void cmd_sensor_encoder_read(String args) {
	Serial.println("Encoder esquerdo\n");
	Serial.println("Pulsos = " + String(getPulsosEsq()));
	Serial.println();
	Serial.println("Encoder direito\n");
	Serial.println("Pulsos = " + String(getPulsosDir()));
}

static void cmd_sensor_encoder_reset(String args) {
	resetEncoders();
	Serial.println("Encoders zerados.");
}

static void cmd_sensor_ultra_read(String args) {
	Serial.println("Distância\n");
	Serial.println(String(getDistancia()) + " cm");
}

static void cmd_sensor_ultra_stream(String args) {
	ultraStream = true;
	lastStreamMillis = millis();
}

static void cmd_sensor_stop(String args) {
	mpuStream = false;
	ultraStream = false;
}

static void cmd_sensor_exit(String args) {
	state = STATE_MAIN;
}

static void cmd_sensor_help(String args) {
	Serial.println("Comandos sensores:");
	Serial.println("MPU READ - leituras IMU");
	Serial.println("MPU STREAM - fluxo contínuo (STOP para parar)");
	Serial.println("ENCODER READ - pulsos dos encoders");
	Serial.println("ENCODER RESET - zerar encoders");
	Serial.println("ULTRA READ - distância única");
	Serial.println("ULTRA STREAM - fluxo contínuo");
	Serial.println("EXIT - voltar ao menu principal");
}

// ==================================
// Comandos do menu MOTOR
// ==================================
static void handleMotorCommand(String cmd) {
	cmd.trim();
	String u = toUpper(cmd);

	if (u.startsWith("F ")) {
		int v = cmd.substring(2).toInt();
		moverFrente(v);
	}
	else if (u.startsWith("T ")) {
		int v = cmd.substring(2).toInt();
		moverTras(v);
	}
	else if (u.startsWith("VE ")) {
		int v = cmd.substring(3).toInt();
		virarEsquerda(v);
	}
	else if (u.startsWith("VD ")) {
		int v = cmd.substring(3).toInt();
		virarDireita(v);
	}
	else if (u.startsWith("E ")) {
		int v = cmd.substring(2).toInt();
		setVelocidade(v, 0);
	}
	else if (u.startsWith("D ")) {
		int v = cmd.substring(2).toInt();
		setVelocidade(0, v);
	}
	else if (u == "STOP") {
		pararMotores();
	}
	else if (u == "EXIT") {
		pararMotores();
		state = STATE_MAIN;
	}
	else if (u == "HELP") {
		Serial.println("Comandos Motores: F T VE VD E D STOP EXIT");
	}
	else {
		Serial.println("Comando MOTOR não reconhecido");
	}
}

// ==================================
// Comandos do menu SENSOR
// ==================================
static void handleSensorCommand(String cmd) {
	cmd.trim();
	String u = toUpper(cmd);

	if (u == "HELP") {
		Serial.println("Comandos sensores:");
		Serial.println("MPU READ - leituras IMU");
		Serial.println("MPU STREAM - fluxo contínuo (STOP para parar)");
		Serial.println("ENCODER READ - pulsos dos encoders");
		Serial.println("ENCODER RESET - zerar encoders");
		Serial.println("ULTRA READ - distância única");
		Serial.println("ULTRA STREAM - fluxo contínuo");
		Serial.println("EXIT - voltar ao menu principal");
	}
	else if (u == "MPU READ") {
		Serial.println("Accel\n");
		Serial.println("X = " + String(getAccelX(), 3) + " g");
		Serial.println("Y = " + String(getAccelY(), 3) + " g");
		Serial.println("Z = " + String(getAccelZ(), 3) + " g");
		Serial.println();
		Serial.println("Gyro\n");
		Serial.println("X = " + String(getGyroX(), 3) + " °/s");
		Serial.println("Y = " + String(getGyroY(), 3) + " °/s");
		Serial.println("Z = " + String(getGyroZ(), 3) + " °/s");
	}
	else if (u == "MPU STREAM") {
		mpuStream = true;
		lastStreamMillis = millis();
	}
	else if (u == "ENCODER READ") {
		Serial.println("Encoder esquerdo\n");
		Serial.println("Pulsos = " + String(getPulsosEsq()));
		Serial.println();
		Serial.println("Encoder direito\n");
		Serial.println("Pulsos = " + String(getPulsosDir()));
	}
	else if (u == "ENCODER RESET") {
		resetEncoders();
		Serial.println("Encoders zerados.");
	}
	else if (u == "ULTRA READ") {
		Serial.println("Distância\n");
		Serial.println(String(getDistancia()) + " cm");
	}
	else if (u == "ULTRA STREAM") {
		ultraStream = true;
		lastStreamMillis = millis();
	}
	else if (u == "STOP") {
		mpuStream = false;
		ultraStream = false;
	}
	else if (u == "EXIT") {
		state = STATE_MAIN;
	}
	else {
		Serial.println("Comando SENSOR não reconhecido. Digite HELP.");
	}
}

// ==================================
// Inicialização do console (setup)
// ==================================
void testes_iniciar() {
	// Exibe informações iniciais e status dos módulos
	printBootStatus();
	printPrompt();
}

// ==================================
// Atualização chamada no loop()
// - Processa entrada Serial
// - Executa streams periódicos
// ==================================
void atualizarTestes() {
	// Leitura serial (linha por linha) com ECO (mostra o que o usuário digita)
	while (Serial.available()) {
		char c = Serial.read();

		if (c == '\r') continue; // ignora CR

		// ENTER: ecoa nova linha e processa o comando
		if (c == '\n') {
			Serial.println(); // ecoa o Enter para o monitor

			String cmd = lineBuffer;
			lineBuffer = "";

			if (cmd.length() > 0) {
				switch (state) {
					case STATE_MAIN:
						handleMainCommand(cmd);
						break;
					case STATE_MOTOR:
						if (!executarComandoDaTabela(comandosMotor, sizeof(comandosMotor)/sizeof(Command), cmd)) {
							Serial.println("Comando MOTOR não reconhecido. Digite HELP.");
						}
						break;
					case STATE_SENSOR:
						if (!executarComandoDaTabela(comandosSensor, sizeof(comandosSensor)/sizeof(Command), cmd)) {
							Serial.println("Comando SENSOR não reconhecido. Digite HELP.");
						}
						break;
				}
			}

			printPrompt();
		}
		// BACKSPACE / DEL: remove último caractere do buffer e apaga no terminal
		else if (c == 8 || c == 127) {
			if (lineBuffer.length() > 0) {
				lineBuffer.remove(lineBuffer.length() - 1);
				// Sequência clássica para apagar caractere no terminal
				Serial.print("\b \b");
			}
		}
		// Caracter imprimível: adiciona ao buffer e ecoa
		else {
			if (c >= 32 && c <= 126) {
				lineBuffer += c;
				Serial.print(c); // ECO local
			}
		}
	}

	// Streams periódicos (exemplo: MPU e ULTRA)
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

