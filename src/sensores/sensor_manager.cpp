#include "sensor_manager.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <math.h>

#include "sensores.h"
#include "../controle/controle.h"
#include "../robot/robot_state.h"

static float g_ultraDistanceCm = -1.0f;
static bool g_ultraDistanceValid = false;
static float g_ultraRawDistanceCm = -1.0f;
static float g_ultraCalibrationFactor = 1.0f;
static UltraFilterMode g_ultraFilterMode = ULTRA_FILTER_MEDIAN;
static float g_ultraFilteredBuffer[5] = {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f};
static size_t g_ultraBufferCount = 0;
static size_t g_ultraBufferIndex = 0;
static float g_ultraExpValue = -1.0f;
static bool g_ultraExpInitialized = false;
static unsigned long g_ultraLastUpdateMs = 0;
static unsigned long g_ultraPreviousUpdateMs = 0;
static float g_ultraUpdateHz = 0.0f;
static UltraStats g_ultraStats = {0, 0, 0, 0, 0, 0, 0, 0};
static bool g_ultraConfigLoaded = false;

static const char* ULTRA_CONFIG_PATH = "/ultra_config.json";
static const float ULTRA_EXP_ALPHA = 0.35f;
static const unsigned long ULTRA_PERIOD_MS = 60UL;

const char* ultraFilterModeToString(UltraFilterMode mode);

static bool littleFsFileExistsSilent(const char* path) {
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) {
        return false;
    }

    File entry = root.openNextFile();
    while (entry) {
        if (String(entry.name()) == path) {
            entry.close();
            root.close();
            return true;
        }

        entry.close();
        entry = root.openNextFile();
    }

    root.close();
    return false;
}

static float echoUsToCm(unsigned long echoUs) {
    // Distância (cm) = tempo(us) * velocidadeSom(cm/us) / 2.
    return static_cast<float>(echoUs) * 0.0343f / 2.0f;
}

static void pushUltraBuffer(float value) {
    g_ultraFilteredBuffer[g_ultraBufferIndex] = value;
    g_ultraBufferIndex = (g_ultraBufferIndex + 1) % 5;
    if (g_ultraBufferCount < 5) {
        g_ultraBufferCount++;
    }
}

static float medianOfBuffer() {
    float values[5];
    size_t count = g_ultraBufferCount;
    if (count == 0) {
        return -1.0f;
    }

    for (size_t i = 0; i < count; ++i) {
        values[i] = g_ultraFilteredBuffer[i];
    }

    for (size_t i = 0; i + 1 < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            if (values[j] < values[i]) {
                float tmp = values[i];
                values[i] = values[j];
                values[j] = tmp;
            }
        }
    }

    return values[count / 2];
}

static float movingAverageOfBuffer() {
    if (g_ultraBufferCount == 0) {
        return -1.0f;
    }

    float soma = 0.0f;
    for (size_t i = 0; i < g_ultraBufferCount; ++i) {
        soma += g_ultraFilteredBuffer[i];
    }

    return soma / static_cast<float>(g_ultraBufferCount);
}

static float applyUltraFilter(float value) {
    if (value < 0.0f) {
        return value;
    }

    switch (g_ultraFilterMode) {
        case ULTRA_FILTER_NONE:
            return value;
        case ULTRA_FILTER_MEDIAN:
            pushUltraBuffer(value);
            return medianOfBuffer();
        case ULTRA_FILTER_MOVING:
            pushUltraBuffer(value);
            return movingAverageOfBuffer();
        case ULTRA_FILTER_EXP:
            if (!g_ultraExpInitialized) {
                g_ultraExpValue = value;
                g_ultraExpInitialized = true;
            } else {
                g_ultraExpValue = (ULTRA_EXP_ALPHA * value) + ((1.0f - ULTRA_EXP_ALPHA) * g_ultraExpValue);
            }
            return g_ultraExpValue;
        default:
            return value;
    }
}

static void resetFilterState() {
    for (size_t i = 0; i < 5; ++i) {
        g_ultraFilteredBuffer[i] = -1.0f;
    }
    g_ultraBufferCount = 0;
    g_ultraBufferIndex = 0;
    g_ultraExpValue = -1.0f;
    g_ultraExpInitialized = false;
}

static void updateUltraSampleIfDue(bool force) {
    unsigned long now = millis();
    if (!force && g_ultraLastUpdateMs > 0 && (now - g_ultraLastUpdateMs) < ULTRA_PERIOD_MS) {
        return;
    }

    atualizarUltrassonico();

    g_ultraPreviousUpdateMs = g_ultraLastUpdateMs;
    g_ultraLastUpdateMs = now;
    if (g_ultraPreviousUpdateMs > 0) {
        unsigned long delta = g_ultraLastUpdateMs - g_ultraPreviousUpdateMs;
        if (delta > 0) {
            g_ultraUpdateHz = 1000.0f / static_cast<float>(delta);
        }
    }

    UltrasonicStatus ultraStatus = getUltraStatus();
    unsigned long ultraEchoUs = getUltraEchoTimeUs();
    g_ultraRawDistanceCm = (ultraEchoUs > 0) ? echoUsToCm(ultraEchoUs) : -1.0f;

    g_ultraStats.reads++;
    if (ultraStatus == ULTRA_OK) {
        g_ultraStats.validReads++;
    } else if (ultraStatus == ULTRA_TIMEOUT) {
        g_ultraStats.timeouts++;
    } else if (ultraStatus == ULTRA_OUT_OF_RANGE) {
        g_ultraStats.outOfRange++;
    } else if (ultraStatus == ULTRA_ECHO_TOO_SHORT) {
        g_ultraStats.echoShort++;
    } else if (ultraStatus == ULTRA_ECHO_TOO_LONG) {
        g_ultraStats.echoLong++;
    } else if (ultraStatus == ULTRA_SENSOR_ERROR) {
        g_ultraStats.sensorError++;
    } else if (ultraStatus == ULTRA_INVALID_READING) {
        g_ultraStats.invalidRead++;
    }

    if (ultraStatus == ULTRA_OK && g_ultraRawDistanceCm > 0.0f) {
        float filtered = applyUltraFilter(g_ultraRawDistanceCm);
        if (filtered > 0.0f) {
            g_ultraDistanceCm = filtered * g_ultraCalibrationFactor;
            g_ultraDistanceValid = true;
            return;
        }
    }

    g_ultraDistanceCm = -1.0f;
    g_ultraDistanceValid = false;
}

static bool saveUltraConfig() {
    StaticJsonDocument<256> doc;
    doc["calibration"] = g_ultraCalibrationFactor;
    doc["filter"] = ultraFilterModeToString(g_ultraFilterMode);

    File file = LittleFS.open(ULTRA_CONFIG_PATH, "w");
    if (!file) {
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        file.close();
        return false;
    }

    file.close();
    return true;
}

static void loadUltraConfig() {
    g_ultraConfigLoaded = true;
    if (!littleFsFileExistsSilent(ULTRA_CONFIG_PATH)) {
        return;
    }

    File file = LittleFS.open(ULTRA_CONFIG_PATH, "r");
    if (!file) {
        return;
    }

    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (err) {
        return;
    }

    if (doc.containsKey("calibration")) {
        g_ultraCalibrationFactor = doc["calibration"].as<float>();
    }

    if (doc.containsKey("filter")) {
        String filter = doc["filter"].as<String>();
        filter.toUpperCase();
        if (filter == "NONE") {
            g_ultraFilterMode = ULTRA_FILTER_NONE;
        } else if (filter == "MEDIAN") {
            g_ultraFilterMode = ULTRA_FILTER_MEDIAN;
        } else if (filter == "MOVING") {
            g_ultraFilterMode = ULTRA_FILTER_MOVING;
        } else if (filter == "EXP") {
            g_ultraFilterMode = ULTRA_FILTER_EXP;
        }
    }
}

void initSensorManager() {
    loadUltraConfig();
    // Mantém o estado consistente no boot com uma leitura inicial do ultrassônico.
    updateUltraSampleIfDue(true);
    updateSensorManager();
}

void updateSensorManager() {
    atualizarIMU();
    updateUltraSampleIfDue(false);

    robotStateSetSensors(
        getPulsosEsq(),
        getPulsosDir(),
        g_ultraDistanceCm,
        getAccelX(),
        getAccelY(),
        getAccelZ(),
        getGyroX(),
        getGyroY(),
        getGyroZ());

    robotStateSetMotorCommand(getVelEsq(), getVelDir());
    robotStateSetUptime(millis());
}

float getUltraDistanceCm() {
    return g_ultraDistanceCm;
}

bool isUltraDistanceValid() {
    return g_ultraDistanceValid;
}

float getUltraRawDistanceCm() {
    return g_ultraRawDistanceCm;
}

float getUltraFilteredDistanceCm() {
    return g_ultraDistanceCm;
}

unsigned long getUltraLastUpdateMs() {
    return g_ultraLastUpdateMs;
}

float getUltraUpdateHz() {
    return g_ultraUpdateHz;
}

unsigned long getUltraLastAgeMs() {
    if (g_ultraLastUpdateMs == 0) {
        return 0;
    }
    return millis() - g_ultraLastUpdateMs;
}

float getUltraCalibrationFactor() {
    return g_ultraCalibrationFactor;
}

bool setUltraCalibrationFactor(float factor) {
    if (!isfinite(factor) || factor <= 0.0f || factor > 10.0f) {
        return false;
    }

    g_ultraCalibrationFactor = factor;
    return saveUltraConfig();
}

UltraFilterMode getUltraFilterMode() {
    return g_ultraFilterMode;
}

bool setUltraFilterMode(UltraFilterMode mode) {
    if (mode < ULTRA_FILTER_NONE || mode > ULTRA_FILTER_EXP) {
        return false;
    }

    g_ultraFilterMode = mode;
    resetFilterState();
    return saveUltraConfig();
}

const char* ultraFilterModeToString(UltraFilterMode mode) {
    switch (mode) {
        case ULTRA_FILTER_NONE: return "NONE";
        case ULTRA_FILTER_MEDIAN: return "MEDIAN";
        case ULTRA_FILTER_MOVING: return "MOVING";
        case ULTRA_FILTER_EXP: return "EXP";
        default: return "UNKNOWN";
    }
}

UltraStats getUltraStats() {
    return g_ultraStats;
}

void resetUltraStats() {
    g_ultraStats = {0, 0, 0, 0, 0, 0, 0, 0};
}

bool calibrateUltraWithKnownDistance(float knownDistanceCm) {
    if (!isfinite(knownDistanceCm) || knownDistanceCm <= 0.0f) {
        return false;
    }

    if (getUltraStatus() != ULTRA_OK) {
        return false;
    }

    float raw = getUltraRawDistanceCm();
    if (raw <= 0.0f) {
        return false;
    }

    float factor = knownDistanceCm / raw;
    return setUltraCalibrationFactor(factor);
}
