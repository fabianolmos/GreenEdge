/**
 * @file sensor_thread.h
 * @brief Sensor thread management interface
 */

#ifndef SENSOR_THREAD_H
#define SENSOR_THREAD_H

/**
 * @brief Start the sensor thread
 * 
 * Initializes and starts the sensor reading thread.
 * This thread will wait for trigger signals to perform readings.
 */
void sensor_thread_start(void);

/**
 * @brief Trigger a sensor reading
 * 
 * Signals the sensor thread to perform a new reading.
 * Non-blocking call.
 */
void sensor_thread_trigger(void);

/**
 * @brief Start periodic sensor readings
 * 
 * @param period_ms Period in milliseconds between readings
 */
void sensor_thread_start_periodic(uint32_t period_ms);

/**
 * @brief Stop periodic sensor readings
 */
void sensor_thread_stop_periodic(void);

#endif /* SENSOR_THREAD_H */
