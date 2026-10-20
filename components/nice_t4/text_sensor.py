import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, CONF_TYPE

from . import CONF_NICE_T4_ID, NiceT4Hub, nice_t4_ns

DEPENDENCIES = ["nice_t4"]

NiceT4TextSensor = nice_t4_ns.class_("NiceT4TextSensor", text_sensor.TextSensor, cg.Component)

TEXT_SENSOR_TYPES = {"status": 0, "last_event": 1}
# Типы из плана, которые появятся вместе с опросом привода при включении (этап P2)
PLANNED_TYPES = ("menu_json", "model_name", "fw_version", "error", "capabilities")


def validate_type(value):
    value = cv.string(value).lower()
    if value in PLANNED_TYPES:
        raise cv.Invalid(f"Тип '{value}' ещё не реализован (этап P2). Доступны: {', '.join(TEXT_SENSOR_TYPES)}")
    return cv.enum(TEXT_SENSOR_TYPES, lower=True)(value)


CONFIG_SCHEMA = text_sensor.text_sensor_schema(NiceT4TextSensor).extend(
    {
        cv.GenerateID(CONF_NICE_T4_ID): cv.use_id(NiceT4Hub),
        cv.Required(CONF_TYPE): validate_type,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_NICE_T4_ID])
    var = cg.new_Pvariable(config[CONF_ID], hub, config[CONF_TYPE])
    await cg.register_component(var, config)
    await text_sensor.register_text_sensor(var, config)
