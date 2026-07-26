#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

void initSensorManager();
void updateSensorManager();

// Distância frontal em cm convertida a partir do tempo de eco bruto.
float getUltraDistanceCm();

// Indica se a distância atual é válida para uso de controle.
bool isUltraDistanceValid();

#endif
