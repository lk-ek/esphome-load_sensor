import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    UNIT_PERCENT,
    ICON_CHIP,
    DEVICE_CLASS_EMPTY,
    STATE_CLASS_MEASUREMENT,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
from esphome.core import CORE

import platform

CONF_LOAD_1M = "load_1m"
CONF_LOAD_5M = "load_5m"
CONF_LOAD_15M = "load_15m"
CONF_BASELINE_LOOP_TIME = "baseline_loop_time"
CONF_MAX_LOOP_TIME = "max_loop_time"

load_sensor_ns = cg.esphome_ns.namespace("load_sensor")
LoadSensor = load_sensor_ns.class_("LoadSensor", cg.PollingComponent)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(LoadSensor),
    cv.Optional(CONF_LOAD_1M): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        icon=ICON_CHIP,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_EMPTY,
        state_class=STATE_CLASS_MEASUREMENT,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_LOAD_5M): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        icon=ICON_CHIP,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_EMPTY,
        state_class=STATE_CLASS_MEASUREMENT,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_LOAD_15M): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        icon=ICON_CHIP,
        accuracy_decimals=1,
        device_class=DEVICE_CLASS_EMPTY,
        state_class=STATE_CLASS_MEASUREMENT,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
    ),
    cv.Optional(CONF_BASELINE_LOOP_TIME): cv.positive_float,
    cv.Optional(CONF_MAX_LOOP_TIME): cv.positive_float,
}).extend(cv.polling_component_schema("10s"))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    if CONF_LOAD_1M in config:
        sens = await sensor.new_sensor(config[CONF_LOAD_1M])
        cg.add(var.set_load_1m(sens))
    
    if CONF_LOAD_5M in config:
        sens = await sensor.new_sensor(config[CONF_LOAD_5M])
        cg.add(var.set_load_5m(sens))
    
    if CONF_LOAD_15M in config:
        sens = await sensor.new_sensor(config[CONF_LOAD_15M])
        cg.add(var.set_load_15m(sens))

    # --- ESP8266: wire up debug loop_time sensor if present in YAML ---
    if CORE.is_esp8266:
        for s in CORE.config.get("sensor", []):
            if (
                isinstance(s, dict)
                and s.get("platform") == "debug"
                and "loop_time" in s
            ):
                loop_time_cfg = s["loop_time"]
                # Try to get the id if set, otherwise use the default object name
                if isinstance(loop_time_cfg, dict) and "id" in loop_time_cfg:
                    loop_time_id = loop_time_cfg["id"]
                else:
                    loop_time_id = "loop_time"
                # Convert to ID object using cv.use_id
                loop_time_id_obj = cv.use_id(sensor.Sensor)(loop_time_id)
                loop_time_var = await cg.get_variable(loop_time_id_obj)
                cg.add(var.set_loop_time(loop_time_var))
                break

        # Pass baseline/max loop time to C++ if set (ESP8266 only)
        if CONF_BASELINE_LOOP_TIME in config:
            cg.add(var.set_baseline_loop_time(config[CONF_BASELINE_LOOP_TIME]))
        if CONF_MAX_LOOP_TIME in config:
            cg.add(var.set_max_loop_time(config[CONF_MAX_LOOP_TIME]))