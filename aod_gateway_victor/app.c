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
#include "uart.h"
#include "app.h"
#include "mqtt.h"
#include "tcp.h"

#include "conn.h"
#include "aoa.h"
#include "aoa_config.h"
#include "aoa_parse.h"
#include "aoa_util.h"

#define USAGE "\nUsage: %s -t <wstk_address> | -u <serial_port> [-b <baud_rate>] [-f <flow control: 1(on, default) or 0(off)>] [-m <mqtt_address>[:<port>]] [-c <config>] [-v <verbose_level>]\n"
#define DEFAULT_UART_PORT             NULL
#define DEFAULT_UART_BAUD_RATE        115200
#define DEFAULT_UART_FLOW_CONTROL     1
#define DEFAULT_UART_TIMEOUT          100
#define DEFAULT_TCP_PORT              "4901"
#define MAX_OPT_LEN                   255
#define INVALID_IDX              UINT32_MAX
#define CHECK_ERROR(x)           if ((x) != SL_RTL_ERROR_SUCCESS) return (x)
enum axis_list {
  AXIS_X,
  AXIS_Y,
  AXIS_Z,
  AXIS_COUNT
};


SL_BT_API_DEFINE();

static int serial_port_init(char* uartPort, uint32_t uartBaudRate, uint32_t uartFlowControl, int32_t timeout);
static void uart_tx_wrapper(uint32_t len, uint8_t *data);
static void tcp_tx_wrapper(uint32_t len, uint8_t *data);
static void parse_config(char *filename);

// Locator ID
static aoa_id_t locator_id;
static uint32_t locator_count = 0;
static uint32_t asset_tag_count = 0;
typedef struct {
  aoa_id_t id;
  uint32_t loc_id[MAX_NUM_LOCATORS]; // assigned by RTL lib
  sl_rtl_loc_libitem loc;
  sl_rtl_util_libitem filter[AXIS_COUNT];
  aoa_angle_t angle[MAX_NUM_LOCATORS];
  bool ready[MAX_NUM_LOCATORS];
  aoa_position_t position;
} aoa_asset_tag_t;

// MQTT variables
static mqtt_handle_t mqtt_handle = MQTT_DEFAULT_HANDLE;
static char *mqtt_host = NULL;

// Verbose output
uint32_t verbose_level;

typedef struct {
  aoa_id_t id;
  struct sl_rtl_loc_locator_item item;
} aoa_locator_t;

static aoa_locator_t locator_list[MAX_NUM_LOCATORS];
static aoa_asset_tag_t asset_tag_list[MAX_NUM_TAGS];

static aoa_id_t multilocator_id = "";

static char uart_target_port[MAX_OPT_LEN]; // Serail port name of the NCP target
static char tcp_target_address[MAX_OPT_LEN]; // IP address or host name of the NCP target using TCP connection

static void parse_config(char *filename);
static void on_message(const char *topic, const char *payload);
static void publish_position(aoa_asset_tag_t *tag);
static enum sl_rtl_error_code run_estimation(aoa_asset_tag_t *tag);
static enum sl_rtl_error_code init_asset_tag(aoa_asset_tag_t *tag, aoa_id_t id);
static uint32_t find_asset_tag(aoa_id_t id);
static uint32_t find_locator(aoa_id_t id);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(int argc, char *argv[])
{
  int opt;
  uint32_t target_baud_rate = DEFAULT_UART_BAUD_RATE;
  uint32_t target_flow_control = DEFAULT_UART_FLOW_CONTROL;
  char *port_sep;

  uart_target_port[0] = '\0';
  tcp_target_address[0] = '\0';

  aoa_whitelist_init();

  //Parse command line arguments
  while ((opt = getopt(argc, argv, "t:u:b:m:f:i:c:v:h")) != -1) {
    switch (opt) {
      case 'c':
        parse_config(optarg);
        break;
      case 'u': //Target port or address.
        strncpy(uart_target_port, optarg, MAX_OPT_LEN);
        break;
      case 't': //Target TCP address
        strncpy(tcp_target_address, optarg, MAX_OPT_LEN);
        break;
      case 'f': //Target flow control
        target_flow_control = atol(optarg);
        break;
      case 'b': //Target baud rate
        target_baud_rate = atol(optarg);
        break;
      case 'm': //MQTT connection parameter
        // Separate the host and the port field. We chaise only one ':' character
        port_sep = strchr(optarg, ':');
        if (port_sep != NULL) { // Port number given. Replace the ':' with a \0. Port number starts at the next character.
          *port_sep = '\0';
          mqtt_handle.port = atol(port_sep + 1);
        }
        mqtt_host = malloc(strlen(optarg) + 1);
        if (mqtt_host != NULL) {
          strcpy(mqtt_host, optarg);
          mqtt_handle.host = mqtt_host;
        }
        break;
      case 'v':
        verbose_level = atol(optarg);
        break;
      case 'h': //Help!
        app_log(USAGE, argv[0]);
        exit(EXIT_SUCCESS);
      default: /* '?' */
        app_log(USAGE, argv[0]);
        exit(EXIT_FAILURE);
    }
  }

  if (uart_target_port[0] != '\0') {
    // Initialise serial communication as non-blocking.
    SL_BT_API_INITIALIZE_NONBLOCK(uart_tx_wrapper, uartRx, uartRxPeek);
    if (serial_port_init(uart_target_port, target_baud_rate, target_flow_control, DEFAULT_UART_TIMEOUT) < 0) {
      app_log("Non-blocking serial port init failure\n");
      exit(EXIT_FAILURE);
    }
  } else if (tcp_target_address[0] != '\0') {
    // Initialise socket communication
    SL_BT_API_INITIALIZE_NONBLOCK(tcp_tx_wrapper, tcp_rx, tcp_rx_peek);
    if (tcp_open(tcp_target_address, DEFAULT_TCP_PORT) < 0) {
      app_log("Non-blocking TCP connection init failure\n");
      exit(EXIT_FAILURE);
    }
  } else {
    app_log("Either uart port or TCP address shall be given.\n");
    app_log(USAGE, argv[0]);
    exit(EXIT_FAILURE);
  }

  app_log("AoA NCP-host initialised\n");
  app_log("Resetting NCP...\n");
  // Reset NCP to ensure it gets into a defined state.
  // Once the chip successfully boots, boot event should be received.
  sl_bt_system_reset(0);

  // AoA specific init
  aoa_init_buffers();

  init_connection();
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  mqtt_status_t rc;
  bd_addr address;
  uint8_t address_type;

  // Catch boot event...
  if (SL_BT_MSG_ID(evt->header) == sl_bt_evt_system_boot_id) {
    // Print boot message.
    app_log("Bluetooth stack booted: v%d.%d.%d-b%d\n",
            evt->data.evt_system_boot.major,
            evt->data.evt_system_boot.minor,
            evt->data.evt_system_boot.patch,
            evt->data.evt_system_boot.build);
    // Extract unique ID from BT Address.
    sc = sl_bt_system_get_identity_address(&address, &address_type);
    app_assert(sc == SL_STATUS_OK,
               "[E: 0x%04x] Failed to get Bluetooth address\n",
               (int)sc);
    app_log("Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\n",
            address_type ? "static random" : "public device",
            address.addr[5],
            address.addr[4],
            address.addr[3],
            address.addr[2],
            address.addr[1],
            address.addr[0]);

    aoa_address_to_id(address.addr, address_type, locator_id);

    // Connect to the MQTT broker
    mqtt_handle.client_id = locator_id;
    mqtt_handle.on_connect = aoa_on_connect;
    rc = mqtt_init(&mqtt_handle);
    app_assert(rc == MQTT_SUCCESS, "MQTT init failed.\n");
  }
  // ...then call the connection specific event handler.
  app_bt_on_event(evt);
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void)
{
  mqtt_step(&mqtt_handle);
}

/**************************************************************************//**
 * UART TX Wrapper.
 *****************************************************************************/
static void uart_tx_wrapper(uint32_t len, uint8_t *data)
{
  if (0 > uartTx(len, data)) {
    app_log("Failed to write to serial port\n");
    exit(EXIT_FAILURE);
  }
}

/**************************************************************************//**
 * TCP TX Wrapper.
 *****************************************************************************/
static void tcp_tx_wrapper(uint32_t len, uint8_t *data)
{
  if (0 > tcp_tx(len, data)) {
    app_log("Failed to write to TCP port\n");
    tcp_close();
    exit(EXIT_FAILURE);
  }
}

/**************************************************************************//**
 * Initialise serial port.
 *****************************************************************************/
static int serial_port_init(char* uartPort, uint32_t uartBaudRate, uint32_t uartFlowControl, int32_t timeout)
{
  int ret;

  // Sanity check of arguments.
  if (!uartPort || !uartBaudRate || (uartFlowControl > 1)) {
    app_log("Serial port setting error.\n");
    ret = -1;
  } else {
    ret = uartOpen((int8_t*)uartPort, uartBaudRate, uartFlowControl, timeout);
  }

  // Initialise the serial port with RTS/CTS enabled.
  return ret;
}

uint8_t find_service_in_advertisement(uint8_t *advdata, uint8_t advlen, uint8_t *service_uuid)
{
  uint8_t ad_field_length;
  uint8_t ad_field_type;
  uint8_t *ad_uuid_field;
  uint32_t i = 0;
  uint32_t next_ad_structure;
  uint8_t ret = 0;

  // Parse advertisement packet
  while (i < advlen) {
    ad_field_length = advdata[i];
    ad_field_type = advdata[i + 1];
    next_ad_structure = i + ad_field_length + 1;
    // incomplete or complete UUIDs
    if (ad_field_type == AD_FIELD_I || ad_field_type == AD_FILED_C) {
      // compare UUID to the service UUID to be found
      for (ad_uuid_field = advdata + i + 2; ad_uuid_field < advdata + next_ad_structure; ad_uuid_field += SERVICE_UUID_LEN) {
        if (memcmp(ad_uuid_field, service_uuid, SERVICE_UUID_LEN) == 0) {
          ret = 1;
          break;
        }
      }
      if (ret == 1) {
        break;
      }
    }
    // advance to the next AD struct
    i = next_ad_structure;
  }
  return ret;
}

void app_deinit(void)
{
  app_log("Shutting down.\n");
  mqtt_deinit(&mqtt_handle);
  if (uart_target_port[0] != '\0') {
    uartClose();
  } else if (tcp_target_address[0] != '\0') {
    tcp_close();
  }
  if (mqtt_host != NULL) {
    free(mqtt_host);
  }
}

void app_on_iq_report(conn_properties_t *tag, aoa_iq_report_t *iq_report)
{
  aoa_angle_t angle;
  aoa_id_t tag_id;
  mqtt_status_t rc;
  uint32_t loc_idx;
  char *payload;
  const char topic_template[] = AOA_TOPIC_ANGLE_PRINT;
  char topic[sizeof(topic_template) + sizeof(aoa_id_t) + sizeof(aoa_id_t)];

  aoa_address_to_id(iq_report->address.addr, iq_report->address_type, locator_id);
   // Find locator.
  loc_idx = find_locator(locator_id);
  // app_assert(loc_idx != INVALID_IDX, "Failed to find locator %s.\n", loc_id);
  if(loc_idx == INVALID_IDX)
  {
      app_log("Failed to find locator %s.\n", locator_id);
      return;
  }

  //if (aoa_calculate(&tag->aoa_states, iq_report, &angle) != SL_STATUS_OK) {
  if (aoa_calculate(get_libitem_by_locator_address(tag, &iq_report->address), iq_report, &angle) != SL_STATUS_OK) {
    return;
  }

  // Compile topic
  aoa_address_to_id(tag->address.addr, tag->address_type, tag_id);
  // aoa_address_to_id(iq_report->address.addr, iq_report->address_type, locator_id);
  // app_log("locator id = %s",locator_id);
  snprintf(topic, sizeof(topic), topic_template, locator_id, tag_id);

  // Compile payload
  aoa_angle_to_string(&angle, &payload);

  on_message(topic, payload);

  // Send message
  rc = mqtt_publish(&mqtt_handle, topic, payload);
  app_assert(rc == MQTT_SUCCESS, "Failed to publish to topic '%s'.\n", topic);

  // Clean up
  free(payload);
}

/**************************************************************************//**
 * Check if all data are ready for a tag to run estimation.
 *****************************************************************************/
static bool is_ready(aoa_asset_tag_t *tag)
{
  for (uint32_t i = 0; i < locator_count; i++) {
    if (tag->ready[i] == false) {
      return false;
    }
  }
  return true;
}

static void on_message(const char *topic, const char *payload)
{
  int result;
  aoa_id_t loc_id, tag_id;
  uint32_t loc_idx, tag_idx;
  aoa_asset_tag_t *tag;
  enum sl_rtl_error_code sc;

  // Parse topic.
  result = sscanf(topic, AOA_TOPIC_ANGLE_SCAN, loc_id,tag_id);
  app_assert(result == 2, "Failed to parse angle topic: %d.\n", result);

  // Find locator.
  loc_idx = find_locator(loc_id);
  // app_assert(loc_idx != INVALID_IDX, "Failed to find locator %s.\n", loc_id);
  if(loc_idx == INVALID_IDX)
  {
      app_log("Failed to find locator %s.\n", loc_id);
      return;
  }
  


  // Find asset tag.
  tag_idx = find_asset_tag(tag_id);

  if (tag_idx == INVALID_IDX) {
    if (asset_tag_count < MAX_NUM_TAGS) {
      // Add new tag
      sc = init_asset_tag(&asset_tag_list[asset_tag_count], tag_id);
      app_assert(sc == SL_RTL_ERROR_SUCCESS,
                 "[E: 0x%04x] Failed to init asset tag %s.\n", sc, tag_id);
      app_log("New tag added (%d): %s\n", asset_tag_count, tag_id);
      tag_idx = asset_tag_count++;
    } else {
      app_log("Warning! Maximum number of asset tags reached: %d\n", asset_tag_count);
      // No further procesing possible.
      return;
    }
  }

  // Create shortcut.
  tag = &asset_tag_list[tag_idx];

  // Parse payload.
  aoa_string_to_angle((char *)payload, &tag->angle[loc_idx]);
  tag->ready[loc_idx] = true;

  // Run estimation and publish results.
  if (is_ready(tag)) {
    sc = run_estimation(tag);
    app_assert(sc == SL_RTL_ERROR_SUCCESS,
               "[E: 0x%04x] Position estimation failed for %s.\n", sc, tag->id);
    publish_position(tag);
  }
}


/**************************************************************************//**
 * Run position estimation algorithm for a given asset tag.
 *****************************************************************************/
static enum sl_rtl_error_code run_estimation(aoa_asset_tag_t *tag)
{
  enum sl_rtl_error_code sc;

  // Feed measurement values into RTL lib.
  for (uint32_t i = 0; i < locator_count; i++) {
    sc = sl_rtl_loc_set_locator_measurement(&tag->loc,
                                            tag->loc_id[i],
                                            SL_RTL_LOC_LOCATOR_MEASUREMENT_AZIMUTH,
                                            tag->angle[i].azimuth);
    CHECK_ERROR(sc);

    sc = sl_rtl_loc_set_locator_measurement(&tag->loc,
                                            tag->loc_id[i],
                                            SL_RTL_LOC_LOCATOR_MEASUREMENT_ELEVATION,
                                            tag->angle[i].elevation);
    CHECK_ERROR(sc);

    // Feeding RSSI distance measurement to the RTL library improves location
    // accuracy when the measured distance is reasonably correct.
    // If the received signal strength of the incoming signal is altered for any
    // other reason than the distance between the TX and RX itself, it will lead
    // to incorrect measurement and it will lead to incorrect position estimates.
    // For this reason the RSSI distance usage is disabled by default in the
    // multilocator case.
    // Single locator mode however always requires the distance measurement in
    // addition to the angle, please note the if-condition below.
    // In case the distance estimation should be used in the  multilocator case,
    // you can enable it by commenting out the condition.
    if (locator_count == 1) {
      sc = sl_rtl_loc_set_locator_measurement(&tag->loc,
                                              tag->loc_id[i],
                                              SL_RTL_LOC_LOCATOR_MEASUREMENT_DISTANCE,
                                              tag->angle[i].distance);
      CHECK_ERROR(sc);
    }
    tag->ready[i] = false;
  }

  // Process new measurements, time step given in seconds.
  sc = sl_rtl_loc_process(&tag->loc, ESTIMATION_INTERVAL_SEC);
  CHECK_ERROR(sc);

  // Get results from the estimator.
  sc = sl_rtl_loc_get_result(&tag->loc, SL_RTL_LOC_RESULT_POSITION_X, &tag->position.x);
  CHECK_ERROR(sc);
  sc = sl_rtl_loc_get_result(&tag->loc, SL_RTL_LOC_RESULT_POSITION_Y, &tag->position.y);
  CHECK_ERROR(sc);
  sc = sl_rtl_loc_get_result(&tag->loc, SL_RTL_LOC_RESULT_POSITION_Z, &tag->position.z);
  CHECK_ERROR(sc);

  // Apply filter on the result.
  sc = sl_rtl_util_filter(&tag->filter[AXIS_X], tag->position.x, &tag->position.x);
  CHECK_ERROR(sc);
  sc = sl_rtl_util_filter(&tag->filter[AXIS_Y], tag->position.y, &tag->position.y);
  CHECK_ERROR(sc);
  sc = sl_rtl_util_filter(&tag->filter[AXIS_Z], tag->position.z, &tag->position.z);
  CHECK_ERROR(sc);

  // Clear measurements.
  sc = sl_rtl_loc_clear_measurements(&tag->loc);
  CHECK_ERROR(sc);

  return SL_RTL_ERROR_SUCCESS;
}

/**************************************************************************//**
 * Publish position of a given tag.
 *****************************************************************************/
static void publish_position(aoa_asset_tag_t *tag)
{
 mqtt_status_t rc;
  char *payload;
  const char topic_template[] = AOA_TOPIC_POSITION_PRINT;
  char topic[sizeof(topic_template) + sizeof(aoa_id_t) + sizeof(aoa_id_t)];

  // Compile topic.
  snprintf(topic, sizeof(topic), topic_template, multilocator_id, tag->id);

  // Compile payload.
  aoa_position_to_string(&tag->position, &payload);
  
  printf("%s\r\n", payload);

  rc = mqtt_publish(&mqtt_handle, topic, payload);
  app_assert(rc == MQTT_SUCCESS, "Failed to publish to topic '%s'.\n", topic);

  // Clean up.
  free(payload);
}


/**************************************************************************//**
 * Initialise a new asset tag.
 *****************************************************************************/
static enum sl_rtl_error_code init_asset_tag(aoa_asset_tag_t *tag, aoa_id_t id)
{
  enum sl_rtl_error_code sc;

  aoa_id_copy(tag->id, id);

  // Initialize RTL library
  sc = sl_rtl_loc_init(&tag->loc);
  CHECK_ERROR(sc);

  // Select estimation mode.
  sc = sl_rtl_loc_set_mode(&tag->loc, ESTIMATION_MODE);
  CHECK_ERROR(sc);

  // Provide locator configurations to the position estimator.
  for (uint32_t i = 0; i < locator_count; i++) {
    sc = sl_rtl_loc_add_locator(&tag->loc, &locator_list[i].item, &tag->loc_id[i]);
    CHECK_ERROR(sc);
    tag->ready[i] = false;
  }

  // Create position estimator.
  sc = sl_rtl_loc_create_position_estimator(&tag->loc);
  CHECK_ERROR(sc);

  // Initialize util functions.
  for (enum axis_list i = 0; i < AXIS_COUNT; i++) {
    sc = sl_rtl_util_init(&tag->filter[i]);
    CHECK_ERROR(sc);
    // Set position filtering parameter for every axis.
    sc = sl_rtl_util_set_parameter(&tag->filter[i],
                                   SL_RTL_UTIL_PARAMETER_AMOUNT_OF_FILTERING,
                                   FILTERING_AMOUNT);
    CHECK_ERROR(sc);
  }

  return SL_RTL_ERROR_SUCCESS;
}

/**************************************************************************//**
 * Find asset tag in the local list based on its ID.
 *****************************************************************************/
static uint32_t find_asset_tag(aoa_id_t id)
{
  uint32_t retval = INVALID_IDX;

  for (uint32_t i = 0; (i < asset_tag_count) && (retval == INVALID_IDX); i++) {
    if (aoa_id_compare(asset_tag_list[i].id, id) == 0) {
      retval = i;
    }
  }
  return retval;
}

/**************************************************************************//**
 * Find locator in the local list based on its ID.
 *****************************************************************************/
static uint32_t find_locator(aoa_id_t id)
{
  uint32_t retval = INVALID_IDX;

  for (uint32_t i = 0; (i < locator_count) && (retval == INVALID_IDX); i++) {
    if (aoa_id_compare(locator_list[i].id, id) == 0) {
      retval = i;
    }
  }
  return retval;
}
/**************************************************************************//**
 * Configuration file parser.
 *****************************************************************************/
static void parse_config(char *filename)
{
  sl_status_t sc;
  char *buffer;
  aoa_locator_t *loc;

  buffer = load_file(filename);
  app_assert(buffer != NULL, "Failed to load file: %s\n", filename);

  sc = aoa_parse_init(buffer);
  app_assert(sc == SL_STATUS_OK,
             "[E: 0x%04x] aoa_parse_init failed\n",
             (int)sc);

  sc = aoa_parse_multilocator(multilocator_id);
  app_assert(sc == SL_STATUS_OK,
             "[E: 0x%04x] aoa_parse_multilocator failed\n",
             (int)sc);

  do {
    loc = &locator_list[locator_count];
    sc = aoa_parse_locator(loc->id, &loc->item);
    if (sc == SL_STATUS_OK) {
      app_log("Locator added: id: %s, coordinate: %f %f %f, orientation: %f %f %f\n",
              loc->id,
              loc->item.coordinate_x,
              loc->item.coordinate_y,
              loc->item.coordinate_z,
              loc->item.orientation_x_axis_degrees,
              loc->item.orientation_y_axis_degrees,
              loc->item.orientation_z_axis_degrees);
      ++locator_count;
    } else {
      app_assert(sc == SL_STATUS_NOT_FOUND,
                 "[E: 0x%04x] aoa_parse_locator failed\n",
                 (int)sc);
    }
  } while ((locator_count < MAX_NUM_LOCATORS) && (sc == SL_STATUS_OK));

  app_log("Locator count: %d\n", locator_count);

  sc = aoa_parse_deinit();
  app_assert(sc == SL_STATUS_OK,
             "[E: 0x%04x] aoa_parse_deinit failed\n",
             (int)sc);

  free(buffer);
}

// static void parse_config(char *filename)
// {
//   sl_status_t sc;
//   char *buffer;
//   aoa_id_t id;

//   buffer = load_file(filename);
//   app_assert(buffer != NULL, "Failed to load file: %s\n", filename);

//   sc = aoa_parse_init(buffer);
//   app_assert(sc == SL_STATUS_OK,
//              "[E: 0x%04x] aoa_parse_init failed\n",
//              (int)sc);

//   sc = aoa_parse_azimuth(&aoa_azimuth_min, &aoa_azimuth_max);
//   app_assert((sc == SL_STATUS_OK) || (sc == SL_STATUS_NOT_FOUND),
//              "[E: 0x%04x] aoa_parse_azimuth failed\n",
//              (int)sc);

//   do {
//     sc = aoa_parse_whitelist(id);
//     if (sc == SL_STATUS_OK) {
//       app_log("Adding tag id '%s' to the whitelist.\n", id);
//       sc = aoa_whitelist_add(id);
//     } else {
//       app_assert(sc == SL_STATUS_NOT_FOUND,
//                  "[E: 0x%04x] aoa_parse_whitelist failed\n",
//                  (int)sc);
//     }
//   } while (sc == SL_STATUS_OK);

//   sc = aoa_parse_deinit();
//   app_assert(sc == SL_STATUS_OK,
//              "[E: 0x%04x] aoa_parse_deinit failed\n",
//              (int)sc);

//   free(buffer);
// }
