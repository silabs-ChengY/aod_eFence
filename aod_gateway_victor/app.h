/***************************************************************************//**
 * @file
 * @brief AoA locator application.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "aoa.h"
#include "conn.h"

void app_init(int argc, char *argv[]);
void app_process_action(void);
void app_deinit(void);

#define SCAN_INTERVAL                 16   //10ms
#define SCAN_WINDOW                   16   //10ms
#define SCAN_PASSIVE                  0
#define SCAN_ACTIVE                   1

#define SERVICE_UUID_LEN 16
#define CHAR_UUID_LEN 16
#define AD_FIELD_I 0x06
#define AD_FILED_C 0x07

// Common functions for all operating mode
uint8_t find_service_in_advertisement(uint8_t *advdata, uint8_t advlen, uint8_t *service_uuid);
void app_bt_on_event(sl_bt_msg_t *evt);
void app_on_iq_report(conn_properties_t *tag, aoa_iq_report_t *iq_report);

// Variables
extern uint32_t verbose_level;       // App verbose level

#ifdef __cplusplus
};
#endif

#endif // APP_H
