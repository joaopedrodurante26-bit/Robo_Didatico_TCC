#include "cmd_sensores.h"

#include "../../sensores/sensores.h"
#include "../../sensores/sensor_manager.h"

static const char* ultraStatusToText(UltrasonicStatus status) {
    switch (status) {
        case ULTRA_OK: return "OK";
        case ULTRA_TIMEOUT: return "TIMEOUT";
        case ULTRA_ECHO_TOO_SHORT: return "ECHO TOO SHORT";
        case ULTRA_ECHO_TOO_LONG: return "ECHO TOO LONG";
        case ULTRA_OUT_OF_RANGE: return "OUT OF RANGE";
        case ULTRA_INVALID_READING: return "INVALID READING";
        case ULTRA_SENSOR_ERROR: return "SENSOR ERROR";
        default: return "UNKNOWN";
    }
}

static void cmd_sensor_mpu_read(String args) {
    console_println("Accel");
    console_println("");
    console_println("X = " + String(getAccelX(), 3) + " g");
    console_println("Y = " + String(getAccelY(), 3) + " g");
    console_println("Z = " + String(getAccelZ(), 3) + " g");
    console_println("");
    console_println("Gyro");
    console_println("");
    console_println("X = " + String(getGyroX(), 3) + " °/s");
    console_println("Y = " + String(getGyroY(), 3) + " °/s");
    console_println("Z = " + String(getGyroZ(), 3) + " °/s");
}

static void cmd_sensor_mpu_stream(String args) { console_startMpuStream(); console_println("[OK] STREAM MPU iniciado. Use STOP ou STOP STREAM."); }
static void cmd_sensor_encoder_read(String args) {
    console_println("Encoder esquerdo");
    console_println("");
    console_println("Pulsos = " + String(getPulsosEsq()));
    console_println("");
    console_println("Encoder direito");
    console_println("");
    console_println("Pulsos = " + String(getPulsosDir()));
}

static void cmd_sensor_encoder_reset(String args) { resetEncoders(); console_println("Encoders zerados."); }
static void cmd_sensor_ultra_raw(String args) {
    console_println("HC-SR04");
    console_println("");
    console_println("Trigger............." + String(ultraTriggerOk() ? "OK" : "FAIL"));
    console_println("Echo................" + String(ultraEchoOk() ? "OK" : "FAIL"));
    console_println("Tempo..............." + String(getUltraEchoTimeUs()) + " us");
}

static void cmd_sensor_ultra_status(String args) {
    console_println("HC-SR04");
    console_println("");
    console_println("Estado.............." + String(ultraStatusToText(getUltraStatus())));
    console_println("Sensor.............." + String(ultraSensorPresente() ? "PRESENTE" : "AUSENTE"));
    if (getUltraEchoTimeUs() > 0) {
        console_println("Distancia..........." + String(getUltraDistanceCm(), 1) + " cm");
    } else {
        console_println("Distancia...........N/A");
    }
    console_println("Leituras............" + String(getUltraReadCount()));
    console_println("Timeouts............" + String(getUltraTimeoutCount()));
}

static void cmd_sensor_ultra_read(String args) {
    console_println("HC-SR04");
    console_println("");
    console_println("Estado.............." + String(ultraStatusToText(getUltraStatus())));
    console_println("Tempo..............." + String(getUltraEchoTimeUs()) + " us");
}

static void cmd_sensor_ultra_dist(String args) {
    console_println("HC-SR04");
    console_println("");
    console_println("Tempo..............." + String(getUltraEchoTimeUs()) + " us");
    if (isUltraDistanceValid()) {
        console_println("Distancia..........." + String(getUltraDistanceCm(), 1) + " cm");
    } else {
        console_println("Distancia...........N/A");
    }
}

static void cmd_sensor_ultra_stream(String args) { console_startUltraStream(); console_println("[OK] STREAM ULTRA iniciado. Use STOP ou STOP STREAM."); }
static void cmd_sensor_stop(String args) { console_stopStreams(); console_println("[OK] STREAM interrompido."); }
static void cmd_sensor_exit(String args) { console_setState(STATE_MAIN); }
static void cmd_sensor_help(String args) {
    console_println("Comandos sensores:");
    console_println("MPU READ - leituras IMU");
    console_println("MPU STREAM - fluxo contínuo (STOP para parar)");
    console_println("ENCODER READ - pulsos dos encoders");
    console_println("ENCODER RESET - zerar encoders");
    console_println("ULTRA READ - estado + tempo de eco");
    console_println("ULTRA DIST - distancia convertida (cm)");
    console_println("ULTRA RAW - valida trigger/echo + tempo");
    console_println("ULTRA STATUS - status e estatísticas");
    console_println("ULTRA STREAM - fluxo contínuo");
    console_println("EXIT - voltar ao menu principal");
}

static Command comandosSensor[] = {
    {"MPU READ", cmd_sensor_mpu_read, "Leituras IMU"},
    {"READ MPU", cmd_sensor_mpu_read, "Leituras IMU (alias)"},
    {"MPU STREAM", cmd_sensor_mpu_stream, "Fluxo contínuo IMU"},
    {"STREAM MPU", cmd_sensor_mpu_stream, "Fluxo contínuo IMU (alias)"},
    {"STOP MPU", cmd_sensor_stop, "Para stream de MPU"},
    {"ENCODER READ", cmd_sensor_encoder_read, "Pulsos dos encoders"},
    {"READ ENCODER", cmd_sensor_encoder_read, "Pulsos dos encoders (alias)"},
    {"ENCODER RESET", cmd_sensor_encoder_reset, "Zerar encoders"},
    {"ULTRA READ", cmd_sensor_ultra_read, "Estado + tempo de eco"},
    {"READ ULTRA", cmd_sensor_ultra_read, "Estado + tempo de eco (alias)"},
    {"ULTRA DIST", cmd_sensor_ultra_dist, "Distancia convertida em cm"},
    {"DIST ULTRA", cmd_sensor_ultra_dist, "Distancia convertida em cm (alias)"},
    {"ULTRA RAW", cmd_sensor_ultra_raw, "Validação elétrica + tempo de eco"},
    {"RAW ULTRA", cmd_sensor_ultra_raw, "Validação elétrica + tempo de eco (alias)"},
    {"ULTRA STATUS", cmd_sensor_ultra_status, "Status e contadores do driver"},
    {"STATUS ULTRA", cmd_sensor_ultra_status, "Status e contadores do driver (alias)"},
    {"ULTRA STREAM", cmd_sensor_ultra_stream, "Fluxo contínuo ultrassônico"},
    {"STREAM ULTRA", cmd_sensor_ultra_stream, "Fluxo contínuo ultrassônico (alias)"},
    {"STOP ULTRA", cmd_sensor_stop, "Para stream de ultrassônico"},
    {"STOP", cmd_sensor_stop, "Para fluxos"},
    {"EXIT", cmd_sensor_exit, "Volta ao menu principal"},
    {"HELP", cmd_sensor_help, "Ajuda do menu sensores"},
};

Command* getSensorCommands(size_t &count) {
    count = sizeof(comandosSensor)/sizeof(Command);
    return comandosSensor;
}
