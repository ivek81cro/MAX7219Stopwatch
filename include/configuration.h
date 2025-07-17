#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Arduino.h>

// MAX7219 konfiguracija
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 5
#define DATA_PIN 23
#define CLK_PIN 18
#define CS_PIN 5
#define TOTAL_COLUMNS 40

// Laser senzor konfiguracija
#define LASER_SENSOR_PIN 4

#endif // CONFIGURATION_H
