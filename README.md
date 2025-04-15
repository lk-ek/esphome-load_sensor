# ESP32/ESP8266 Load Average Sensor for ESPHome

This component provides Linux-style CPU load averages for ESP32 devices running ESPHome. It monitors FreeRTOS task statistics to calculate 1-minute, 5-minute, and 15-minute load averages.

## Installation

Add this as an external component in your ESPHome configuration:

```yaml
external_components:
  - source: https://github.com/lk-ek/esphome-load_sensor
    components: [ load_sensor ]
```

## Usage

```yaml
sensor:
  - platform: load_sensor
    update_interval: 10s
    load_1m:
      name: "ESP32 Load Average (1m)"
    load_5m:
      name: "ESP32 Load Average (5m)"
    load_15m:
      name: "ESP32 Load Average (15m)"
```

## Configuration Variables

- **update_interval** (*Optional*, Time): The interval between measurements. Defaults to 10s.
- **load_1m** (*Optional*, Sensor): One-minute load average sensor
- **load_5m** (*Optional*, Sensor): Five-minute load average sensor
- **load_15m** (*Optional*, Sensor): Fifteen-minute load average sensor

Each sensor supports all standard [sensor filtering options](https://esphome.io/components/sensor/index.html#sensor-filters).

## How it Works

The load sensor works by:
1. Monitoring FreeRTOS task statistics
2. Calculating the ratio of active vs idle CPU time
3. Maintaining moving averages over different time periods
4. Publishing values as percentages (0-100%)

## Requirements

- ESP32 device running ESP-IDF framework
- ESPHome with following build flags:
  ```yaml
  esphome:
    platformio_options:
      build_flags:
        - -DconfigGENERATE_RUN_TIME_STATS=1
        - -DconfigUSE_STATS_FORMATTING_FUNCTIONS=1
        - -DconfigUSE_TRACE_FACILITY=1
  ```

## Framework Support

### ESP-IDF Framework
No additional configuration needed beyond the build flags listed above.

### Arduino Framework
When using Arduino framework, add these build flags:
```yaml
esphome:
  platformio_options:
    build_flags:
      - -DconfigUSE_TRACE_FACILITY=1
      - -DconfigGENERATE_RUN_TIME_STATS=1
      - -DINCLUDE_uxTaskGetStackHighWaterMark=1
```

## Platform Support

### ESP32

Uses FreeRTOS task statistics for accurate CPU load measurement.

**Configuration:**

1. **Add the component as an external component:**
    ```yaml
    external_components:
      - source: https://github.com/lk-ek/esphome-load_sensor
        components: [ load_sensor ]
    ```

2. **Add the load sensor to your sensors:**
    ```yaml
    sensor:
      - platform: load_sensor
        update_interval: 10s
        load_1m:
          name: "ESP32 Load Average (1m)"
        load_5m:
          name: "ESP32 Load Average (5m)"
        load_15m:
          name: "ESP32 Load Average (15m)"
    ```

3. **Add required build flags for FreeRTOS stats:**
    ```yaml
    esphome:
      platformio_options:
        build_flags:
          - -DconfigGENERATE_RUN_TIME_STATS=1
          - -DconfigUSE_STATS_FORMATTING_FUNCTIONS=1
          - -DconfigUSE_TRACE_FACILITY=1
    ```

4. **For Arduino framework, add:**
    ```yaml
    esphome:
      platformio_options:
        build_flags:
          - -DconfigUSE_TRACE_FACILITY=1
          - -DconfigGENERATE_RUN_TIME_STATS=1
          - -DINCLUDE_uxTaskGetStackHighWaterMark=1
    ```

### ESP8266

Uses the main loop execution time as a proxy for system load.

**Configuration:**

1. **Add the component as an external component:**
    ```yaml
    external_components:
      - source: https://github.com/lk-ek/esphome-load_sensor
        components: [ load_sensor ]
    ```

2. **Add the debug and load_sensor platforms to your YAML:**
    ```yaml
    debug:
      update_interval: 5s

    sensor:
      - platform: debug
        loop_time:
          name: "Loop Time"
      - platform: load_sensor
        update_interval: 10s
        load_1m:
          name: "ESP8266 Load Average (1m)"
        load_5m:
          name: "ESP8266 Load Average (5m)"
        load_15m:
          name: "ESP8266 Load Average (15m)"
    ```

3. **No special build flags are required for ESP8266.**

**Notes:**
- The ESP8266 load is a relative indicator based on how much the main loop is delayed compared to an idle baseline.
- For best results, calibrate the baseline and max loop time values in the C++ code for your device.

## Example Output

The sensor provides three values similar to Linux load averages:
```
ESP32 Load Average (1m): 4.5%
ESP32 Load Average (5m): 3.2%
ESP32 Load Average (15m): 2.8%
```

These values represent the percentage of CPU time spent on non-idle tasks over different time periods.
