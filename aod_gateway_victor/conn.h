/***********************************************************************************************//**
 * @file
 * @brief  Connection handler header file
 ***************************************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 ***************************************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this software is governed
 * by the terms of Silicon Labs Master Software License Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This software is distributed to
 * you in Source Code format and is governed by the sections of the MSLA applicable to Source Code.
 **************************************************************************************************/

#ifndef CONN_H
#define CONN_H

#include <stdint.h>
#include "sl_bt_api.h"
#include "aoa.h"
#include "app_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************//**
 * \defgroup app Application Code
 * \brief Sample Application Implementation
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup Application
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app
 * @{
 **************************************************************************************************/

/***************************************************************************************************
 * Type Definitions
 **************************************************************************************************/

// Connection state, used only in connection oriented mode
typedef enum {
  DISCOVER_SERVICES,
  DISCOVER_CHARACTERISTICS,
  ENABLE_CTE,
  RUNNING
} connection_state_t;

typedef struct {
  uint16_t connection_handle;   //This is used for connection handle for connection oriented, and for sync handle for connection less mode
  bd_addr address;
  uint8_t address_type;
  uint32_t cte_service_handle;
  uint16_t cte_enable_char_handle;
  connection_state_t connection_state;
  uint8_t num_locators_found;
  aoa_libitems_t aoa_states[MAX_NUM_LOCATORS];
} conn_properties_t;

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

void init_connection(void);

conn_properties_t* add_connection(uint16_t connection, bd_addr *address, uint8_t address_type);

uint8_t remove_connection(uint16_t connection);

uint8_t is_connection_list_full(void);

conn_properties_t* get_connection_by_handle(uint16_t connection_handle);
conn_properties_t* get_connection_by_address(bd_addr* address);

aoa_libitems_t* get_libitem_by_locator_address(conn_properties_t* conn, bd_addr* address);

/** @} (end addtogroup app) */
/** @} (end addtogroup Application) */

#ifdef __cplusplus
};
#endif

#endif /* CONN_H */
