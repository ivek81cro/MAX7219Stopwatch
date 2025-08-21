#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Arduino.h>

// MAX7219 konfiguracija
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 5
#define DATA_PIN 26
#define CLK_PIN 14
#define CS_PIN 27
#define TOTAL_COLUMNS 40

// Laser senzor konfiguracija
#define LASER_SENSOR_PIN 13

#endif // CONFIGURATION_H
