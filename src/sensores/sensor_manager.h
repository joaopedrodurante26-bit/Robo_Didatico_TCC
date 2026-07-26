#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

enum UltraFilterMode {
	ULTRA_FILTER_NONE = 0,
	ULTRA_FILTER_MEDIAN,
	ULTRA_FILTER_MOVING,
	ULTRA_FILTER_EXP
};

struct UltraStats {
	unsigned long reads;
	unsigned long validReads;
	unsigned long timeouts;
	unsigned long outOfRange;
	unsigned long echoShort;
	unsigned long echoLong;
	unsigned long sensorError;
	unsigned long invalidRead;
};

void initSensorManager();
void updateSensorManager();

// Distância frontal em cm convertida a partir do tempo de eco bruto.
float getUltraDistanceCm();

// Distância bruta convertida a partir do tempo de eco, antes de filtro/calibração.
float getUltraRawDistanceCm();

// Distância após filtro e calibração.
float getUltraFilteredDistanceCm();

// Indica se a distância atual é válida para uso de controle.
bool isUltraDistanceValid();

// Timestamp da última atualização do ultrassônico.
unsigned long getUltraLastUpdateMs();

// Frequência real observada do ciclo do ultrassônico.
float getUltraUpdateHz();

// Tempo desde a última leitura em milissegundos.
unsigned long getUltraLastAgeMs();

// Calibração persistente.
float getUltraCalibrationFactor();
bool setUltraCalibrationFactor(float factor);

// Filtro configurável.
UltraFilterMode getUltraFilterMode();
bool setUltraFilterMode(UltraFilterMode mode);
const char* ultraFilterModeToString(UltraFilterMode mode);

// Estatísticas do ultrassônico.
UltraStats getUltraStats();
void resetUltraStats();

// Função de utilidade para calibração com distância conhecida.
bool calibrateUltraWithKnownDistance(float knownDistanceCm);

#endif
