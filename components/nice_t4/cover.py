import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover
from esphome.const import CONF_ID, DEVICE_CLASS_GATE

from . import CONF_NICE_T4_ID, NiceT4Hub, nice_t4_ns

DEPENDENCIES = ["nice_t4"]

NiceT4Cover = nice_t4_ns.class_("NiceT4Cover", cover.Cover, cg.Component)

CONFIG_SCHEMA = cover.cover_schema(NiceT4Cover, device_class=DEVICE_CLASS_GATE).extend(
    {cv.GenerateID(CONF_NICE_T4_ID): cv.use_id(NiceT4Hub)}
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_NICE_T4_ID])
    var = cg.new_Pvariable(config[CONF_ID], hub)
    await cg.register_component(var, config)
    await cover.register_cover(var, config)
