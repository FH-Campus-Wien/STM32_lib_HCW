/**
 * @file pn532.h
 * @brief STM32 HAL driver for a PN532 connected through I2C.
 *
 * The public API initializes the PN532, reads its firmware information,
 * configures reader mode and detects one passive ISO/IEC 14443 Type A target.
 */

#ifndef PN532_H
#define PN532_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/** PN532 I2C address in the left-shifted format expected by STM32 HAL. */
#define PN532_I2C_ADDRESS        (0x24U << 1)
/** Default maximum wait time for ACK and response frames. */
#define PN532_DEFAULT_TIMEOUT_MS 1000U
/** Maximum ISO/IEC 14443 Type A UID length supported by the driver. */
#define PN532_MAX_UID_LENGTH     10U

/** Status and error codes returned by the PN532 driver API. */
typedef enum
{
  PN532_OK = 0,              /**< Operation completed successfully. */
  PN532_NO_TARGET = 1,       /**< No passive target was detected. */
  PN532_ERROR_ARGUMENT = -1, /**< A required pointer or argument is invalid. */
  PN532_ERROR_I2C = -2,      /**< An STM32 HAL I2C operation failed. */
  PN532_ERROR_TIMEOUT = -3,  /**< The PN532 did not become ready in time. */
  PN532_ERROR_ACK = -4,      /**< The received PN532 ACK frame is invalid. */
  PN532_ERROR_FRAME = -5,    /**< A response frame or checksum is invalid. */
  PN532_ERROR_RESPONSE = -6  /**< A response contains implausible data. */
} PN532_Status;

/** Firmware information returned by PN532_GetFirmwareVersion(). */
typedef struct
{
  uint8_t ic;       /**< IC identification byte; expected PN532 value is 0x32. */
  uint8_t version;  /**< Firmware version byte. */
  uint8_t revision; /**< Firmware revision byte. */
  uint8_t support;  /**< Bit field describing supported card types. */
} PN532_FirmwareInfo;

/** Data identifying one detected ISO/IEC 14443 Type A target. */
typedef struct
{
  uint8_t target_number;                /**< Logical target number from PN532. */
  uint16_t sens_res;                    /**< SENS_RES, also known as ATQA. */
  uint8_t sel_res;                      /**< SEL_RES, also known as SAK. */
  uint8_t uid_length;                   /**< Number of valid bytes in uid. */
  uint8_t uid[PN532_MAX_UID_LENGTH];    /**< NFCID1/UID bytes. */
} PN532_Target;

/** Runtime state of one PN532 driver instance. */
typedef struct
{
  I2C_HandleTypeDef *i2c; /**< STM32 HAL I2C peripheral used by the module. */
  uint32_t timeout_ms;    /**< ACK and response timeout in milliseconds. */
} PN532_Handle;

/**
 * @brief Initialize a PN532 driver instance for I2C communication.
 *
 * The function stores the HAL handle, waits for module startup, aborts a
 * command that may have survived an MCU reset and verifies the I2C device.
 * Reader operation is configured separately with PN532_ConfigureReader().
 *
 * @param handle Driver instance to initialize.
 * @param i2c Initialized STM32 HAL I2C handle.
 * @return PN532_OK on success, otherwise a PN532_Status error code.
 */
PN532_Status PN532_Init(PN532_Handle *handle, I2C_HandleTypeDef *i2c);

/**
 * @brief Read identification and firmware information from the PN532.
 *
 * @param handle Initialized driver instance.
 * @param info Destination for IC, version, revision and support bytes.
 * @return PN532_OK on success, otherwise a PN532_Status error code.
 */
PN532_Status PN532_GetFirmwareVersion(PN532_Handle *handle,
                                      PN532_FirmwareInfo *info);

/**
 * @brief Configure the PN532 SAM interface for normal reader operation.
 *
 * Normal mode is selected without a virtual-card timeout and without using
 * the P70_IRQ output. The driver therefore waits by polling the I2C status.
 *
 * @param handle Initialized driver instance.
 * @return PN532_OK on success, otherwise a PN532_Status error code.
 */
PN532_Status PN532_SAMConfigure(PN532_Handle *handle);

/**
 * @brief Configure the PN532 for passive ISO/IEC 14443 Type A detection.
 *
 * This calls PN532_SAMConfigure(), disables automatic RATS and configures
 * finite passive-activation retries so a search can return PN532_NO_TARGET.
 *
 * @param handle Initialized driver instance.
 * @return PN532_OK on success, otherwise a PN532_Status error code.
 */
PN532_Status PN532_ConfigureReader(PN532_Handle *handle);

/**
 * @brief Search for one passive ISO/IEC 14443 Type A target.
 *
 * On success, target receives the logical target number, ATQA/SENS_RES,
 * SAK/SEL_RES and a UID of up to PN532_MAX_UID_LENGTH bytes.
 *
 * @param handle Initialized and configured driver instance.
 * @param target Destination object for detected target information.
 * @return PN532_OK when a target was found, PN532_NO_TARGET when none was
 *         detected, or another PN532_Status error code.
 */
PN532_Status PN532_ReadPassiveTarget(PN532_Handle *handle,
                                     PN532_Target *target);

#ifdef __cplusplus
}
#endif

#endif /* PN532_H */
