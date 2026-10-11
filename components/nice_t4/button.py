import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID

DEPENDENCIES = ['nice_t4']

nice_t4_ns = cg.esphome_ns.namespace('nice_t4')
NiceT4Button = nice_t4_ns.class_('NiceT4Button', button.Button, cg.Component)

CONF_COMMAND = "command"

# Словарь команд согласно протоколу Nice BusT4 (из t4_types.h namespace cmd)
COMMAND_OPTIONS = {
    "release_last": 0x00,
    "sbs": 0x01,
    "stop": 0x02,
    "open": 0x03,
    "close": 0x04,
    "partial_open_1": 0x05,
    "partial_open_2": 0x06,
    "partial_open_3": 0x07,
    "apartment_sbs": 0x0B,
    "sbs_high_priority": 0x0C,
    "open_and_block": 0x0D,
    "close_and_block": 0x0E,
    "block": 0x0F,
    "release": 0x10,
    "release_and_open": 0x19,
    "release_and_close": 0x1A,
    "run_command": 0x82,
}

CONFIG_SCHEMA = button.button_schema(
    NiceT4Button
).extend({
    cv.GenerateID("nice_t4_id"): cv.use_id(nice_t4_ns.class_('NiceT4Hub')),
    cv.Required(CONF_COMMAND): cv.enum(COMMAND_OPTIONS, lower=True),
})

async def to_code(config):
    hub = await cg.get_variable(config["nice_t4_id"])
    var = cg.new_Pvariable(config[CONF_ID], hub, config[CONF_COMMAND])
    await button.register_button(var, config)
    await cg.register_component(var, config)
