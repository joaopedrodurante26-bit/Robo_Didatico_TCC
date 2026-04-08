#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

void initLogger();

void logInfo(String msg);
void logWarn(String msg);
void logError(String msg);

#endif