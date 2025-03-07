# LD1115H Sensor Configuration Guide

## Overview
The **HLK-LD1115H-24G** is a 24GHz millimeter-wave radar sensor for human presence detection.  
This guide explains how to retrieve and interpret the current sensor configuration using the **ESP32**.

## Retrieving Configuration
To fetch the current configuration, send the `get_all` command to the sensor via UART.  
The expected response contains key sensor parameters.

### **Example Output**

15:08:51.810 -> ===============================
15:08:51.810 -> LD1115H Sensor Configuration
15:08:51.810 -> Listening on RX: 4, TX: 5
15:08:51.810 -> Baud Rate: 115200
15:08:51.810 -> Requesting configuration...
15:08:51.810 -> ===============================
15:08:51.843 -> [Sent] get_all
15:08:51.843 -> 
15:08:51.843 -> [Response] th1 is 130
15:08:51.843 -> [Response] th2 is 250
15:08:51.843 -> [Response] th_in is 300
15:08:51.843 -> [Response] output_mode is 0
15:08:51.843 -> [Response] tons is 30
15:08:51.843 -> [Response] utons is 100
15:08:51.883 -> [Response] mov, 1 4868
15:08:51.966 -> [Response] mov, 1 5311

## Parameter Explanation

| **Parameter**   | **Value** | **Description** |
|---------------|---------|----------------|
| `th1` | **130** | **Motion Detection Sensitivity** - Higher values make the sensor **less** sensitive to motion. Default is `120`. |
| `th2` | **250** | **Presence Detection Sensitivity** - Affects detection of static humans. |
| `th_in` | **300** | **Unknown** - Possibly an internal threshold for signal processing. |
| `output_mode` | **0** | **Output Mode** - `0` means standard ASCII text output. Other modes may be available. |
| `tons` | **30** | **Output Hold Time** - Keeps the GPIO signal **active for 30 units** (likely milliseconds or seconds). |
| `utons` | **100** | **Output Release Time** - Defines how long the signal remains after detection before resetting. |

## Adjusting Sensor Parameters
To change a setting, use the following command format:

`set <parameter>=<value>`

For example, to increase motion sensitivity:

`set th1=100`

### **Saving Changes**
Changes must be **saved permanently** using:

`save`

If `save` is not executed, settings will reset after a power cycle.

## Next Steps
- If motion detection is **too sensitive**, increase `th1`.
- If detection **lags too long**, reduce `tons` and `utons`.

---

## Troubleshooting
- If no response is received after `get_all`, check:
  - **Baud rate (115200)**
  - **Correct TX/RX wiring**
  - **Sensor power supply (3.6V-5V)**
  - **Wait 15 seconds after startup** before sending commands.

---
### **Author**
This guide was generated based on live testing with an **ESP32** and the **LD1115H sensor**.


