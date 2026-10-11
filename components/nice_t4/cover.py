import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover
from esphome.const import CONF_ID

DEPENDENCIES = ['nice_t4']

nice_t4_ns = cg.esphome_ns.namespace('nice_t4')
NiceT4Cover = nice_t4_ns.class_('NiceT4Cover', cover.Cover, cg.Component)

CONFIG_SCHEMA = cover.COVER_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(NiceT4Cover),
    cv.GenerateID("nice_t4_id"): cv.use_id(nice_t4_ns.class_('NiceT4Hub')),
})

async def to_code(config):
    hub = await cg.get_variable(config["nice_t4_id"])
    var = cg.new_Pvariable(config[CONF_ID], hub)
    await cg.register_component(var, config)
    await cover.register_cover(var, config)
