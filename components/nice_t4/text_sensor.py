import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, CONF_TYPE

DEPENDENCIES = ['nice_t4']

nice_t4_ns = cg.esphome_ns.namespace('nice_t4')
NiceT4TextSensor = nice_t4_ns.class_('NiceT4TextSensor', text_sensor.TextSensor, cg.Component)

# Типы текстовых сенсоров
TEXT_SENSOR_TYPES = {
    "menu_json": 0,
    "model_name": 1,
    "fw_version": 2,
    "status": 3,
    "error": 4,
    "capabilities": 5,
}

CONFIG_SCHEMA = text_sensor.text_sensor_schema(
    NiceT4TextSensor
).extend({
    cv.GenerateID("nice_t4_id"): cv.use_id(nice_t4_ns.class_('NiceT4Hub')),
    cv.Required(CONF_TYPE): cv.enum(TEXT_SENSOR_TYPES, lower=True),
})

async def to_code(config):
    hub = await cg.get_variable(config["nice_t4_id"])
    var = cg.new_Pvariable(config[CONF_ID], hub, config[CONF_TYPE])
    await text_sensor.register_text_sensor(var, config)
    await cg.register_component(var, config)
