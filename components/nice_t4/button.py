import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID

DEPENDENCIES = ['nice_t4']

nice_t4_ns = cg.esphome_ns.namespace('nice_t4')
NiceT4Button = nice_t4_ns.class_('NiceT4Button', button.Button, cg.Component)

CONF_COMMAND = "command"

CONFIG_SCHEMA = button.button_schema(
    NiceT4Button
).extend({
    cv.GenerateID("nice_t4_id"): cv.use_id(nice_t4_ns.class_('NiceT4Hub')),
    cv.Required(CONF_COMMAND): cv.hex_uint8_t,
})

async def to_code(config):
    hub = await cg.get_variable(config["nice_t4_id"])
    var = cg.new_Pvariable(config[CONF_ID], hub, config[CONF_COMMAND])
    await button.register_button(var, config)
    await cg.register_component(var, config)
