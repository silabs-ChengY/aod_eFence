/***************************************************************************//**
 * @file
 * @brief main() function.
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

#include <stdbool.h>
#include <stdlib.h>
#include "system.h"
#include "app_signal.h"
#include "app.h"

// Main loop execution status.
static volatile bool run = true;

// Custom signal handler.
static void signal_handler(int sig)
{
  (void)sig;
  run = false;
}

int main(int argc, char* argv[])
{
  // Set up custom signal handler for user interrupt and termination request.
  app_signal(SIGINT, signal_handler);
  app_signal(SIGTERM, signal_handler);

  // Initialize Silicon Labs device, system, service(s) and protocol stack(s).
  // Note that if the kernel is present, processing task(s) will be created by
  // this call.
  sl_system_init();

  // Initialize the application. For example, create periodic timer(s) or
  // task(s) if the kernel is present.
  app_init(argc, argv);

  while (run) {
    // Do not remove this call: Silicon Labs components process action routine
    // must be called from the super loop.
    sl_system_process_action();

    // Application process.
    app_process_action();
  }

  // Deinitialize the application.
  app_deinit();

  return EXIT_SUCCESS;
}
