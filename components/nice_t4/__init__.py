import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ['uart']
CODEOWNERS = ["@pruwait"]

nice_t4_ns = cg.esphome_ns.namespace('nice_t4')
NiceT4Hub = nice_t4_ns.class_('NiceT4Hub', cg.Component, uart.UARTDevice)

# Константы для параметров
CONF_ADDRESS = "address"
CONF_IDENTITY = "identity"
CONF_MODEL = "model"
CONF_ALLOW_WRITE = "allow_write"

# Словарь допустимых значений для identity
IDENTITY_OPTIONS = {
    "standard": 0x00,
    "oview": 0x01,
    "it4wifi": 0x02,
    "controller": 0x04,
    "screen": 0x06,
    "radio": 0x0A,
    "psu": 0x11,
}

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(NiceT4Hub),
    cv.Optional(CONF_ADDRESS, default=0x01): cv.hex_uint8_t,
    cv.Optional(CONF_IDENTITY, default="standard"): cv.enum(IDENTITY_OPTIONS, lower=True),
    cv.Optional(CONF_MODEL, default="auto"): cv.string,
    cv.Optional(CONF_ALLOW_WRITE, default=True): cv.boolean,
}).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    
    # Передаём параметры в C++ объект
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_identity(config[CONF_IDENTITY]))
    cg.add(var.set_model(config[CONF_MODEL]))
    cg.add(var.set_allow_write(config[CONF_ALLOW_WRITE]))
