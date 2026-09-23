#ifndef PN532_H
#define PN532_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

#define PN532_I2C_ADDRESS        (0x24U << 1)
#define PN532_DEFAULT_TIMEOUT_MS 1000U
#define PN532_MAX_UID_LENGTH     10U

typedef enum
{
  PN532_OK = 0,
  PN532_NO_TARGET = 1,
  PN532_ERROR_ARGUMENT = -1,
  PN532_ERROR_I2C = -2,
  PN532_ERROR_TIMEOUT = -3,
  PN532_ERROR_ACK = -4,
  PN532_ERROR_FRAME = -5,
  PN532_ERROR_RESPONSE = -6
} PN532_Status;

typedef struct
{
  uint8_t ic;
  uint8_t version;
  uint8_t revision;
  uint8_t support;
} PN532_FirmwareInfo;

typedef struct
{
  uint8_t target_number;
  uint16_t sens_res;
  uint8_t sel_res;
  uint8_t uid_length;
  uint8_t uid[PN532_MAX_UID_LENGTH];
} PN532_Target;

typedef struct
{
  I2C_HandleTypeDef *i2c;
  uint32_t timeout_ms;
} PN532_Handle;

PN532_Status PN532_Init(PN532_Handle *handle, I2C_HandleTypeDef *i2c);
PN532_Status PN532_GetFirmwareVersion(PN532_Handle *handle,
                                      PN532_FirmwareInfo *info);
PN532_Status PN532_SAMConfigure(PN532_Handle *handle);
PN532_Status PN532_ConfigureReader(PN532_Handle *handle);
PN532_Status PN532_ReadPassiveTarget(PN532_Handle *handle,
                                     PN532_Target *target);

#ifdef __cplusplus
}
#endif

#endif /* PN532_H */
