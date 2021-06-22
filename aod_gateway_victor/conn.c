/***********************************************************************************************//**
 * @file
 * @brief  Connection handler module, responsible for storing states of open connections
 ***************************************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
 ***************************************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this software is governed
 * by the terms of Silicon Labs Master Software License Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This software is distributed to
 * you in Source Code format and is governed by the sections of the MSLA applicable to Source Code.
 **************************************************************************************************/

#include <string.h>
#include "app_config.h"
#include "conn.h"

#define CONNECTION_HANDLE_INVALID     (uint16_t)0xFFFFu
#define SERVICE_HANDLE_INVALID        (uint32_t)0xFFFFFFFFu
#define CHARACTERISTIC_HANDLE_INVALID (uint16_t)0xFFFFu
#define TABLE_INDEX_INVALID           (uint8_t)0xFFu

/***************************************************************************************************
 * Static Variable Declarations
 **************************************************************************************************/

// Array for holding properties of multiple (parallel) connections
static conn_properties_t conn_properties[MAX_NUM_TAGS];

// Counter of active connections
static uint8_t active_connections_num;

/***************************************************************************************************
 * Public Function Definitions
 **************************************************************************************************/
void init_connection(void)
{
  uint8_t i;
  active_connections_num = 0;

  // Initialize connection state variables
  for (i = 0; i < MAX_NUM_TAGS; i++) {
    conn_properties[i].connection_handle = CONNECTION_HANDLE_INVALID;
    conn_properties[i].cte_service_handle = SERVICE_HANDLE_INVALID;
    conn_properties[i].cte_enable_char_handle = CHARACTERISTIC_HANDLE_INVALID;
  }
}

conn_properties_t* add_connection(uint16_t connection, bd_addr *address, uint8_t address_type)
{
  conn_properties_t* ret = NULL;

  // If there is place to store new connection
  if (active_connections_num < MAX_NUM_TAGS) {
    // Store the connection handle, and the server address
    conn_properties[active_connections_num].connection_handle = connection;
    conn_properties[active_connections_num].address = *address;
    conn_properties[active_connections_num].address_type = address_type;
    conn_properties[active_connections_num].connection_state = DISCOVER_SERVICES;
    //aoa_init(&conn_properties[active_connections_num].aoa_states);
    conn_properties[active_connections_num].num_locators_found = 0;
    for (uint8_t i=0; i < MAX_NUM_LOCATORS; i++){
      conn_properties[active_connections_num].aoa_states[i].libitem = NULL;
      conn_properties[active_connections_num].aoa_states[i].util_libitem = NULL;
    }
    // Entry is now valid
    ret = &conn_properties[active_connections_num];
    active_connections_num++;
  }
  return ret;
}

uint8_t remove_connection(uint16_t connection)
{
  uint8_t i;
  uint8_t table_index = TABLE_INDEX_INVALID;

  // If there are no open connections, return error
  if (active_connections_num == 0) {
    return 1;
  }

  // Find the table index of the connection to be removed
  for (uint8_t i = 0; i < active_connections_num; i++) {
    if (conn_properties[i].connection_handle == connection) {
      table_index = i;
      break;
    }
  }

  // If connection not found, return error
  if (table_index == TABLE_INDEX_INVALID) {
    return 1;
  }

  for (uint8_t i = 0; i < conn_properties[table_index].num_locators_found; i++) {
    aoa_deinit(&conn_properties[table_index].aoa_states[i]);
  }
  // Decrease number of active connections
  active_connections_num--;

  // Shift entries after the removed connection toward 0 index
  for (i = table_index; i < active_connections_num; i++) {
    conn_properties[i] = conn_properties[i + 1];
  }

  // Clear the slots we've just removed so no junk values appear
  for (i = active_connections_num; i < MAX_NUM_TAGS; i++) {
    conn_properties[i].connection_handle = CONNECTION_HANDLE_INVALID;
    conn_properties[i].cte_service_handle = SERVICE_HANDLE_INVALID;
    conn_properties[i].cte_enable_char_handle = CHARACTERISTIC_HANDLE_INVALID;
  }

  return 0;
}

uint8_t is_connection_list_full(void)
{
  // Return if connection state table is full
  return (active_connections_num >= MAX_NUM_TAGS);
}

conn_properties_t* get_connection_by_handle(uint16_t connection_handle)
{
  conn_properties_t* ret = NULL;
  // Find the connection state entry in the table corresponding to the connection handle
  for (uint8_t i = 0; i < active_connections_num; i++) {
    if (conn_properties[i].connection_handle == connection_handle) {
      // Return a pointer to the connection state entry
      ret = &conn_properties[i];
      break;
    }
  }
  // Return error if connection not found
  return ret;
}

conn_properties_t* get_connection_by_address(bd_addr* address)
{
  conn_properties_t* ret = NULL;
  // Find the connection state entry in the table corresponding to the connection address
  for (uint8_t i = 0; i < active_connections_num; i++) {
    if (0 == memcmp(address, &(conn_properties[i].address), sizeof(bd_addr))) {
      // Return a pointer to the connection state entry
      ret = &conn_properties[i];
      break;
    }
  }
  // Return error if connection not found
  return ret;
}

aoa_libitems_t* get_libitem_by_locator_address(conn_properties_t* conn, bd_addr* address)
{
  aoa_libitems_t* ret = NULL;
  // Find the connection state entry in the table corresponding to the connection address
  for (uint8_t i = 0; i < conn->num_locators_found; i++) {
    if (0 == memcmp(address, &(conn->aoa_states[i].locator_addr), sizeof(bd_addr))) {
      // Return a pointer to the connection state entry
      ret = &(conn->aoa_states[i]);
      break;
    }
  }
  // Return error if connection not found
  return ret;
}