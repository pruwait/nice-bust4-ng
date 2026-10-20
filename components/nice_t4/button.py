import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID

from . import CONF_NICE_T4_ID, NiceT4Hub, nice_t4_ns
from .command_codes import DEP_COMMANDS

DEPENDENCIES = ["nice_t4"]

NiceT4Button = nice_t4_ns.class_("NiceT4Button", button.Button, cg.Component)

CONF_COMMAND = "command"

# Короткие имена основных команд; полный список (47 команд библиотеки) берётся из command_codes.py.
ALIASES = {
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
    "light_timer": 0x11,
    "light_toggle": 0x12,
    "release_and_open": 0x19,
    "release_and_close": 0x1A,
}
COMMANDS = dict(DEP_COMMANDS)
COMMANDS.update(ALIASES)

CONFIG_SCHEMA = button.button_schema(NiceT4Button).extend(
    {
        cv.GenerateID(CONF_NICE_T4_ID): cv.use_id(NiceT4Hub),
        # имя команды или код (например 0x1B)
        cv.Required(CONF_COMMAND): cv.Any(cv.enum(COMMANDS, lower=True), cv.hex_uint8_t),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_NICE_T4_ID])
    var = cg.new_Pvariable(config[CONF_ID], hub, config[CONF_COMMAND])
    await cg.register_component(var, config)
    await button.register_button(var, config)
