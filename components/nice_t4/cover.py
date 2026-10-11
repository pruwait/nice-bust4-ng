import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import cover
from esphome.const import CONF_ID, DEVICE_CLASS_GATE

DEPENDENCIES = ['nice_t4']

nice_t4_ns = cg.esphome_ns.namespace('nice_t4')
NiceT4Cover = nice_t4_ns.class_('NiceT4Cover', cover.Cover, cg.Component)

# Используем функцию cover_schema вместо устаревшего COVER_SCHEMA
CONFIG_SCHEMA = cover.cover_schema(
    NiceT4Cover,
    device_class=DEVICE_CLASS_GATE,
).extend({
    cv.GenerateID("nice_t4_id"): cv.use_id(nice_t4_ns.class_('NiceT4Hub')),
})

async def to_code(config):
    # Получаем ссылку на основной Hub
    hub = await cg.get_variable(config["nice_t4_id"])
    
    # Создаём переменную cover, передавая ей hub
    var = cg.new_Pvariable(config[CONF_ID], hub)
    
    # Регистрируем как cover-сущность и как компонент
    await cover.register_cover(var, config)
    await cg.register_component(var, config)
