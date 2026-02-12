/**
 * @file sensors.h
 * @brief Sensor data management interface
 * 
 * This module provides thread-safe access to sensor data
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

/**
 * @brief Sensor data structure
 */
struct sensor_data {
    int temperature;  /**< Temperature in Celsius */
    int humidity;     /**< Humidity in percentage */
};

/**
 * @brief Initialize sensor subsystem
 * 
 * Must be called before any other sensor function
 */
void sensors_init(void);

/**
 * @brief Read current sensor values (simulated)
 * 
 * This function updates the internal sensor state.
 * Thread-safe when called from sensor thread.
 * 
 * @param data Pointer to sensor data structure to update
 */
void sensors_read(void);

/**
 * @brief Get current sensor values (thread-safe)
 * 
 * This function provides thread-safe access to sensor data
 * from any context (shell, other threads, etc.)
 * 
 * @param data Pointer to sensor data structure to fill
 */
void sensors_get(struct sensor_data *data);

/**
 * @brief Trigger a sensor reading
 * 
 * Wakes up the sensor thread to perform a new reading
 */
void sensors_trigger_read(void);

#endif /* SENSORS_H */
