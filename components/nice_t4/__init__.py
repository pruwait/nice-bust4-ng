import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ['uart']

# Пространство имён должно совпадать с namespace в вашем C++ коде
nice_t4_ns = cg.esphome_ns.namespace('nice_t4')

# Имя класса должно точно совпадать с основным классом-обёрткой в C++
NiceT4Component = nice_t4_ns.class_('NiceT4Component', cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(NiceT4Component),
}).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
