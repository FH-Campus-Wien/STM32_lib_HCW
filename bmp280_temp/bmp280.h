/**
 * @file bmp280.h
 * @brief STM32 HAL driver for BMP280 and BME280 temperature/pressure data.
 *
 * The BME280 is accepted as a compatible temperature/pressure sensor. Humidity
 * acquisition is intentionally not implemented by this driver.
 */

#ifndef BMP280_H
#define BMP280_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

/** Result codes returned by the BMP280 driver API. */
typedef enum
{
  BMP280_OK = 0,
  BMP280_ERROR_ARGUMENT = 1,
  BMP280_ERROR_NOT_FOUND = 2,
  BMP280_ERROR_CHIP_ID = 3,
  BMP280_ERROR_I2C = 4,
  BMP280_ERROR_TIMEOUT = 5,
  BMP280_ERROR_CALIBRATION = 6,
  BMP280_ERROR_DATA = 7
} bmp280_status_t;

/** Driver state and sensor-specific calibration data. */
typedef struct
{
  I2C_HandleTypeDef *hi2c;
  uint16_t address;
  uint8_t chip_id;

  uint16_t dig_T1;
  int16_t dig_T2;
  int16_t dig_T3;
  uint16_t dig_P1;
  int16_t dig_P2;
  int16_t dig_P3;
  int16_t dig_P4;
  int16_t dig_P5;
  int16_t dig_P6;
  int16_t dig_P7;
  int16_t dig_P8;
  int16_t dig_P9;

  int32_t t_fine;
  float temperature;
  float pressure_hpa;
} bmp280_t;

/**
 * @brief Detect, reset and configure a BMP280 or BME280.
 *
 * Both possible I2C addresses are checked by reading and validating the chip
 * ID. The sensor is configured for temperature and pressure oversampling x1,
 * normal mode, a 1000 ms standby period and a disabled IIR filter.
 *
 * @param hi2c STM32 HAL I2C handle used for communication.
 * @param bmp280 Driver state object to initialize.
 * @return BMP280_OK on success, otherwise a bmp280_status_t error code.
 */
bmp280_status_t bmp280_init(I2C_HandleTypeDef *hi2c, bmp280_t *bmp280);

/**
 * @brief Read and compensate the current temperature value.
 *
 * The result is stored in bmp280->temperature in degrees Celsius. This call
 * also updates bmp280->t_fine for diagnostic purposes.
 *
 * @param bmp280 Initialized driver state object.
 * @return BMP280_OK on success, otherwise a bmp280_status_t error code.
 */
bmp280_status_t bmp280_read_temperature(bmp280_t *bmp280);

/**
 * @brief Read a coherent temperature/pressure sample and return its pressure.
 *
 * Temperature and pressure registers are read in one burst so the pressure
 * compensation always uses the matching t_fine value. Both public result
 * fields are updated; pressure is stored in bmp280->pressure_hpa.
 *
 * @param bmp280 Initialized driver state object.
 * @return BMP280_OK on success, otherwise a bmp280_status_t error code.
 */
bmp280_status_t bmp280_read_pressure(bmp280_t *bmp280);

/**
 * @brief Read one coherent sample and update temperature and pressure.
 *
 * @param bmp280 Initialized driver state object.
 * @return BMP280_OK on success, otherwise a bmp280_status_t error code.
 */
bmp280_status_t bmp280_read_all(bmp280_t *bmp280);

#ifdef __cplusplus
}
#endif

#endif /* BMP280_H */
