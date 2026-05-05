from esphome.components import text_sensor
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_TYPE, CONF_ICON
from .. import truma_inetbox_ns, CONF_TRUMA_INETBOX_ID, TrumaINetBoxApp

DEPENDENCIES = ["truma_inetbox"]
CODEOWNERS = ["@bertel2020"]

CONF_CLASS = "class"
TrumaTextSensor = truma_inetbox_ns.class_("TrumaTextSensor", text_sensor.TextSensor, cg.Component)
TRUMA_TEXT_SENSOR_TYPE_dummy_ns = truma_inetbox_ns.namespace("TRUMA_TEXT_SENSOR_TYPE")

CONF_SUPPORTED_TYPE = {
    "CLOCK": {
        CONF_CLASS: TRUMA_TEXT_SENSOR_TYPE_dummy_ns.CLOCK,
        CONF_ICON: "mdi:clock-outline",
    },
    "UPDATE_STATUS": {
        CONF_CLASS: TRUMA_TEXT_SENSOR_TYPE_dummy_ns.UPDATE_STATUS,
        CONF_ICON: "mdi:update",
    },
    "CP_PLUS_STATUS": {
        CONF_CLASS: TRUMA_TEXT_SENSOR_TYPE_dummy_ns.CP_PLUS_STATUS,
        CONF_ICON: "mdi:connection",
    },
}


def set_default_based_on_type():
    def set_defaults_(config):
        t = CONF_SUPPORTED_TYPE[config[CONF_TYPE]]
        if CONF_ICON in t and CONF_ICON not in config:
            config[CONF_ICON] = t[CONF_ICON]
        return config
    return set_defaults_


CONFIG_SCHEMA = text_sensor.text_sensor_schema().extend(cv.Schema({
    cv.GenerateID(): cv.declare_id(TrumaTextSensor),
    cv.GenerateID(CONF_TRUMA_INETBOX_ID): cv.use_id(TrumaINetBoxApp),
    cv.Required(CONF_TYPE): cv.enum(CONF_SUPPORTED_TYPE, upper=True),
})).extend(cv.COMPONENT_SCHEMA)

FINAL_VALIDATE_SCHEMA = set_default_based_on_type()


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await text_sensor.register_text_sensor(var, config)
    parent = await cg.get_variable(config[CONF_TRUMA_INETBOX_ID])
    await cg.register_parented(var, config[CONF_TRUMA_INETBOX_ID])
    cg.add(var.set_type(CONF_SUPPORTED_TYPE[config[CONF_TYPE]][CONF_CLASS]))
    # Register for update_status polling (UPDATE_STATUS and CP_PLUS_STATUS)
    if config[CONF_TYPE] in ["UPDATE_STATUS", "CP_PLUS_STATUS"]:
        cg.add(parent.register_text_sensor(var))
