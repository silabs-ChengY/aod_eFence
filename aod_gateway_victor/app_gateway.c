/***************************************************************************//**
 * @file
 * @brief Connection oriented AoA locator application.
 *
 * AoA locator application for connection oriented implementation.
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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include "system.h"
#include "sl_bt_api.h"
#include "sl_bt_ncp_host.h"
#include "app_log.h"
#include "app_assert.h"

#include "conn.h"

#include "aoa.h"
#include "app.h"
#include "aoa_util.h"
#include "app_config.h"
#include "aoa_types.h"


// connection parameters
#define CONN_INTERVAL_MIN             80   //100ms
#define CONN_INTERVAL_MAX             80   //100ms
#define CONN_SLAVE_LATENCY            0    //no latency
#define CONN_TIMEOUT                  100  //1000ms
#define CONN_MIN_CE_LENGTH            0
#define CONN_MAX_CE_LENGTH            0xffff

#define CTE_TYPE_AOD_1US              1

//02 9f 62 37-37 c1-7b 10-92 14-f3 61 06 6e 22 45
static const uint8_t iq_service[SERVICE_UUID_LEN] = { 0x45, 0x22, 0x6e, 0x06,
                                                       0x61, 0xf3, 0x14, 0x92,
                                                       0x10, 0x7b, 0xc1, 0x37,
                                                       0x37, 0x62, 0x9f, 0x02 };
//77 8c 62 cc-39 8b-c2 06-21 99-af 3b 80 55 3a d2
static const uint8_t iq_report_char[CHAR_UUID_LEN] = { 0xd2, 0x3a, 0x55, 0x80,
                                                        0x3b, 0xaf, 0x99, 0x21,
                                                        0x06, 0xc2, 0x8b, 0x39,
                                                        0xcc, 0x62, 0x8c, 0x77 };

// Antenna switching pattern
//static const uint8_t antenna_array[AOA_NUM_ARRAY_ELEMENTS] = SWITCHING_PATTERN;

/**************************************************************************//**
 * Connection specific Bluetooth event handler.
 *****************************************************************************/
void app_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  conn_properties_t* conn;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      // Set passive scanning on 1Mb PHY
      sc = sl_bt_scanner_set_mode(gap_1m_phy, SCAN_PASSIVE);

      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to set scanner mode\n",
                 (int)sc);

      // Set scan interval and scan window
      sc = sl_bt_scanner_set_timing(gap_1m_phy, SCAN_INTERVAL, SCAN_WINDOW);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to set scanner timing\n",
                 (int)sc);

      // Set the default connection parameters for subsequent connections
      sc = sl_bt_connection_set_default_parameters(CONN_INTERVAL_MIN,
                                                   CONN_INTERVAL_MAX,
                                                   CONN_SLAVE_LATENCY,
                                                   CONN_TIMEOUT,
                                                   CONN_MIN_CE_LENGTH,
                                                   CONN_MAX_CE_LENGTH);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to set parameters\n",
                 (int)sc);
      // Start scanning - looking for tags
      sc = sl_bt_scanner_start(gap_1m_phy, scanner_discover_generic);
      app_assert(sc == SL_STATUS_OK || sc == SL_STATUS_INVALID_STATE,
                 "[E: 0x%04x] Failed to start scanner\n",
                 (int)sc);

      app_log("Start scanning...\n");

      break;

    case sl_bt_evt_scanner_scan_report_id:
      // Check if the tag is whitelisted
    {
      aoa_id_t local_id;
      aoa_address_to_id(evt->data.evt_scanner_scan_report.address.addr,
                        evt->data.evt_scanner_scan_report.address_type,
                        local_id);
      if (SL_STATUS_NOT_FOUND == aoa_whitelist_find(local_id)) {
        if (verbose_level > 0 ) {
          app_log("Tag is not on the whitelist, ignoring.\n");
        }

        break;
      }
      // Check for connectable advertising type
      if ((evt->data.evt_scanner_scan_report.packet_type & 0x06) != 0x0) {
        break;
      }
      // If a CTE service is found...
      if (find_service_in_advertisement(&(evt->data.evt_scanner_scan_report.data.data[0]),
                                        evt->data.evt_scanner_scan_report.data.len,
                                        (uint8_t *) iq_service) != 0) {
        conn = get_connection_by_address(&evt->data.evt_scanner_scan_report.address);
        if (!is_connection_list_full() && conn == NULL) {
          uint8_t conn_handle;
          sc = sl_bt_scanner_stop();
          app_assert(sc == SL_STATUS_OK || sc == SL_STATUS_INVALID_STATE,
                     "[E: 0x%04x] Failed to stop scanner\n",
                     (int)sc);
          sc = sl_bt_connection_open(evt->data.evt_scanner_scan_report.address,
                                     evt->data.evt_scanner_scan_report.address_type,
                                     gap_1m_phy, &conn_handle);
          app_assert(sc == SL_STATUS_OK || sc == SL_STATUS_INVALID_STATE,
                     "[E: 0x%04x] Failed to open connection\n",
                     (int)sc);
        }
      }
      break;
    }
    case sl_bt_evt_connection_opened_id:
      // Add connection to the connection_properties array
      add_connection((uint16_t)evt->data.evt_connection_opened.connection,
                     &evt->data.evt_connection_opened.address,
                     evt->data.evt_connection_opened.address_type);
      // Discover CTE service on the slave device
      sc = sl_bt_gatt_discover_primary_services_by_uuid(evt->data.evt_connection_opened.connection,
                                                        SERVICE_UUID_LEN,
                                                        iq_service);
      app_assert(sc == SL_STATUS_OK,
                 "[E: 0x%04x] Failed to discover primary services\n",
                 (int)sc);
      app_log("Connected to tag. Discovering services...\r\n");

      app_log("Tag's Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\n",
              evt->data.evt_connection_opened.address_type ? "static random" : "public device",
              evt->data.evt_connection_opened.address.addr[5],
              evt->data.evt_connection_opened.address.addr[4],
              evt->data.evt_connection_opened.address.addr[3],
              evt->data.evt_connection_opened.address.addr[2],
              evt->data.evt_connection_opened.address.addr[1],
              evt->data.evt_connection_opened.address.addr[0]);

      break;

    // This event is generated when a new service is discovered
    case sl_bt_evt_gatt_service_id:
      // Find connection
      app_log("Got service handle\n");
      if ((conn = get_connection_by_handle(evt->data.evt_gatt_service.connection)) == NULL) {
        break;
      }
      // Save service handle for future reference
      if (memcmp((uint8_t*)&(evt->data.evt_gatt_service.uuid.data[0]),
                 iq_service, SERVICE_UUID_LEN) == 0) {
        conn->cte_service_handle = evt->data.evt_gatt_service.service;
      }
      break;

    // This event is generated when a new characteristic is discovered
    case sl_bt_evt_gatt_characteristic_id:
      // Find connection
      app_log("Got new characteristic\n");
      if ((conn = get_connection_by_handle(evt->data.evt_gatt_characteristic.connection)) == NULL) {
        break;
      }
      // Save characteristic handle for future reference
      if (memcmp((uint8_t*)&(evt->data.evt_gatt_characteristic.uuid.data[0]),
                 iq_report_char, CHAR_UUID_LEN) == 0) {
        conn->cte_enable_char_handle = evt->data.evt_gatt_characteristic.characteristic;
      }
      conn->connection_state = DISCOVER_CHARACTERISTICS;
      break;

    // This event is generated for various procedure completions, e.g. when a
    // write procedure is completed, or service discovery is completed
    case sl_bt_evt_gatt_procedure_completed_id:
      // Find connection
      if ((conn = get_connection_by_handle(evt->data.evt_gatt_procedure_completed.connection)) == NULL) {
        break;
      }

      switch (conn->connection_state) {
        // If service discovery finished
        case DISCOVER_SERVICES:
          app_log("Service discovering finished.\n");
          // Discover CTE enable characteristic on the slave device
          sc = sl_bt_gatt_discover_characteristics_by_uuid(evt->data.evt_gatt_procedure_completed.connection,
                                                           conn->cte_service_handle,
                                                           CHAR_UUID_LEN,
                                                           iq_report_char);
          app_assert(sc == SL_STATUS_OK,
                     "[E: 0x%04x] Failed to discover characteristics\n",
                     (int)sc);
          break;
        // If characteristic discovery finished
        case DISCOVER_CHARACTERISTICS:
          app_log("Services discovered. Enabling CTE...\n");
          //uint8_t data = 0x01;

          // enable CTE on slave device (by writing 0x01 into the CTE enable characteristic)
          //sc = sl_bt_gatt_write_characteristic_value(evt->data.evt_gatt_procedure_completed.connection,
          //                                           conn->iq_report_char_handle,
          //                                           1,
          //                                           &data);

          sc = sl_bt_gatt_set_characteristic_notification(evt->data.evt_gatt_procedure_completed.connection,
                                                          conn->cte_enable_char_handle,
                                                          0x01);

          app_assert(sc == SL_STATUS_OK,
                     "[E: 0x%04x] Failed to enable notifications\n",
                     (int)sc);
          conn->connection_state = ENABLE_CTE;

          break;
        case ENABLE_CTE:
          // If CTE was enabled
          app_log("IQ sample notifications enabled. Start receiving IQ samples...\n");
          conn->connection_state = RUNNING;
          break;
        default:
          break;
      }
      break;
    // This event is generated when a connection is dropped
    case sl_bt_evt_connection_closed_id:
      // remove connection from active connections
      app_log("Connection lost.\n");
      remove_connection((uint16_t)evt->data.evt_connection_closed.connection);
      // Restart the scanner to discover new tags
      sc = sl_bt_scanner_start(gap_1m_phy, scanner_discover_generic);
      app_assert(sc == SL_STATUS_OK || sc == SL_STATUS_INVALID_STATE,
                 "[E: 0x%04x] Failed to start scanner\n",
                 (int)sc);

      break;

    //case sl_bt_evt_cte_receiver_connection_iq_report_id:
    case sl_bt_evt_gatt_characteristic_value_id: 
    {
      sl_bt_evt_cte_receiver_silabs_iq_report_t *iq_report_ptr;
      aoa_iq_report_t iq_report;
      //uint8_t iq_report_len;

      //printf("IQ samples received\r\n");
      //fflush(stdout);

      // Check if asset tag is known.
      if ((conn = get_connection_by_handle(evt->data.evt_gatt_characteristic_value.connection)) == NULL) {
        break;
      }

      //iq_report_len = evt->data.evt_gatt_characteristic_value.value.len;
      iq_report_ptr = (sl_bt_evt_cte_receiver_silabs_iq_report_t*)&evt->data.evt_gatt_characteristic_value.value.data;

      if (iq_report_ptr->samples.len == 0) {
        // Nothing to be processed.
        break;
      }

      if (get_libitem_by_locator_address(conn, &iq_report_ptr->address) == NULL){
        memcpy(&conn->aoa_states[conn->num_locators_found].locator_addr,&iq_report_ptr->address,sizeof(bd_addr));
        aoa_init(&(conn->aoa_states[conn->num_locators_found]));
        conn->num_locators_found++;
      }

      // Convert event to common IQ report format.
      iq_report.address = iq_report_ptr->address;
      iq_report.address_type = iq_report_ptr->address_type;
      iq_report.channel = iq_report_ptr->channel;
      iq_report.rssi = iq_report_ptr->rssi;
      iq_report.event_counter = iq_report_ptr->packet_counter;
      iq_report.length = iq_report_ptr->samples.len;
      iq_report.samples = (int8_t *)&iq_report_ptr->samples.data[0];

      app_on_iq_report(conn, &iq_report);
    }
    break;

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}