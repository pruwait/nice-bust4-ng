import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@pruwait"]

nice_t4_ns = cg.esphome_ns.namespace("nice_t4")
NiceT4Hub = nice_t4_ns.class_("NiceT4Hub", cg.Component, uart.UARTDevice)

CONF_NICE_T4_ID = "nice_t4_id"
CONF_ADDRESS = "address"
CONF_TARGET = "target"
CONF_IDENTITY = "identity"
CONF_MODEL = "model"
CONF_ALLOW_WRITE = "allow_write"
CONF_ALLOW_COMMANDS = "allow_commands"
CONF_BREAK_MODE = "break_mode"
CONF_STATUS_POLL_INTERVAL = "status_poll_interval"

# DEVICE в кадрах команд: идентичность отправителя (O-View или IT4WIFI, второй нужен для блокировок)
IDENTITY_OPTIONS = {"oview": 0x01, "it4wifi": 0x02}
# byte: простой байт 00 на рабочей скорости (проверено на RB500HS), baud: 00 на 9200 бод
BREAK_MODES = {"byte": 0, "baud": 1}


def validate_address(value):
    """Адрес 16 бит: старший байт ADR, младший EPT (например 0x0066)."""
    value = cv.hex_uint16_t(value)
    ept = value & 0xFF
    if ept in (0x00, 0xFF):
        raise cv.Invalid("EPT 0x00 и 0xFF недопустимы (0xFF это широковещание)")
    return value


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(NiceT4Hub),
            cv.Optional(CONF_ADDRESS, default=0x0066): validate_address,
            cv.Optional(CONF_TARGET, default=0x00FF): cv.hex_uint16_t,
            cv.Optional(CONF_IDENTITY, default="oview"): cv.enum(IDENTITY_OPTIONS, lower=True),
            cv.Optional(CONF_MODEL, default="auto"): cv.string,
            cv.Optional(CONF_ALLOW_WRITE, default=False): cv.boolean,
            cv.Optional(CONF_ALLOW_COMMANDS, default=True): cv.boolean,
            cv.Optional(CONF_BREAK_MODE, default="byte"): cv.enum(BREAK_MODES, lower=True),
            cv.Optional(CONF_STATUS_POLL_INTERVAL, default="15s"): cv.time_period,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "nice_t4",
    baud_rate=19200,
    require_tx=True,
    require_rx=True,
    data_bits=8,
    parity="NONE",
    stop_bits=1,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_target(config[CONF_TARGET]))
    cg.add(var.set_identity(config[CONF_IDENTITY]))
    cg.add(var.set_model(config[CONF_MODEL]))
    cg.add(var.set_allow_write(config[CONF_ALLOW_WRITE]))
    cg.add(var.set_allow_commands(config[CONF_ALLOW_COMMANDS]))
    cg.add(var.set_break_mode(config[CONF_BREAK_MODE]))
    cg.add(var.set_status_poll_interval(config[CONF_STATUS_POLL_INTERVAL].total_milliseconds))
