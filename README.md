<details>
<summary><font size=5>Table of Contents</font> </summary>

- [1. Introduction](#1-introduction)
- [2. Prerequisites](#2-prerequisites)
- [3. Demos and Examples](#3-demos-and-examples)
  - [3.1. Sample Applications](#31-sample-applications)
  - [3.2. Demo Setup](#32-demo-setup)
    - [3.2.1. NCP mode AoD asset](#321-ncp-mode-aod-asset)
    - [3.2.2. SoC mode AoD asset](#322-soc-mode-aod-asset)
- [4. Antenna Array Accuracy](#4-antenna-array-accuracy)
  - [4.1. Environment 1 (3 tag, 4 locators, static measurement)](#41-environment-1-3-tag-4-locators-static-measurement)
    - [4.1.1. Real position of the locators and tags.](#411-real-position-of-the-locators-and-tags)
    - [4.1.2. Estimated position by RTL lib](#412-estimated-position-by-rtl-lib)
  - [4.2. Environment 2 (9 tag, 4 locators, static measurement)](#42-environment-2-9-tag-4-locators-static-measurement)
    - [4.2.1. Real position of the locators and tags.](#421-real-position-of-the-locators-and-tags)
    - [4.2.2. Estimated position by RTL lib](#422-estimated-position-by-rtl-lib)
  - [4.3. Environment 3 (3 tag, 4 locators, static measurement)](#43-environment-3-3-tag-4-locators-static-measurement)
    - [4.3.1. Real position of the locators and tags.](#431-real-position-of-the-locators-and-tags)
    - [4.3.2. Estimated position by RTL lib](#432-estimated-position-by-rtl-lib)
  - [4.4. Environment 4 (9 tag, 4 locators, static measurement)](#44-environment-4-9-tag-4-locators-static-measurement)
    - [4.4.1. Real position of the locators and tags.](#441-real-position-of-the-locators-and-tags)
    - [4.4.2. Estimated position by RTL lib](#442-estimated-position-by-rtl-lib)
  - [4.5. Environment 5 (10 tag, 5 locators, static measurement)](#45-environment-5-10-tag-5-locators-static-measurement)
    - [4.5.1. Real position of the locators and tags.](#451-real-position-of-the-locators-and-tags)
    - [4.5.2. Estimated position by RTL lib](#452-estimated-position-by-rtl-lib)
  - [4.6. Environment 5 (10 tag, 5 locators, static measurement)](#46-environment-5-10-tag-5-locators-static-measurement)
    - [4.6.1. Real position of the locators and tags.](#461-real-position-of-the-locators-and-tags)
    - [4.6.2. Estimated position by RTL lib](#462-estimated-position-by-rtl-lib)
- [5. Development and Code Walk Through](#5-development-and-code-walk-through)
  - [5.1. Create the AoD Beacon project base on soc-empty example](#51-create-the-aod-beacon-project-base-on-soc-empty-example)
  - [5.2. Create NCP mode AoD asset base on ncp example](#52-create-ncp-mode-aod-asset-base-on-ncp-example)
  - [5.3. Build the AoA_Locator project](#53-build-the-aoa_locator-project)
  - [5.4. Build the AoD_Locator project](#54-build-the-aod_locator-project)
  - [5.5. Create the AoD SoC mode Asset project base on soc-empty example](#55-create-the-aod-soc-mode-asset-project-base-on-soc-empty-example)
- [Conclusion](#conclusion)

</details>

***

# 1. Introduction
Bluetooth Angle of Arrival (AoA) and Angle of Departure (AoD) are new technologies that establish a standardized framework for indoor positioning. With these technologies, the fundamental problem of positioning comes down to solving the arrival and departure angles of radio frequency signals.   
SiliconLabs has the [UG103.18: Bluetooth® Direction Finding
Fundamentals](https://www.silabs.com/documents/public/user-guides/ug103-18-bluetooth-direction-finding-fundamentals.pdf) to explain the basic of the AoA and AoD technology, and 
has [AN1296: Application Development with Silicon Labs’ RTL Library] and [AN1297: Custom Direction-Finding Solutions using the Silicon Labs Bluetooth Stack] to explain how develop the direction finding (DF) applications using the Silicon Labs Bluetooth LE stack and the Real Time Locating Library (RTL lib).   
Please note that all of these three documentations are the fundament to understand the basic of Bluetooth Direction Finding as well as the Silicon Labs solution for it. It's supposed that you've read them.   
This project will show you how to build a AoD application with Silicon Labs Bluetooth LE stack and the RTL Lib. This documentation walks through the steps to help anyone get stared with AoD solution.   

# 2. Prerequisites
Running the demo requires the following devices:   
* Thunderboard BG22 (SLTB010A)
* BRD4185A antenna array board with a WSTK x 3
* A PC running Simplicity Studio 5 with Gecko SDK Suite v3.2 or later
* A Raspberry Pi

# 3. Demos and Examples
## 3.1. Sample Applications

* **soc_aod_beacon** should be built in Studio, and flashed to the Antenna array board. This will act as a beacon (CTE transmitter)   
* **ncp_aod_asset** should be built in Studio and flashed to a Thunderboard BG22. This will act as the asset, that receives the CTE and wants to determine its position.   
* **soc_aod_asset** should be built in Studio and flashed to Tunderboard BG22, it is the SoC mode aod asset that do the I/Q sample and transfer it to the gateway via bluetooth connection. Due to the limited resource, it cannot do any angle estimation or position calculation locally.
* **aod_locator** is the host sample app running on the host demonstrates the CTE Receiver feature and the usage of the angle estimation feature of the RTL library.   
* **aod_gateway** is similar as **aod_locator** but it collects the I/Q sample from the SoC mode aod asset tag via bluetooth connection. And calculate the angle and position accordingly.

## 3.2. Demo Setup
There are two kind of system structure for the multi-transmitter AoD demo. Customer can choose anyone of them depends on their own system design.   
In the first case, the AoD tag asset will work in NCP mode for I/Q sample, the host application **aod_locator** runs on the host MCU or PC will receive the I/Q sample result from the tag via BGAPI interface, and calculate the angle and position according to the I/Q data.   
In the second case, the AoD tag asset work in SoC mode for I/Q sample, after finishing the sampling it will transmit the I/Q sample data to the aod gateway via Bluetooth connection. After receiving the I/Q sample from each AoD tag, the gateway runs **aod_gateway** application will calculate the angle and position.   

For getting start with these two kind of multi-transmitter AoD demos, please follow the steps below.    

### 3.2.1. NCP mode AoD asset
* Please flash a bootloader to each of your boards     
* Please build and flash the **soc_aod_beacon.sls** project to your antenna array boards. You can also find the prebuilt image now for your convenience.     
* Please build and flash the **ncp_aod_asset.sls** project to your Thunderboard. You can also find the prebuilt image now for your convenience.    
* Please copy the attached **aod_locator** project into the following folder:   
    C:\SiliconLabs\SimplicityStudio\v5\developer\sdks\gecko_sdk_suite\v3.x\app\bluetooth\example_host   
* Start MSYS2 MinGW 64-bit, navigate to the **aod_locator** folder, and build the project simply by running ```make ANGLE=1``` (there will be some warnings, neglect them)   
* Navigate to the config folder, and change the multilocator_config.json file according to your setup (change the addresses, position and orientation of your antenna array boards)   
* Navigate to the exe folder, and start the project like this (change the COM port to the one used by the Thunderboard BG22):   
    ./aod_locator.exe -u COM49 -c ../config/multilocator_config.json   
Below is the system block diagram for the case of AoD tag asset works in NCP mode.   
<div align="center">
  <img src="image/aod_ncp_mode_block_diagram.png">  
</div>  
<div align="center">
  <b>Figure 1-1 AoD NCP mode block diagram</b>
</div>  
</br>

### 3.2.2. SoC mode AoD asset
* Build the **soc_aod_beacon** project and flash the image to the antenna array boards. Please make sure that a bootloader is also flashed before.   
* Build the **soc_aod_asset** project and flash the image to the Thunderboard BG22.   
* Flash **NCP - Empty Demo** to a BG22 radio board which acts as the gateway combine with the Raspberry Pi or your PC runs the **aod_gateway** host application.   
* Copy the **aod_gateway** project to the folder ```C:\SiliconLabs\SimplicityStudio\v5\developer\sdks\gecko_sdk_suite\v3.x\app\bluetooth\example_host```   
* Open MSYS2 MinGW 64 bit, browse to the **aod_gateway** folder and run ```make ANGLE=1```   
* Make sure mqtt broker is running   
* start the **aod_gateway** app like this:   
    ```./exe/aod_gateway.exe -c xxx.jason -u COM8```   
    Where the COM port should be the COM port of the BG22 board programmed with NCP empty demo image.   
* start mqtt explorer, and check if you can see the angles calculated for each locator   
<div align="center">
  <img src="image/mqtt_explorer.png">  
</div>   
</br>

* Execute the python script **aod_gui**, it will subscribe the MQTT topic for receiving the position and angle data of each tag, and show the tag in GUI.   
  ```python3 app.py -c ../aod_gateway/config/multilocator_config.json```
    Note: The multilocator configuration file should be exactly the same for aod_gateway program.   

Below is the system block diagram for the case of AoD tag asset works in SoC mode.   

<div align="center">
  <img src="image/aod_soc_mode_block_diagram.png">  
</div>  
<div align="center">
  <b>Figure 1-2 AoD SoC mode block diagram</b>
</div>  
</br>

<div align="center">
  <img src="image/aod_soc_mode_block_diagram2.png">  
</div>  
<div align="center">
  <b>Figure 1-3 AoD SoC mode block diagram</b>
</div>  
</br>

# 4. Antenna Array Accuracy
Direction finding accuracy depends strongly on the amount of different phase information in the receiver. The number of channels/antennas affects the overall angle accuracy. However, increasing the number of antennas requires more and more memory for the calculations and increases PCB size as well.   
The 4x4 array was chosen as the reference based on the most optimal system performance for smallest array size.   
We has performed real environment testing on the 4x4 array with the following results. The measurements were performed using Gecko SDK 3.2 and RTL library API: SL_RTL_AOX_MODE_REAL_TIME_BASIC.

The following devices were used for all antenna array accuracy measurements in this section:   
• Locator: BRD4185A Rev A01   
• Tag: BRD4184A Rev A02   

## 4.1. Environment 1 (3 tag, 4 locators, static measurement)
* Location: Office environment, open space    
* Locator height from floor: 0.0 m    
* Tag height from floor: 0.75 m   
* Testing range: 3.8x3.9m2   

There are total 4 locators and 3 tags in the 3.8x3.9 area, below are the measured position and angle accuracy for the tags.   

### 4.1.1. Real position of the locators and tags.
|Locator ID | X | Y | Z |
|-|-|-|-|
|#1 ble-pd-588E81A54222 | 3.80 | 3.90 | 0.00 |
|#2 ble-pd-588E8166AF43 | 0.00 | 0.00 | 0.00 |
|#3 ble-pd-84FD27EEE4FF | 3.80 | 0.00 | 0.00 |
|#4 ble-pd-588E81A5421C | 0.00 | 3.90 | 0.00 |
<div align="center">
  <img src="image/3.8x3.9_4locators.png">  
</div>  
</br>  

|Tag ID | X | Y | Z |
|-|-|-|-|
|ble-pd-60A423C96825 | 2.00 | 2.90 | 0.75 |
|ble-pd-60A423C96B3C | 2.00 | 2.10 | 0.75 |
|ble-pd-60A423C96746 | 2.00 | 1.30 | 0.75 |

### 4.1.2. Estimated position by RTL lib 
Below is the estimated position of these three tags. We collected all of the position and angle data for each tag, the curve below reflects the X/Y/Z axis value for each tag in 5mins.   
<div align="center">
  <img src="image/3.8x3.9_4locators_3tags_all.png">  
</div> 
</br>  

## 4.2. Environment 2 (9 tag, 4 locators, static measurement)
* Location: Office environment, open space    
* Locator height from floor: 0.0 m   
* Tag height from floor: 0.75 m   
* Testing range: 3.8x3.9m2   

### 4.2.1. Real position of the locators and tags.
|Locator ID | X | Y | Z |
|-|-|-|-|
|#1 ble-pd-588E81A54222 | 3.80 | 3.90 | 0.00 |
|#2 ble-pd-588E8166AF43 | 0.00 | 0.00 | 0.00 |
|#3 ble-pd-84FD27EEE4FF | 3.80 | 0.00 | 0.00 |
|#4 ble-pd-588E81A5421C | 0.00 | 3.90 | 0.00 |
<div align="center">
  <img src="image/3.8x3.9_4locators.png">  
</div>  
</br>  


|Tag ID | X | Y | Z |
|-|-|-|-|
|ble-pd-60A423C96B13 | 2.00 | 3.20 | 0.75 |
|ble-pd-60A423C96896 | 2.00 | 2.90 | 0.75 |
|ble-pd-60A423C96AB5 | 2.00 | 2.60 | 0.75 |
|ble-pd-60A423C96721 | 2.00 | 2.30 | 0.75 |
|ble-pd-60A423C96B3C | 2.00 | 2.00 | 0.75 |
|ble-pd-60A423C96825 | 2.00 | 1.70 | 0.75 |
|ble-pd-60A423C968C6 | 2.00 | 1.40 | 0.75 |
|ble-pd-60A423C96FC6 | 2.00 | 1.10 | 0.75 |
|ble-pd-60A423C96746 | 2.00 | 0.80 | 0.75 |

### 4.2.2. Estimated position by RTL lib 
Below is the estimated position shown in GUI, and we can monitor the value with the MQTT Explorer.   
<div align="center">
  <img src="image/3.8x3.9_9tags.png">  
</div>  
</br>  

<div align="center">
  <img src="image/3.8x3.9_9tags_position.png">  
</div>  
</br>  

Also we collected all of the position and angle data for each tag, the curve below reflects the X/Y/Z axis value for each tag in 5mins.   

<div align="center">
  <img src="image/3.8x3.9_4locators_9tags_all.png">  
</div> 
</br>  

## 4.3. Environment 3 (3 tag, 4 locators, static measurement)
* Location: Office environment, open space    
* Locator height from floor: 0.0 m   
* Tag height from floor: 0.75 m   
* Testing range: 2x9m2   

### 4.3.1. Real position of the locators and tags.
|Locator ID | X | Y | Z | Orientation
|-|-|-|-|-|
|#1 ble-pd-588E81A54222 | 2.00 | 9.00 | 0.00 | (0, 0, 0) |
|#2 ble-pd-588E8166AF43 | 0.00 | 3.00 | 0.00 | (0, 0, 0) |
|#3 ble-pd-84FD27EEE4FF | 2.00 | 0.00 | 0.00 | (0, 0, 0) |
|#4 ble-pd-588E81A5421C | 0.00 | 6.00 | 0.00 | (0, 0, 0) |
<div align="center">
  <img src="image/2x9_4locators.png">  
</div>  
</br>  

|Tag ID | X | Y | Z |
|-|-|-|-|
|ble-pd-60A423C96825 | 1.40 | 5.30 | 0.75 |
|ble-pd-60A423C96746 | 1.40 | 4.50 | 0.75 |
|ble-pd-60A423C96B3C | 1.40 | 3.70 | 0.75 |

### 4.3.2. Estimated position by RTL lib 
Below is the estimated position of these three tags. We collected all of the position and angle data for each tag, the curve below reflects the X/Y/Z axis value for each tag in 5mins.   
**ble-pd-60A423C96825**
<div align="center">
  <img src="image/2x9_4locators_3tags_ble-pd-60A423C96825.png">  
</div> 

**ble-pd-60A423C96746**
<div align="center">
  <img src="image/2x9_4locators_3tags_ble-pd-60A423C96746.png">  
</div> 

**ble-pd-60A423C96B3C**
<div align="center">
  <img src="image/2x9_4locators_3tags_ble-pd-60A423C96B3C.png">  
</div> 
</br>  

## 4.4. Environment 4 (9 tag, 4 locators, static measurement)
* Location: Office environment, open space    
* Locator height from floor: 0.0 m   
* Tag height from floor: 0.75 m   
* Testing range: 2x9m2   

### 4.4.1. Real position of the locators and tags.
|Locator ID | X | Y | Z | Orientation
|-|-|-|-|-|
|#1 ble-pd-588E81A54222 | 2.00 | 9.00 | 0.00 | (0, 0, 0) |
|#2 ble-pd-588E8166AF43 | 0.00 | 3.00 | 0.00 | (0, 0, 90) |
|#3 ble-pd-84FD27EEE4FF | 2.00 | 0.00 | 0.00 | (0, 0, 90) |
|#4 ble-pd-588E81A5421C | 0.00 | 6.00 | 0.00 | (0, 0, 0) |
<div align="center">
  <img src="image/2x9_4locators.png">  
</div>  
</br>  

|Tag ID | X | Y | Z |
|-|-|-|-|
|ble-pd-60A423C96825 | 1.40 | 5.30 | 0.75 |
|ble-pd-60A423C96746 | 1.40 | 4.50 | 0.75 |
|ble-pd-60A423C96B3C | 1.40 | 3.70 | 0.75 |

### 4.4.2. Estimated position by RTL lib 
Below is the estimated position of these three tags. We collected all of the position and angle data for each tag, the curve below reflects the X/Y/Z axis value for each tag in 5mins.   

<div align="center">
  <img src="image/2x9_9tags_position.png">  
</div> 
</br>  

## 4.5. Environment 5 (10 tag, 5 locators, static measurement)
* Location: Office environment, open space    
* Locator height from floor: 0.0 m   
* Tag height from floor: 0.75 m   
* Testing range: 2x10m2   

### 4.5.1. Real position of the locators and tags.
|Locator ID | X | Y | Z | Orientation
|-|-|-|-|-|
|#1 ble-pd-588E81A54222 | 2.00 | 10.00 | 0.00 | (0, 0, 90) |
|#2 ble-pd-588E8166AF43 | 0.00 | 2.50 | 0.00 | (0, 0, 180) |
|#3 ble-pd-84FD27EEE4FF | 2.00 | 0.00 | 0.00 | (0, 0, 270) |
|#4 ble-pd-588E81A5421C | 0.00 | 7.50 | 0.00 | (0, 0, 180) |
|#5 ble-pd-588E8166AFD7 | 2.00 | 5.00 | 0.00 | (0, 0, 0) |

<div align="center">
  <img src="image/eFence_layout_2x10_5locators.png">  
</div>  
</br>  

|Tag ID | X | Y | Z |
|-|-|-|-|
|ble-pd-60A423C96896 | 1.40 | 8.60 | 0.75 |
|ble-pd-60A423C9689C | 1.40 | 7.80 | 0.75 |
|ble-pd-60A423C96B13 | 1.40 | 7.00 | 0.75 |
|ble-pd-60A423C96AB5 | 1.40 | 6.20 | 0.75 |
|ble-pd-60A423C96721 | 1.40 | 5.40 | 0.75 |
|ble-pd-60A423C96B3C | 1.40 | 4.60 | 0.75 |
|ble-pd-60A423C96825 | 1.40 | 3.80 | 0.75 |
|ble-pd-60A423C968C6 | 1.40 | 3.00 | 0.75 |
|ble-pd-60A423C96746 | 1.40 | 2.20 | 0.75 |
|ble-pd-60A423C96FC6 | 1.40 | 1.40 | 0.75 |

### 4.5.2. Estimated position by RTL lib 
Below is the estimated position of these ten tags. We collected all of the position and angle data for each tag, the curve for each tag below reflects the X/Y/Z axis value for each tag in 5mins.   

<div align="center">
  <img src="image/2x10_10tags_position_3D.png">  
</div> 
</br>  

<div align="center">
  <img src="image/2x10_10tags_position_data.png">  
</div>
</br>   


## 4.6. Environment 5 (10 tag, 5 locators, static measurement)
* Location: Outdoor environment, sharing bicycle parking area.
* Locator height from floor: 0.0 m   
* Tag height from floor: 0.75 m   
* Testing range: 2x10m2   
* Location estimation mode, Two-dimensional high accuracy mode 

Below is the picture of the sharing bicycle parking area, there are around 30 sharing bicycle in the 2x10m area.
<div align="center">
  <img src="image/parking_area.png">  
</div>  
</br>  

And we attached all of the tags on the locker of the sharing bicycle during the test as below.
<div align="center">
  <img src="image/tag_on_sharing_bicycle.png">  
</div>  
</br>  

### 4.6.1. Real position of the locators and tags.
|Locator ID | X | Y | Z | Orientation
|-|-|-|-|-|
|#1 ble-pd-588E81A54222 | 2.00 | 10.00 | 0.00 | (0, 0, 90) |
|#2 ble-pd-588E8166AF43 | 0.00 | 2.50 | 0.00 | (0, 0, 180) |
|#3 ble-pd-84FD27EEE4FF | 2.00 | 0.00 | 0.00 | (0, 0, 270) |
|#4 ble-pd-588E81A5421C | 0.00 | 7.50 | 0.00 | (0, 0, 180) |
|#5 ble-pd-588E8166AFD7 | 2.00 | 5.00 | 0.00 | (0, 0, 0) |

<div align="center">
  <img src="image/eFence_layout_2x10_5locators.png">  
</div>  
</br>  

|Tag ID | X | Y | Z |
|-|-|-|-|
|ble-pd-60A423C968C6 | 0.70 | 8.60 | 0.75 |
|ble-pd-60A423C96896 | 0.70 | 7.80 | 0.75 |
|ble-pd-60A423C96B13 | 0.70 | 7.00 | 0.75 |
|ble-pd-60A423C96AB5 | 0.70 | 6.20 | 0.75 |
|ble-pd-60A423C96721 | 0.70 | 5.40 | 0.75 |
|ble-pd-60A423C96B3C | 0.70 | 4.60 | 0.75 |
|ble-pd-60A423C96746 | 0.70 | 3.80 | 0.75 |
|ble-pd-60A423C9689C | 0.70 | 3.00 | 0.75 |
|ble-pd-60A423C96825 | 0.70 | 2.20 | 0.75 |
|ble-pd-60A423C96FC6 | 0.70 | 1.40 | 0.75 |

### 4.6.2. Estimated position by RTL lib 
Below is the estimated position of these ten tags. We collected all of the position and angle data for each tag, the curve for each tag below reflects the X/Y/Z axis value for each tag in 5mins.   
<div align="center">
  <img src="image/2x10_10tags_position_2D_outdoor.png">  
</div> 
</br>  

# 5. Development and Code Walk Through
The section below guide you how to implement the AoD projects with SiliconLabs Bluetooth SDK as well as the example projects come with the SDK.   
## 5.1. Create the AoD Beacon project base on soc-empty example
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

## 5.2. Create NCP mode AoD asset base on ncp example
Create a ncp-empty project, and install the components below.   
* RAIL Utility, AoX (Utility to aid with Angle of Arrival/Departure (AoX) Configuration)   
* AoA Receiver (Bluetooth AoA CTE receiving feature)   

For the RAIL Utility, AoX component, please configure the Number of AoX Antenna Pins as 4, and configure the SL_RAIL_UTIL_AOX_ANTENNA_PIN0-SL_RAIL_UTIL_AOX_ANTENNA_PIN3 as PC04-PC07 respectively.   

## 5.3. Build the AoA_Locator project
With the v3.2.0 Bluetooth SDK release, there is **AoA Analyzer** tool which is a Java based application, built into Simplicity Studio 5, that showcases the angle estimation skills of the RTL library in a graphical user interface.   
For how to get started with AoA Analyzer Tool, please see the section "2.1.3 Start the AoA Analyzer Tool", and similar as the previous release, the is a c based host sample app "aoa_locator" which can run on the host demonstrates the CTE Receiver feature and the use of the angle estimation feature of the RTL library. It uses the same application logic as the AoA Analyzer used in the demo setup.    
For how to build the host sample "AoA Locator", please see the section "3.2.1 Building a Single AoA Locator Host Sample Application" of [AN12960](https://www.silabs.com/documents/public/application-notes/an1296-application-development-with-rtl-library.pdf) for step by step instructions.   

Please note that the single AoA locator can be detached from the RTL library and thus does not have to calculate angle values. Instead, it can publish IQ reports directly to the MQTT broker. This can be done using the ANGLE make variable.   
```make APP_MODE=silabs ANGLE=0```

If want to calculate and expose the calculated angle information in the single AoA locator, it can be done with the ANGLE make variable as below.   
```make APP_MODE=silabs ANGLE=1```

在v3.2.0中，AoA_Locator可以支持两种模式，直接publish IQ sample数据，或者publish计算得出的angle数据。

## 5.4. Build the AoD_Locator project
Copy the aod_locator project to the folder below, and build with the command "make APP_MODE=silabs ANGLE=1".

## 5.5. Create the AoD SoC mode Asset project base on soc-empty example
Create a soc-empty project, and install the components below.
* RAIL Utility, AoX (Utility to aid with Angle of Arrival/Departure (AoX) Configuration)
* AoA Receiver (Bluetooth AoA CTE receiving feature)

For the RAIL Utility, AoX component, please configure the Number of AoX Antenna Pins as 4, and configure the SL_RAIL_UTIL_AOX_ANTENNA_PIN0-SL_RAIL_UTIL_AOX_ANTENNA_PIN3 as PC04-PC07 respectively.

For the soc mode AoD tag, it will send the IQ sample data to the gateway device via bluetooth connection, and the gateway is responsible for angle calculation.

# Conclusion
TBD