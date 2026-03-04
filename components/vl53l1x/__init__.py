from esphome import pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
import esphome.components.sensor as esphome_sensor
from esphome.const import (
    CONF_ADDRESS,
    CONF_ENABLE_PIN,
    CONF_TIMEOUT,
    ICON_ARROW_EXPAND_VERTICAL,
    STATE_CLASS_MEASUREMENT,
    UNIT_METER,
)

AUTO_LOAD = ["sensor"]
DEPENDENCIES = ["i2c"]

vl53l1x_ns = cg.esphome_ns.namespace("vl53l1x")
VL53L1XSensor = vl53l1x_ns.class_(
    "VL53L1XSensor", esphome_sensor.Sensor, cg.PollingComponent, i2c.I2CDevice
)

CONF_SIGNAL_RATE_LIMIT = "signal_rate_limit"
CONF_LONG_RANGE = "long_range"
CONF_TIMING_BUDGET = "timing_budget"
CONF_ROI_WIDTH = "roi_width"
CONF_ROI_HEIGHT = "roi_height"
CONF_ROI_CENTER = "roi_center"
SUPPORTED_TIMING_BUDGET_US = [20000, 33000, 50000, 100000, 200000, 500000]
DEFAULT_ROI_WIDTH = 16
DEFAULT_ROI_HEIGHT = 16


def check_keys(obj):
    if obj[CONF_ADDRESS] != 0x29 and CONF_ENABLE_PIN not in obj:
        msg = "Address other then 0x29 requires enable_pin definition to allow sensor\r"
        msg += "re-addressing. Also if you have more then one VL53 device on the same\r"
        msg += "i2c bus, then all VL53 devices must have enable_pin defined."
        raise cv.Invalid(msg)
    return obj


def check_timeout(value):
    value = cv.positive_time_period_microseconds(value)
    if value.total_seconds > 60:
        raise cv.Invalid("Maximum timeout can not be greater then 60 seconds")
    return value


def check_timing_budget(value):
    value = cv.positive_time_period_microseconds(value)
    if value.total_microseconds not in SUPPORTED_TIMING_BUDGET_US:
        raise cv.Invalid(
            "timing_budget must be one of 20ms, 33ms, 50ms, 100ms, 200ms, or 500ms"
        )
    return value


CONFIG_SCHEMA = cv.All(
    esphome_sensor.sensor_schema(
        VL53L1XSensor,
        unit_of_measurement=UNIT_METER,
        icon=ICON_ARROW_EXPAND_VERTICAL,
        accuracy_decimals=2,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.Optional(CONF_SIGNAL_RATE_LIMIT, default=0.25): cv.float_range(
                min=0.0, max=512.0, min_included=False, max_included=False
            ),
            cv.Optional(CONF_LONG_RANGE, default=True): cv.boolean,
            cv.Optional(CONF_TIMEOUT, default="500ms"): check_timeout,
            cv.Optional(CONF_ENABLE_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_TIMING_BUDGET, default="50ms"): cv.All(
                check_timing_budget,
            ),
            cv.Optional(CONF_ROI_WIDTH): cv.int_range(min=4, max=16),
            cv.Optional(CONF_ROI_HEIGHT): cv.int_range(min=4, max=16),
            cv.Optional(CONF_ROI_CENTER, default=199): cv.int_range(min=0, max=255),
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(i2c.i2c_device_schema(0x29)),
    check_keys,
)


async def to_code(config):
    var = await esphome_sensor.new_sensor(config)
    await cg.register_component(var, config)
    cg.add(var.set_signal_rate_limit(config[CONF_SIGNAL_RATE_LIMIT]))
    cg.add(var.set_long_range(config[CONF_LONG_RANGE]))
    cg.add(var.set_timeout_us(config[CONF_TIMEOUT].total_microseconds))

    if CONF_ENABLE_PIN in config:
        enable = await cg.gpio_pin_expression(config[CONF_ENABLE_PIN])
        cg.add(var.set_enable_pin(enable))

    if timing_budget := config.get(CONF_TIMING_BUDGET):
        cg.add(var.set_timing_budget(timing_budget.total_microseconds))

    if CONF_ROI_WIDTH in config or CONF_ROI_HEIGHT in config:
        cg.add(var.set_roi_width(config.get(CONF_ROI_WIDTH, DEFAULT_ROI_WIDTH)))
        cg.add(var.set_roi_height(config.get(CONF_ROI_HEIGHT, DEFAULT_ROI_HEIGHT)))

    cg.add(var.set_roi_center(config[CONF_ROI_CENTER]))

    await i2c.register_i2c_device(var, config)
