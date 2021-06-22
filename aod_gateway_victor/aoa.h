/***********************************************************************************************//**
 * @file
 * @brief  AoA header file
 ***************************************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 ***************************************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this software is governed
 * by the terms of Silicon Labs Master Software License Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This software is distributed to
 * you in Source Code format and is governed by the sections of the MSLA applicable to Source Code.
 **************************************************************************************************/

#ifndef AOA_H
#define AOA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include "aoa_types.h"
#include "sl_bt_api.h"
#include "sl_rtl_clib_api.h"
#include "sl_ncp_evt_filter_common.h"

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

typedef struct aoa_libitems {
  bd_addr locator_addr;
  sl_rtl_aox_libitem libitem;
  sl_rtl_util_libitem util_libitem;
} aoa_libitems_t;

/***************************************************************************************************
 * Public variables
 **************************************************************************************************/
extern float aoa_azimuth_min;
extern float aoa_azimuth_max;

/***************************************************************************************************
 * Function Declarations
 **************************************************************************************************/

void aoa_init_buffers(void);
void aoa_init(aoa_libitems_t *aoa_state);
sl_status_t aoa_calculate(aoa_libitems_t *aoa_state, aoa_iq_report_t *iq_report, aoa_angle_t *angle);
sl_status_t aoa_deinit(aoa_libitems_t *aoa_state);

/** @} (end addtogroup app) */
/** @} (end addtogroup Application) */

#ifdef __cplusplus
};
#endif

#endif /* AOA_H */
