/***************************************************************************//**
 * @file
 * @brief Application configuration values.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <math.h>
#include "sl_rtl_clib_api.h"

// -----------------------------------------------------------------------------
// Primary configuration values.

// Maximum number of asset tags handled by the application.
#define MAX_NUM_TAGS                30

// Maximum number of locators handled by the application.
#define MAX_NUM_LOCATORS                8



// AoA antenna array type
#define AOX_ARRAY_TYPE                 SL_RTL_AOX_ARRAY_TYPE_4x4_URA

// AoA estimator mode
#define AOX_MODE                       SL_RTL_AOX_MODE_REAL_TIME_BASIC

// Location estimation mode.
#define ESTIMATION_MODE         SL_RTL_LOC_ESTIMATION_MODE_THREE_DIM_HIGH_ACCURACY

// Estimation interval in seconds.
// This value should approximate the time interval between two consecutive CTEs.
#define ESTIMATION_INTERVAL_SEC 0.1f

// Reference RSSI value of the asset tag at 1.0 m distance in dBm.
#define TAG_TX_POWER                   (-45.0)

// Filter weight applied on the estimated distance. Ranges from 0 to 1.
#define FILTERING_AMOUNT               0.6f

// Default value for the lower bound of the azimuth mask.
// Can be overridden with runtime configuration. Use NAN to disable.
#define AOA_AZIMUTH_MASK_MIN_DEFAULT   NAN

// Default value for the upper bound of the azimuth mask.
// Can be overridden with runtime configuration. Use NAN to disable.
#define AOA_AZIMUTH_MASK_MAX_DEFAULT   NAN

// Measurement interval expressed as the number of connection events.
#define CTE_SAMPLING_INTERVAL          3

// Minimum CTE length requested in 8 us units. Ranges from 16 to 160 us.
#define CTE_MIN_LENGTH                 20

// Maximum number of sampled CTEs in each advertising interval.
// 0: Sample and report all available CTEs.
#define CTE_COUNT                      0

// Switching and sampling slots in us (1 or 2).
#define CTE_SLOT_DURATION              1

// -----------------------------------------------------------------------------
// Secondary configuration values based on primary values.

#if (AOX_ARRAY_TYPE == SL_RTL_AOX_ARRAY_TYPE_4x4_URA)
#define AOA_NUM_SNAPSHOTS       (4)
#define AOA_NUM_ARRAY_ELEMENTS  (4 * 4)
#define AOA_REF_PERIOD_SAMPLES  (7)
#define SWITCHING_PATTERN       { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
#elif (AOX_ARRAY_TYPE == SL_RTL_AOX_ARRAY_TYPE_3x3_URA)
#define AOA_NUM_SNAPSHOTS       (4)
#define AOA_NUM_ARRAY_ELEMENTS  (3 * 3)
#define AOA_REF_PERIOD_SAMPLES  (7)
#define SWITCHING_PATTERN       { 1, 2, 3, 5, 6, 7, 9, 10, 11 }
#elif (AOX_ARRAY_TYPE == SL_RTL_AOX_ARRAY_TYPE_1x4_ULA)
#define AOA_NUM_SNAPSHOTS       (18)
#define AOA_NUM_ARRAY_ELEMENTS  (1 * 4)
#define AOA_REF_PERIOD_SAMPLES  (7)
#define SWITCHING_PATTERN       { 0, 1, 2, 3 }
#endif

#endif // APP_CONFIG_H
