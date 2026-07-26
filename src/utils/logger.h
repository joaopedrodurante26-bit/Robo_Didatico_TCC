#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

void initLogger();

void logDebug(String msg);
void logInfo(String msg);
void logWarn(String msg);
void logError(String msg);
void logFatal(String msg);

#endif