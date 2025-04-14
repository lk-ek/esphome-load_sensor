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

CONF_LOAD_1M = "load_1m"
CONF_LOAD_5M = "load_5m"
CONF_LOAD_15M = "load_15m"

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