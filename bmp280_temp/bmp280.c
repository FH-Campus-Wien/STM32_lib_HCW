/**
 * @file bmp280.c
 * @brief BMP280/BME280 temperature and pressure driver implementation.
 */

#include "bmp280.h"

#include <stddef.h>

#define BMP280_ADDR_76              (0x76U << 1)
#define BMP280_ADDR_77              (0x77U << 1)

#define BMP280_REG_CALIB            0x88U
#define BMP280_REG_ID               0xD0U
#define BMP280_REG_RESET            0xE0U
#define BMP280_REG_STATUS           0xF3U
#define BMP280_REG_CTRL_MEAS        0xF4U
#define BMP280_REG_CONFIG           0xF5U
#define BMP280_REG_PRESS_MSB        0xF7U
#define BMP280_REG_TEMP_MSB         0xFAU

#define BMP280_CHIP_ID              0x58U
#define BME280_CHIP_ID              0x60U
#define BMP280_RESET_COMMAND        0xB6U
#define BMP280_STATUS_IM_UPDATE     0x01U
#define BMP280_RESET_TIMEOUT_MS     100U

/* t_sb=1000 ms, filter off, SPI 3-wire disabled. */
#define BMP280_CONFIG_VALUE         0xA0U
/* osrs_t=x1, osrs_p=x1, normal mode. */
#define BMP280_CTRL_MEAS_VALUE      0x27U

#define BMP280_RAW_SKIPPED          0x80000L

/** Read one or more consecutive sensor registers. */
static bmp280_status_t bmp280_read_bytes(const bmp280_t *bmp280,
                                         uint8_t reg,
                                         uint8_t *buffer,
                                         uint16_t size)
{
  if (HAL_I2C_Mem_Read(bmp280->hi2c, bmp280->address, reg,
                       I2C_MEMADD_SIZE_8BIT, buffer, size,
                       HAL_MAX_DELAY) != HAL_OK)
  {
    return BMP280_ERROR_I2C;
  }

  return BMP280_OK;
}

/** Write a single sensor register. */
static bmp280_status_t bmp280_write_byte(const bmp280_t *bmp280,
                                         uint8_t reg,
                                         uint8_t value)
{
  if (HAL_I2C_Mem_Write(bmp280->hi2c, bmp280->address, reg,
                        I2C_MEMADD_SIZE_8BIT, &value, 1U,
                        HAL_MAX_DELAY) != HAL_OK)
  {
    return BMP280_ERROR_I2C;
  }

  return BMP280_OK;
}

/** Decode an unsigned little-endian 16-bit calibration value. */
static uint16_t bmp280_u16(const uint8_t *data)
{
  return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
}

/** Decode a signed little-endian 16-bit calibration value. */
static int16_t bmp280_s16(const uint8_t *data)
{
  return (int16_t)bmp280_u16(data);
}

/** Decode a 20-bit uncompensated measurement value. */
static int32_t bmp280_adc20(const uint8_t *data)
{
  return (int32_t)(((uint32_t)data[0] << 12) |
                   ((uint32_t)data[1] << 4) |
                   ((uint32_t)data[2] >> 4));
}

/** Return non-zero when an ID belongs to a supported Bosch sensor. */
static uint8_t bmp280_is_supported_id(uint8_t chip_id)
{
  return (uint8_t)((chip_id == BMP280_CHIP_ID) ||
                   (chip_id == BME280_CHIP_ID));
}

/**
 * Probe both legal addresses and validate the chip ID at each address.
 *
 * Address readiness alone is insufficient because another I2C peripheral may
 * occupy 0x76 while the actual sensor is available at 0x77.
 */
static bmp280_status_t bmp280_detect(bmp280_t *bmp280)
{
  static const uint16_t addresses[] = {BMP280_ADDR_76, BMP280_ADDR_77};
  uint8_t found_i2c_device = 0U;
  uint8_t read_chip_id = 0U;

  for (uint32_t i = 0U; i < (sizeof(addresses) / sizeof(addresses[0])); ++i)
  {
    uint8_t chip_id = 0U;

    if (HAL_I2C_IsDeviceReady(bmp280->hi2c, addresses[i], 2U, 20U) != HAL_OK)
    {
      continue;
    }

    found_i2c_device = 1U;
    bmp280->address = addresses[i];
    if (bmp280_read_bytes(bmp280, BMP280_REG_ID, &chip_id, 1U) != BMP280_OK)
    {
      continue;
    }

    read_chip_id = 1U;
    if (bmp280_is_supported_id(chip_id) != 0U)
    {
      bmp280->chip_id = chip_id;
      return BMP280_OK;
    }
  }

  bmp280->address = 0U;
  if (found_i2c_device == 0U)
  {
    return BMP280_ERROR_NOT_FOUND;
  }

  return (read_chip_id != 0U) ? BMP280_ERROR_CHIP_ID : BMP280_ERROR_I2C;
}

/** Wait until the reset-time NVM copy has completed. */
static bmp280_status_t bmp280_wait_for_nvm_copy(bmp280_t *bmp280)
{
  const uint32_t started = HAL_GetTick();
  uint8_t sensor_status;
  uint8_t received_status = 0U;

  do
  {
    bmp280_status_t result = bmp280_read_bytes(bmp280, BMP280_REG_STATUS,
                                                &sensor_status, 1U);
    if (result == BMP280_OK)
    {
      received_status = 1U;

      if ((sensor_status & BMP280_STATUS_IM_UPDATE) == 0U)
      {
        return BMP280_OK;
      }
    }

    /* A sensor may briefly NACK while its reset sequence is still running. */
    HAL_Delay(1U);
  } while ((HAL_GetTick() - started) < BMP280_RESET_TIMEOUT_MS);

  return (received_status != 0U) ? BMP280_ERROR_TIMEOUT : BMP280_ERROR_I2C;
}

/** Read and validate factory-programmed compensation parameters. */
static bmp280_status_t bmp280_read_calibration(bmp280_t *bmp280)
{
  uint8_t calib[24];
  bmp280_status_t status = bmp280_read_bytes(bmp280, BMP280_REG_CALIB,
                                              calib, sizeof(calib));

  if (status != BMP280_OK)
  {
    return status;
  }

  bmp280->dig_T1 = bmp280_u16(&calib[0]);
  bmp280->dig_T2 = bmp280_s16(&calib[2]);
  bmp280->dig_T3 = bmp280_s16(&calib[4]);
  bmp280->dig_P1 = bmp280_u16(&calib[6]);
  bmp280->dig_P2 = bmp280_s16(&calib[8]);
  bmp280->dig_P3 = bmp280_s16(&calib[10]);
  bmp280->dig_P4 = bmp280_s16(&calib[12]);
  bmp280->dig_P5 = bmp280_s16(&calib[14]);
  bmp280->dig_P6 = bmp280_s16(&calib[16]);
  bmp280->dig_P7 = bmp280_s16(&calib[18]);
  bmp280->dig_P8 = bmp280_s16(&calib[20]);
  bmp280->dig_P9 = bmp280_s16(&calib[22]);

  /* These unsigned coefficients are divisors/scales and must not be zero. */
  if ((bmp280->dig_T1 == 0U) || (bmp280->dig_P1 == 0U))
  {
    return BMP280_ERROR_CALIBRATION;
  }

  return BMP280_OK;
}

/** Apply the Bosch integer temperature compensation formula. */
static bmp280_status_t bmp280_compensate_temperature(bmp280_t *bmp280,
                                                      int32_t adc_temperature)
{
  int32_t var1;
  int32_t var2;
  int32_t temperature_x100;

  if (adc_temperature == BMP280_RAW_SKIPPED)
  {
    return BMP280_ERROR_DATA;
  }

  var1 = ((((adc_temperature >> 3) - ((int32_t)bmp280->dig_T1 << 1))) *
          (int32_t)bmp280->dig_T2) >> 11;
  var2 = (((((adc_temperature >> 4) - (int32_t)bmp280->dig_T1) *
            ((adc_temperature >> 4) - (int32_t)bmp280->dig_T1)) >> 12) *
          (int32_t)bmp280->dig_T3) >> 14;

  /* t_fine from this sample is required by pressure compensation. */
  bmp280->t_fine = var1 + var2;
  temperature_x100 = (bmp280->t_fine * 5 + 128) >> 8;
  bmp280->temperature = (float)temperature_x100 / 100.0f;

  return BMP280_OK;
}

/** Apply the Bosch 64-bit integer pressure compensation formula. */
static bmp280_status_t bmp280_compensate_pressure(bmp280_t *bmp280,
                                                   int32_t adc_pressure)
{
  int64_t var1;
  int64_t var2;
  int64_t pressure;

  if (adc_pressure == BMP280_RAW_SKIPPED)
  {
    return BMP280_ERROR_DATA;
  }

  var1 = (int64_t)bmp280->t_fine - 128000;
  var2 = var1 * var1 * (int64_t)bmp280->dig_P6;
  /* Multiplication avoids left-shifting potentially negative signed values. */
  var2 += var1 * (int64_t)bmp280->dig_P5 * 131072LL;
  var2 += (int64_t)bmp280->dig_P4 * 34359738368LL;
  var1 = ((var1 * var1 * (int64_t)bmp280->dig_P3) >> 8) +
         (var1 * (int64_t)bmp280->dig_P2 * 4096LL);
  var1 = ((((int64_t)1 << 47) + var1) * (int64_t)bmp280->dig_P1) >> 33;

  if (var1 == 0)
  {
    return BMP280_ERROR_CALIBRATION;
  }

  pressure = 1048576 - adc_pressure;
  pressure = (((pressure << 31) - var2) * 3125) / var1;
  var1 = ((int64_t)bmp280->dig_P9 * (pressure >> 13) *
          (pressure >> 13)) >> 25;
  var2 = ((int64_t)bmp280->dig_P8 * pressure) >> 19;
  pressure = ((pressure + var1 + var2) >> 8) +
             ((int64_t)bmp280->dig_P7 * 16LL);

  /* Bosch returns Q24.8 Pa; dividing by 25600 converts directly to hPa. */
  bmp280->pressure_hpa = (float)pressure / 25600.0f;
  return BMP280_OK;
}

/** Read one atomic pressure/temperature sample and compensate both values. */
static bmp280_status_t bmp280_read_sample(bmp280_t *bmp280)
{
  uint8_t data[6];
  bmp280_status_t status;

  /* A single 0xF7..0xFC burst activates Bosch's shadow-register guarantee. */
  status = bmp280_read_bytes(bmp280, BMP280_REG_PRESS_MSB,
                             data, sizeof(data));
  if (status != BMP280_OK)
  {
    return status;
  }

  status = bmp280_compensate_temperature(bmp280, bmp280_adc20(&data[3]));
  if (status != BMP280_OK)
  {
    return status;
  }

  return bmp280_compensate_pressure(bmp280, bmp280_adc20(&data[0]));
}

/* Public API implementation; see bmp280.h for the API documentation. */
bmp280_status_t bmp280_init(I2C_HandleTypeDef *hi2c, bmp280_t *bmp280)
{
  bmp280_status_t status;

  if ((hi2c == NULL) || (bmp280 == NULL))
  {
    return BMP280_ERROR_ARGUMENT;
  }

  bmp280->hi2c = hi2c;
  bmp280->address = 0U;
  bmp280->chip_id = 0U;
  bmp280->t_fine = 0;
  bmp280->temperature = 0.0f;
  bmp280->pressure_hpa = 0.0f;

  status = bmp280_detect(bmp280);
  if (status != BMP280_OK)
  {
    return status;
  }

  status = bmp280_write_byte(bmp280, BMP280_REG_RESET,
                             BMP280_RESET_COMMAND);
  if (status != BMP280_OK)
  {
    return status;
  }

  status = bmp280_wait_for_nvm_copy(bmp280);
  if (status != BMP280_OK)
  {
    return status;
  }

  status = bmp280_read_calibration(bmp280);
  if (status != BMP280_OK)
  {
    return status;
  }

  /* config must be written while the device is still in sleep mode. */
  status = bmp280_write_byte(bmp280, BMP280_REG_CONFIG,
                             BMP280_CONFIG_VALUE);
  if (status != BMP280_OK)
  {
    return status;
  }

  return bmp280_write_byte(bmp280, BMP280_REG_CTRL_MEAS,
                           BMP280_CTRL_MEAS_VALUE);
}

/* Public API implementation; see bmp280.h for the API documentation. */
bmp280_status_t bmp280_read_temperature(bmp280_t *bmp280)
{
  uint8_t data[3];
  bmp280_status_t status;

  if ((bmp280 == NULL) || (bmp280->hi2c == NULL) ||
      (bmp280->address == 0U))
  {
    return BMP280_ERROR_ARGUMENT;
  }

  status = bmp280_read_bytes(bmp280, BMP280_REG_TEMP_MSB,
                             data, sizeof(data));
  if (status != BMP280_OK)
  {
    return status;
  }

  return bmp280_compensate_temperature(bmp280, bmp280_adc20(data));
}

/* Public API implementation; see bmp280.h for the API documentation. */
bmp280_status_t bmp280_read_pressure(bmp280_t *bmp280)
{
  if ((bmp280 == NULL) || (bmp280->hi2c == NULL) ||
      (bmp280->address == 0U))
  {
    return BMP280_ERROR_ARGUMENT;
  }

  /* Pressure compensation always uses temperature from the same burst. */
  return bmp280_read_sample(bmp280);
}

/* Public API implementation; see bmp280.h for the API documentation. */
bmp280_status_t bmp280_read_all(bmp280_t *bmp280)
{
  if ((bmp280 == NULL) || (bmp280->hi2c == NULL) ||
      (bmp280->address == 0U))
  {
    return BMP280_ERROR_ARGUMENT;
  }

  return bmp280_read_sample(bmp280);
}
