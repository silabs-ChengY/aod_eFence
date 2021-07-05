# AoD eFence

***

# Introduction
This is a project demonstrates how to use the AoD for sharing bicycle parking management

# Demos and Examples
## Sample Applications

**soc_aod_beacon** should be built in Studio, and flashed to the Antenna array board. This will act as a beacon (CTE transmitter)   
**ncp_aod_asset** should be built in Studio and flashed to a Thunderboard BG22. This will act as the asset, that receives the CTE and wants to determine its position.   
**aod_compass** should be built outside of studio using MSYS2 mingw 64-bit, the same way as the aoa_compass is built, see documentation in QSG175. The main difference between aoa_compass and aod_compass is, that aod_compass has to connect to the Thunderboard instead of the antenna array board. This time, the angles/position is calculated on the Thunderboard side. So now the Thunderboard acts like an ncp target, and it connects to the PC, which then does the calculation.   
**aod_locator** is the host sample app running on the host demonstrates the CTE Receiver feature and the usage of the angle estimation feature of the RTL library.   

## Demo
For getting start with the multi-transmitter AoD demo, please follow the steps below.   

### NCP mode AoD asset
* Please flash a bootloader to each of your boards   
* Please build and flash the soc_aod_beacon.sls project to your antenna array boards. You can also find the prebuilt image now for your convenience.   
* Please build and flash the ncp_aod_asset.sls project to your Thunderboard. You can also find the prebuilt image now for your convenience.   
* Please copy the attached aod_locator project into the following folder:   
    C:\SiliconLabs\SimplicityStudio\v5\developer\sdks\gecko_sdk_suite\v3.1\app\bluetooth\example_host   
* Start MSYS2 MinGW 64-bit, browse to the aod_locator folder, and build the project simply by running make (there will be some warnings, neglect them)   
* Navigate to the config folder, and change the multilocator_config.json file according to your setup (change the addresses, position and orientation of your antenna array boards)   
* Navigate to the exe folder, and start the project like this (change the COM port to the one used by the Thunderboard):   
    ./aod_locator.exe -u COM49 -c ../config/multilocator_config.json   

### SoC mode AoD asset
* flash soc_aod_beacon to some antenna array boards. Please make sure, that a bootloader is also flashed, e.g. by flashing the NCP AoA Locator Demo to the board first   
* flash soc_aod_asset to a Thunderboard. Please make sure, that a bootloader is also flashed, e.g. by flashing a SoC Thermometer Demo to * the Thunderboard first.   
* flash NCP - Empty Demo to a BG22 radio board   
* copy the aod_gateway project to C:\SiliconLabs\SimplicityStudio\v5\developer\sdks\gecko_sdk_suite\v3.1\app\bluetooth\example_host   
* open MSYS2 MinGW 64 bit, browse to the aod_gateway folder and run 'make'   
* make sure mqtt broker is running   
* start the aod_gateway app like this:   
    ./exe/aod_gateway.exe -u COM8   
    where the COM port should be the COM port of the BG22 board programmed with NCP empty   
* start mqtt explorer, and check if you can see the angles calculated for each locator   
* start the aoa_multilocator and aoa_multilocator_gui apps as described in AN1296   
    Note: the multilocator_configuration should be exactly the same for AoD as it is for AoA   

Below is the block diagram of the SoC mode AoD demo.   

<div align="center">
  <img src="image/aod_soc_mode_block_diagram.png">  
</div>  
<div align="center">
  <b>Figure 1-1 AoD SoC mode block diagram</b>
</div>  
</br>


<div align="center">
  <img src="image/aod_soc_mode_block_diagram2.png">  
</div>  
<div align="center">
  <b>Figure 1-2 AoD SoC mode block diagram</b>
</div>  
</br>


# Development
## Create the AoD Beacon project base on soc-empty example
Create a soc-empty project, and then install the components below. 
* RAIL Utility, AoX (Utility to aid with Angle of Arrival/Departure (AoX) Configuration)
* AoA Transmitter (Bluetooth AoA CTE transmission feature)

And then set advertising data and start extended advertising, the CTE will be appended on the extended advertising package. Below is the code snippet.

```c
uint8_t data[3] = {0x02, 0x01, 0x06};
sl_bt_advertiser_set_data(advertising_set_handle, 0, sizeof(data), data);
app_assert_status(sc);

/* Turn off legacy PDU flag. */
sl_bt_advertiser_clear_configuration(advertising_set_handle,1);
app_assert_status(sc);

// Start general advertising and enable connections.
sc = sl_bt_advertiser_start(
  advertising_set_handle,
  advertiser_user_data,
  advertiser_non_connectable);
app_assert_status(sc);

sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);

/* Add CTE to extended advertisements in AoD mode */
sl_bt_cte_transmitter_enable_silabs_cte(advertising_set_handle, CTE_LENGTH, CTE_TYPE, CTE_COUNT, sizeof(antenna_array), antenna_array);
```

The CTE length and slot size is defined as below.
```c
#define CTE_LENGTH  20  // 160ms
#define CTE_TYPE     1  // AoD with 1us slots
#define CTE_COUNT    1  // 1 per interval
```

## Create NCP mode AoD asset base on ncp example
Create a ncp-empty project, and install the components below.
* RAIL Utility, AoX (Utility to aid with Angle of Arrival/Departure (AoX) Configuration)
* AoA Receiver (Bluetooth AoA CTE receiving feature)

For the RAIL Utility, AoX component, please configure the Number of AoX Antenna Pins as 4, and the SL_RAIL_UTIL_AOX_ANTENNA_PIN0-SL_RAIL_UTIL_AOX_ANTENNA_PIN3 as PC04-PC07 respectively.

## Build the AoA_Locator project
With the v3.2.0 Bluetooth SDK release, there is **AoA Analyzer** tool which is a Java based application, built into Simplicity Studio 5, that showcases the angle estimation skills of the RTL library in a graphical user interface.
For how to get started with AoA Analyzer Tool, please see the section "2.1.3 Start the AoA Analyzer Tool", and similar as the previous release, the is a c based host sample app "aoa_locator" which can run on the host demonstrates the CTE Receiver feature and the use of the angle estimation feature of the RTL library. It uses the same application logic as the AoA Analyzer used in the demo setup.   
For how to build the host sample "AoA Locator", please see the section "3.2.1 Building a Single AoA Locator Host Sample Application" of [AN12960](https://www.silabs.com/documents/public/application-notes/an1296-application-development-with-rtl-library.pdf) for step by step instructions.   

Please note that the single AoA locator can be detached from the RTL library and thus does not have to calculate angle values. Instead, it can publish IQ reports directly to the MQTT broker. This can be done using the ANGLE make variable.   
```make APP_MODE=silabs ANGLE=0```

If want to calculate and expose the calculated angle information in the single AoA locator, it can be done with the ANGLE make variable as below.   
```make APP_MODE=silabs ANGLE=1```

在v3.2.0中，AoA_Locator可以支持两种模式，直接publish IQ sample数据，或者publish计算得出的angle数据。

## Build the AoD_Locator project
