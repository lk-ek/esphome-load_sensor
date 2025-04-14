import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_NAME,
    UNIT_PERCENT,
    ICON_CHIP,
    DEVICE_CLASS_EMPTY,
    STATE_CLASS_MEASUREMENT,
    ENTITY_CATEGORY_DIAGNOSTIC,
)

load_sensor_ns = cg.esphome_ns.namespace("load_sensor")
LoadSensor = load_sensor_ns.class_("LoadSensor", sensor.Sensor, cg.PollingComponent)

CONFIG_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_PERCENT,
    icon=ICON_CHIP,
    accuracy_decimals=1,
    device_class=DEVICE_CLASS_EMPTY,
    state_class=STATE_CLASS_MEASUREMENT,
    entity_category=ENTITY_CATEGORY_DIAGNOSTIC
).extend({
    cv.GenerateID(): cv.declare_id(LoadSensor),
    cv.Optional("update_interval", default="10s"): cv.update_interval,
})

async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    cg.add(var.set_update_interval(config["update_interval"]))