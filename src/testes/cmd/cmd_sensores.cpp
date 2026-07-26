#include "cmd_sensores.h"

#include "../../sensores/sensores.h"
#include "../../sensores/sensor_manager.h"

static String formatAgeMs(unsigned long ms) {
    return String(ms) + " ms";
}

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

static void cmd_sensor_ultra_cal(String args) {
    String entrada = args;
    entrada.trim();

    if (entrada.length() == 0) {
        console_println("ULTRA CAL <distancia_cm>");
        console_println("Use uma distancia conhecida para calcular o fator de calibracao.");
        console_println("Exemplo: ULTRA CAL 100");
        return;
    }

    float knownDistance = entrada.toFloat();
    if (knownDistance <= 0.0f) {
        console_println("Distancia invalida. Informe um valor maior que zero.");
        return;
    }

    if (calibrateUltraWithKnownDistance(knownDistance)) {
        console_println("[OK] Calibracao aplicada.");
        console_println("Fator..............." + String(getUltraCalibrationFactor(), 6));
        console_println("Distancia conhecida.." + String(knownDistance, 1) + " cm");
    } else {
        console_println("Falha ao calibrar. Verifique se existe uma leitura ultrassonica valida.");
    }
}

static void cmd_sensor_ultra_info(String args) {
    UltraStats stats = getUltraStats();
    unsigned long age = getUltraLastAgeMs();

    console_println("HC-SR04 - INFO");
    console_println("");
    console_println("Status.............." + String(ultraStatusToText(getUltraStatus())));
    console_println("Filtro.............." + String(ultraFilterModeToString(getUltraFilterMode())));
    console_println("Calibracao.........." + String(getUltraCalibrationFactor(), 6));
    console_println("Timestamp..........." + String(getUltraLastUpdateMs()) + " ms");
    console_println("Ultima leitura......" + formatAgeMs(age));
    console_println("Frequencia.........." + String(getUltraUpdateHz(), 1) + " Hz");
    console_println("");
    console_println("Leituras............" + String(stats.reads));
    console_println("Validas............." + String(stats.validReads));
    console_println("Timeouts............" + String(stats.timeouts));
    console_println("Out of Range........" + String(stats.outOfRange));
    console_println("Echo curto.........." + String(stats.echoShort));
    console_println("Echo longo.........." + String(stats.echoLong));
    console_println("Erro..............." + String(stats.sensorError));
    console_println("Leitura invalida...." + String(stats.invalidRead));
}

static void cmd_sensor_ultra_filter(String args) {
    String entrada = args;
    entrada.trim();
    entrada.toUpperCase();

    if (entrada.length() == 0) {
        console_println("ULTRA FILTER <NONE|MEDIAN|MOVING|EXP>");
        console_println("Filtro atual: " + String(ultraFilterModeToString(getUltraFilterMode())));
        return;
    }

    UltraFilterMode mode;
    if (entrada == "NONE") {
        mode = ULTRA_FILTER_NONE;
    } else if (entrada == "MEDIAN") {
        mode = ULTRA_FILTER_MEDIAN;
    } else if (entrada == "MOVING") {
        mode = ULTRA_FILTER_MOVING;
    } else if (entrada == "EXP") {
        mode = ULTRA_FILTER_EXP;
    } else {
        console_println("Filtro invalido. Use NONE, MEDIAN, MOVING ou EXP.");
        return;
    }

    if (setUltraFilterMode(mode)) {
        console_println("[OK] Filtro aplicado: " + String(ultraFilterModeToString(getUltraFilterMode())));
    } else {
        console_println("Falha ao salvar o filtro.");
    }
}

static void cmd_sensor_ultra_reset(String args) {
    resetUltraStats();
    console_println("[OK] Estatisticas do ultrassonico reiniciadas.");
}

static void cmd_sensor_ultra_explain(String args) {
    console_println("HC-SR04");
    console_println("");
    console_println("Principio: tempo de voo ultrassonico");
    console_println("Frequencia: 40 kHz");
    console_println("Faixa: 2-400 cm");
    console_println("Resolucao teorica: 3 mm");
    console_println("Velocidade do som usada: 343 m/s");
    console_println("Formula: Distancia = Tempo x Velocidade / 2");
}

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
    console_println("ULTRA STATUS - status e estatísticas básicas");
    console_println("ULTRA INFO - estatísticas, timestamp, frequencia, filtro e calibracao");
    console_println("ULTRA FILTER - altera o filtro: NONE, MEDIAN, MOVING, EXP");
    console_println("ULTRA CAL - calibra com distancia conhecida");
    console_println("ULTRA RESET - zera estatisticas");
    console_println("ULTRA EXPLAIN - explica o funcionamento do HC-SR04");
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
    {"ULTRA CAL", cmd_sensor_ultra_cal, "Calibrar com distancia conhecida"},
    {"ULTRA INFO", cmd_sensor_ultra_info, "Estatisticas detalhadas do ultrassonico"},
    {"ULTRA FILTER", cmd_sensor_ultra_filter, "Configurar filtro do ultrassonico"},
    {"ULTRA RESET", cmd_sensor_ultra_reset, "Zerar estatisticas do ultrassonico"},
    {"ULTRA EXPLAIN", cmd_sensor_ultra_explain, "Explicacao didatica do HC-SR04"},
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
