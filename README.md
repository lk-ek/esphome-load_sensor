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
1. Monitoring FreeRTOS task statistics (ESP32) or main loop time (ESP8266)
2. Calculating the ratio of active vs idle CPU time (ESP32) or relative loop delay (ESP8266)
3. Maintaining moving averages over different time periods
4. Publishing values as percentages (0-100%)

## Platform Support

### ESP32

Uses FreeRTOS task statistics for accurate CPU load measurement.

**Configuration:**

```yaml
external_components:
  - source: https://github.com/lk-ek/esphome-load_sensor
    components: [ load_sensor ]

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

**Required build flags for FreeRTOS stats:**

```yaml
esphome:
  platformio_options:
    build_flags:
      - -DconfigGENERATE_RUN_TIME_STATS=1
      - -DconfigUSE_STATS_FORMATTING_FUNCTIONS=1
      - -DconfigUSE_TRACE_FACILITY=1
```

**For Arduino framework, add:**

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

```yaml
external_components:
  - source: https://github.com/lk-ek/esphome-load_sensor
    components: [ load_sensor ]

debug:
  update_interval: 5s

sensor:
  - platform: debug
    loop_time:
      name: "Loop Time"
  - platform: load_sensor
    update_interval: 10s
    baseline_loop_time: 15    # (Optional) ms, see below
    max_loop_time: 350        # (Optional) ms, see below
    load_1m:
      name: "ESP8266 Load Average (1m)"
    load_5m:
      name: "ESP8266 Load Average (5m)"
    load_15m:
      name: "ESP8266 Load Average (15m)"
```

No special build flags are required for ESP8266.

**Tuning `baseline_loop_time` and `max_loop_time`:**

- `baseline_loop_time`: The loop time (in ms) when your device is idle.  
  To measure:  
  1. Flash your device with only the debug and load_sensor platforms enabled.
  2. Observe the "Loop Time" sensor in the logs or Home Assistant.
  3. Use the lowest stable value as your baseline (e.g., 15 ms).

- `max_loop_time`: The loop time (in ms) that should represent 100% load.  
  To measure:  
  1. Add blocking code or heavy processing to simulate a busy loop.
  2. Observe the highest loop time value in the logs.
  3. Set this as your max (e.g., 350 ms).

**Example:**
```yaml
sensor:
  - platform: load_sensor
    update_interval: 10s
    baseline_loop_time: 15
    max_loop_time: 350
    load_1m:
      name: "ESP8266 Load Average (1m)"
    load_5m:
      name: "ESP8266 Load Average (5m)"
    load_15m:
      name: "ESP8266 Load Average (15m)"
```

**Notes:**
- The ESP8266 load is a relative indicator based on how much the main loop is delayed compared to an idle baseline.
- For best results, calibrate the baseline and max loop time values for your device.

## Example Output

The sensor provides three values similar to Linux load averages:
```
ESP32 Load Average (1m): 4.5%
ESP32 Load Average (5m): 3.2%
ESP32 Load Average (15m): 2.8%
```

These values represent the percentage of CPU time spent on non-idle tasks over different time periods.
