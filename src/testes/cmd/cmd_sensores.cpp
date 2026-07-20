#include "cmd_sensores.h"

#include "../../sensores/sensores.h"

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

static void cmd_sensor_mpu_stream(String args) { console_startMpuStream(); }
static void cmd_sensor_encoder_read(String args) {
    Serial.println("Encoder esquerdo\n");
    Serial.println("Pulsos = " + String(getPulsosEsq()));
    Serial.println();
    Serial.println("Encoder direito\n");
    Serial.println("Pulsos = " + String(getPulsosDir()));
}

static void cmd_sensor_encoder_reset(String args) { resetEncoders(); Serial.println("Encoders zerados."); }
static void cmd_sensor_ultra_read(String args) { Serial.println("Distância\n"); Serial.println(String(getDistancia()) + " cm"); }
static void cmd_sensor_ultra_stream(String args) { console_startUltraStream(); }
static void cmd_sensor_stop(String args) { console_stopStreams(); }
static void cmd_sensor_exit(String args) { console_setState(STATE_MAIN); }
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

Command* getSensorCommands(size_t &count) {
    count = sizeof(comandosSensor)/sizeof(Command);
    return comandosSensor;
}
