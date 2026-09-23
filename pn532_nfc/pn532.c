/**
 * @file pn532.c
 * @brief PN532 I2C transport and passive Type A reader implementation.
 */

#include "pn532.h"

#include <string.h>

#define PN532_PREAMBLE           0x00U
#define PN532_STARTCODE1         0x00U
#define PN532_STARTCODE2         0xFFU
#define PN532_POSTAMBLE          0x00U
#define PN532_HOST_TO_PN532      0xD4U
#define PN532_PN532_TO_HOST      0xD5U
#define PN532_COMMAND_GETFW      0x02U
#define PN532_COMMAND_SETPARAM   0x12U
#define PN532_COMMAND_SAMCONFIG  0x14U
#define PN532_COMMAND_RFCONFIG   0x32U
#define PN532_COMMAND_INLIST     0x4AU
#define PN532_I2C_READY          0x01U
#define PN532_MAX_COMMAND_BYTES  32U
#define PN532_MAX_TARGET_DATA    16U

static const uint8_t pn532_ack_frame[] = {
  0x00U, 0x00U, 0xFFU, 0x00U, 0xFFU, 0x00U
};

/**
 * @brief Abort a command that may still be active after an MCU reset.
 * @param handle Initialized driver instance.
 * @return PN532_OK when the abort ACK was accepted, otherwise an error code.
 */
static PN532_Status PN532_AbortCurrentCommand(PN532_Handle *handle)
{
  uint32_t started = HAL_GetTick();

  /* An ACK frame sent by the host aborts any command that survived an MCU
   * reset. Retry because the PN532 may briefly NACK its address while busy. */
  do
  {
    if (HAL_I2C_Master_Transmit(handle->i2c, PN532_I2C_ADDRESS,
                                (uint8_t *)pn532_ack_frame,
                                sizeof(pn532_ack_frame), 20U) == HAL_OK)
    {
      HAL_Delay(5U);
      return PN532_OK;
    }

    HAL_Delay(2U);
  } while ((HAL_GetTick() - started) < handle->timeout_ms);

  return PN532_ERROR_I2C;
}

/**
 * @brief Poll the I2C status byte until the PN532 has a frame available.
 * @param handle Initialized driver instance.
 * @return PN532_OK when ready or PN532_ERROR_TIMEOUT on expiration.
 */
static PN532_Status PN532_WaitReady(PN532_Handle *handle)
{
  uint8_t status = 0U;
  uint32_t started = HAL_GetTick();

  do
  {
    if (HAL_I2C_Master_Receive(handle->i2c, PN532_I2C_ADDRESS,
                               &status, 1U, 20U) == HAL_OK)
    {
      if ((status & PN532_I2C_READY) != 0U)
      {
        return PN532_OK;
      }
    }

    HAL_Delay(2U);
  } while ((HAL_GetTick() - started) < handle->timeout_ms);

  return PN532_ERROR_TIMEOUT;
}

/**
 * @brief Wrap a command payload in a normal PN532 information frame and send it.
 * @param handle Initialized driver instance.
 * @param payload Command byte followed by its parameters.
 * @param payload_length Number of bytes in payload.
 * @return PN532_OK on success, otherwise an argument or I2C error.
 */
static PN532_Status PN532_WriteCommand(PN532_Handle *handle,
                                       const uint8_t *payload,
                                       uint8_t payload_length)
{
  uint8_t frame[PN532_MAX_COMMAND_BYTES + 8U];
  uint8_t length;
  uint8_t checksum;
  uint8_t index = 0U;

  if ((payload == NULL) || (payload_length == 0U) ||
      (payload_length > PN532_MAX_COMMAND_BYTES))
  {
    return PN532_ERROR_ARGUMENT;
  }

  length = (uint8_t)(payload_length + 1U); /* TFI + command payload */
  frame[index++] = PN532_PREAMBLE;
  frame[index++] = PN532_STARTCODE1;
  frame[index++] = PN532_STARTCODE2;
  frame[index++] = length;
  frame[index++] = (uint8_t)(0U - length);
  frame[index++] = PN532_HOST_TO_PN532;

  checksum = PN532_HOST_TO_PN532;
  memcpy(&frame[index], payload, payload_length);
  for (uint8_t i = 0U; i < payload_length; ++i)
  {
    checksum = (uint8_t)(checksum + payload[i]);
  }
  index = (uint8_t)(index + payload_length);
  frame[index++] = (uint8_t)(0U - checksum);
  frame[index++] = PN532_POSTAMBLE;

  if (HAL_I2C_Master_Transmit(handle->i2c, PN532_I2C_ADDRESS,
                              frame, index, handle->timeout_ms) != HAL_OK)
  {
    return PN532_ERROR_I2C;
  }

  return PN532_OK;
}

/**
 * @brief Wait for and validate the six-byte PN532 ACK frame.
 * @param handle Initialized driver instance.
 * @return PN532_OK for a valid ACK or a transport/frame error.
 */
static PN532_Status PN532_ReadAck(PN532_Handle *handle)
{
  uint8_t frame[sizeof(pn532_ack_frame) + 1U];
  PN532_Status status = PN532_WaitReady(handle);

  if (status != PN532_OK)
  {
    return status;
  }

  if (HAL_I2C_Master_Receive(handle->i2c, PN532_I2C_ADDRESS,
                             frame, sizeof(frame),
                             handle->timeout_ms) != HAL_OK)
  {
    return PN532_ERROR_I2C;
  }

  if (((frame[0] & PN532_I2C_READY) == 0U) ||
      (memcmp(&frame[1], pn532_ack_frame, sizeof(pn532_ack_frame)) != 0))
  {
    return PN532_ERROR_ACK;
  }

  return PN532_OK;
}

/**
 * @brief Read and validate a response with an expected payload length.
 * @param handle Initialized driver instance.
 * @param command Original command code used to verify the response code.
 * @param response Destination for response data, excluding TFI and code.
 * @param response_length Exact number of response data bytes expected.
 * @return PN532_OK on success, otherwise an argument, transport or frame error.
 */
static PN532_Status PN532_ReadResponse(PN532_Handle *handle,
                                       uint8_t command,
                                       uint8_t *response,
                                       uint8_t response_length)
{
  /* RDY + preamble/start + LEN/LCS + TFI/CMD/data + DCS/postamble. */
  uint8_t frame[PN532_MAX_COMMAND_BYTES + 10U];
  uint8_t frame_length = (uint8_t)(response_length + 10U);
  uint8_t packet_length;
  uint8_t checksum = 0U;
  PN532_Status status;

  if (((response == NULL) && (response_length != 0U)) ||
      (response_length > PN532_MAX_COMMAND_BYTES))
  {
    return PN532_ERROR_ARGUMENT;
  }

  status = PN532_WaitReady(handle);
  if (status != PN532_OK)
  {
    return status;
  }

  if (HAL_I2C_Master_Receive(handle->i2c, PN532_I2C_ADDRESS,
                             frame, frame_length,
                             handle->timeout_ms) != HAL_OK)
  {
    return PN532_ERROR_I2C;
  }

  packet_length = (uint8_t)(response_length + 2U); /* TFI + response code */
  if (((frame[0] & PN532_I2C_READY) == 0U) ||
      (frame[1] != PN532_PREAMBLE) ||
      (frame[2] != PN532_STARTCODE1) ||
      (frame[3] != PN532_STARTCODE2) ||
      (frame[4] != packet_length) ||
      ((uint8_t)(frame[4] + frame[5]) != 0U) ||
      (frame[6] != PN532_PN532_TO_HOST) ||
      (frame[7] != (uint8_t)(command + 1U)) ||
      (frame[frame_length - 1U] != PN532_POSTAMBLE))
  {
    return PN532_ERROR_FRAME;
  }

  for (uint8_t i = 6U; i < (uint8_t)(frame_length - 1U); ++i)
  {
    checksum = (uint8_t)(checksum + frame[i]);
  }
  if (checksum != 0U)
  {
    return PN532_ERROR_FRAME;
  }

  if (response_length != 0U)
  {
    memcpy(response, &frame[8], response_length);
  }
  return PN532_OK;
}

/**
 * @brief Read and validate a variable-length passive-target response.
 * @param handle Initialized driver instance.
 * @param command Original command code used to verify the response code.
 * @param response Destination buffer for response data.
 * @param response_capacity Maximum bytes accepted by response.
 * @param response_length Receives the actual response data length.
 * @return PN532_OK on success, otherwise an argument, transport or frame error.
 */
static PN532_Status PN532_ReadVariableResponse(PN532_Handle *handle,
                                               uint8_t command,
                                               uint8_t *response,
                                               uint8_t response_capacity,
                                               uint8_t *response_length)
{
  uint8_t frame[PN532_MAX_TARGET_DATA + 10U];
  uint8_t packet_length;
  uint8_t data_length;
  uint8_t postamble_index;
  uint8_t checksum = 0U;
  PN532_Status status;

  if ((response == NULL) || (response_length == NULL) ||
      (response_capacity > PN532_MAX_TARGET_DATA))
  {
    return PN532_ERROR_ARGUMENT;
  }

  status = PN532_WaitReady(handle);
  if (status != PN532_OK)
  {
    return status;
  }

  /* SetParameters disables automatic RATS, so the largest Type-A response
   * contains one target and a 10-byte UID. Extra bytes read as padding. */
  if (HAL_I2C_Master_Receive(handle->i2c, PN532_I2C_ADDRESS,
                             frame, sizeof(frame),
                             handle->timeout_ms) != HAL_OK)
  {
    return PN532_ERROR_I2C;
  }

  packet_length = frame[4];
  if (packet_length < 2U)
  {
    return PN532_ERROR_FRAME;
  }
  data_length = (uint8_t)(packet_length - 2U);
  postamble_index = (uint8_t)(packet_length + 7U);

  if (((frame[0] & PN532_I2C_READY) == 0U) ||
      (frame[1] != PN532_PREAMBLE) ||
      (frame[2] != PN532_STARTCODE1) ||
      (frame[3] != PN532_STARTCODE2) ||
      ((uint8_t)(frame[4] + frame[5]) != 0U) ||
      (frame[6] != PN532_PN532_TO_HOST) ||
      (frame[7] != (uint8_t)(command + 1U)) ||
      (data_length > response_capacity) ||
      (postamble_index >= sizeof(frame)) ||
      (frame[postamble_index] != PN532_POSTAMBLE))
  {
    return PN532_ERROR_FRAME;
  }

  for (uint8_t i = 6U; i < postamble_index; ++i)
  {
    checksum = (uint8_t)(checksum + frame[i]);
  }
  if (checksum != 0U)
  {
    return PN532_ERROR_FRAME;
  }

  memcpy(response, &frame[8], data_length);
  *response_length = data_length;
  return PN532_OK;
}

/**
 * @brief Execute one command with a fixed-length response.
 * @param handle Initialized driver instance.
 * @param command Command byte followed by its parameters.
 * @param command_length Number of bytes in command.
 * @param response Destination for response data, or NULL for no data.
 * @param response_length Exact response data length expected.
 * @return PN532_OK on success or the first protocol/transport error.
 */
static PN532_Status PN532_ExecuteCommand(PN532_Handle *handle,
                                         const uint8_t *command,
                                         uint8_t command_length,
                                         uint8_t *response,
                                         uint8_t response_length)
{
  PN532_Status status = PN532_WriteCommand(handle, command, command_length);

  if (status == PN532_OK)
  {
    status = PN532_ReadAck(handle);
  }
  if (status == PN532_OK)
  {
    status = PN532_ReadResponse(handle, command[0], response, response_length);
  }

  return status;
}

/**
 * @brief Execute one command with a variable-length response.
 * @param handle Initialized driver instance.
 * @param command Command byte followed by its parameters.
 * @param command_length Number of bytes in command.
 * @param response Destination buffer for response data.
 * @param response_capacity Maximum bytes accepted by response.
 * @param response_length Receives the actual response data length.
 * @return PN532_OK on success or the first protocol/transport error.
 */
static PN532_Status PN532_ExecuteVariableCommand(PN532_Handle *handle,
                                                 const uint8_t *command,
                                                 uint8_t command_length,
                                                 uint8_t *response,
                                                 uint8_t response_capacity,
                                                 uint8_t *response_length)
{
  PN532_Status status = PN532_WriteCommand(handle, command, command_length);

  if (status == PN532_OK)
  {
    status = PN532_ReadAck(handle);
  }
  if (status == PN532_OK)
  {
    status = PN532_ReadVariableResponse(handle, command[0], response,
                                        response_capacity, response_length);
  }

  return status;
}

PN532_Status PN532_Init(PN532_Handle *handle, I2C_HandleTypeDef *i2c)
{
  PN532_Status status;

  if ((handle == NULL) || (i2c == NULL))
  {
    return PN532_ERROR_ARGUMENT;
  }

  handle->i2c = i2c;
  handle->timeout_ms = PN532_DEFAULT_TIMEOUT_MS;

  /* PN532 modules need some time after power-up/mode selection. */
  HAL_Delay(100U);
  status = PN532_AbortCurrentCommand(handle);
  if (status != PN532_OK)
  {
    return status;
  }

  if (HAL_I2C_IsDeviceReady(i2c, PN532_I2C_ADDRESS, 3U, 100U) != HAL_OK)
  {
    return PN532_ERROR_I2C;
  }

  return PN532_OK;
}

PN532_Status PN532_GetFirmwareVersion(PN532_Handle *handle,
                                      PN532_FirmwareInfo *info)
{
  const uint8_t command[] = {PN532_COMMAND_GETFW};
  uint8_t response[4];
  PN532_Status status;

  if ((handle == NULL) || (handle->i2c == NULL) || (info == NULL))
  {
    return PN532_ERROR_ARGUMENT;
  }

  status = PN532_ExecuteCommand(handle, command, sizeof(command),
                                response, sizeof(response));
  if (status != PN532_OK)
  {
    return status;
  }

  info->ic = response[0];
  info->version = response[1];
  info->revision = response[2];
  info->support = response[3];

  if (info->ic != 0x32U)
  {
    return PN532_ERROR_RESPONSE;
  }

  return PN532_OK;
}

PN532_Status PN532_SAMConfigure(PN532_Handle *handle)
{
  /* Normal mode, no Virtual Card timeout, P70_IRQ not used. */
  const uint8_t command[] = {
    PN532_COMMAND_SAMCONFIG, 0x01U, 0x00U, 0x00U
  };

  if ((handle == NULL) || (handle->i2c == NULL))
  {
    return PN532_ERROR_ARGUMENT;
  }

  return PN532_ExecuteCommand(handle, command, sizeof(command), NULL, 0U);
}

PN532_Status PN532_ConfigureReader(PN532_Handle *handle)
{
  /* Keep automatic ATR and PICC support, but disable automatic RATS so the
   * InListPassiveTarget response has no variable-length ATS field. */
  const uint8_t set_parameters[] = {
    PN532_COMMAND_SETPARAM, 0x24U
  };
  /* RFConfiguration / MaxRetries: ATR, PSL and passive activation retries. */
  const uint8_t set_retries[] = {
    PN532_COMMAND_RFCONFIG, 0x05U, 0xFFU, 0x01U, 0x02U
  };
  PN532_Status status;

  if ((handle == NULL) || (handle->i2c == NULL))
  {
    return PN532_ERROR_ARGUMENT;
  }

  status = PN532_SAMConfigure(handle);
  if (status == PN532_OK)
  {
    status = PN532_ExecuteCommand(handle, set_parameters,
                                  sizeof(set_parameters), NULL, 0U);
  }
  if (status == PN532_OK)
  {
    status = PN532_ExecuteCommand(handle, set_retries,
                                  sizeof(set_retries), NULL, 0U);
  }

  return status;
}

PN532_Status PN532_ReadPassiveTarget(PN532_Handle *handle,
                                     PN532_Target *target)
{
  /* One passive target, 106 kbps ISO/IEC 14443 Type A. */
  const uint8_t command[] = {
    PN532_COMMAND_INLIST, 0x01U, 0x00U
  };
  uint8_t response[PN532_MAX_TARGET_DATA];
  uint8_t response_length = 0U;
  uint8_t uid_length;
  PN532_Status status;

  if ((handle == NULL) || (handle->i2c == NULL) || (target == NULL))
  {
    return PN532_ERROR_ARGUMENT;
  }

  memset(target, 0, sizeof(*target));
  status = PN532_ExecuteVariableCommand(handle, command, sizeof(command),
                                        response, sizeof(response),
                                        &response_length);
  if (status != PN532_OK)
  {
    return status;
  }

  if ((response_length == 0U) || (response[0] == 0U))
  {
    return PN532_NO_TARGET;
  }
  if (response_length < 6U)
  {
    return PN532_ERROR_RESPONSE;
  }

  uid_length = response[5];
  if ((uid_length == 0U) || (uid_length > PN532_MAX_UID_LENGTH) ||
      (response_length < (uint8_t)(6U + uid_length)))
  {
    return PN532_ERROR_RESPONSE;
  }

  target->target_number = response[1];
  target->sens_res = (uint16_t)(((uint16_t)response[2] << 8) | response[3]);
  target->sel_res = response[4];
  target->uid_length = uid_length;
  memcpy(target->uid, &response[6], uid_length);

  return PN532_OK;
}
