#include "vl53l1x.h"
extern "C" {
#include "VL53L1X_api.h"
}
#include "esphome/core/log.h"
#include <vector>

namespace esphome {
namespace vl53l1x {

static const char *const TAG = "vl53l1x";

static const uint16_t VL53L1_IDENTIFICATION__MODULE_TYPE = 0x0110;
static const uint16_t VL53L1_IDENTIFICATION__REVISION_ID = 0x0111;
static const uint16_t VL53L1_PAD_I2C_HV__EXTSUP_CONFIG = 0x002E;

static const uint16_t VL53L1_SOFT_RESET = SOFT_RESET;
static const uint16_t VL53L1_SYSTEM__MODE_START = SYSTEM__MODE_START;

std::list<VL53L1XSensor *> VL53L1XSensor::vl53_sensors;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
bool VL53L1XSensor::enable_pin_setup_complete = false;   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
VL53L1XSensor *VL53L1XSensor::active_sensor_ = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

VL53L1XSensor::VL53L1XSensor() { VL53L1XSensor::vl53_sensors.push_back(this); }

void VL53L1XSensor::set_active_sensor(VL53L1XSensor *sensor) { VL53L1XSensor::active_sensor_ = sensor; }

VL53L1XSensor *VL53L1XSensor::get_active_sensor() { return VL53L1XSensor::active_sensor_; }

bool VL53L1XSensor::api_write_multi(uint16_t dev, uint16_t index, const uint8_t *data, uint32_t count) {
	uint8_t dev7 = dev > 0x7F ? static_cast<uint8_t>(dev >> 1) : static_cast<uint8_t>(dev);
	uint8_t prev = this->address_;
	this->set_i2c_address(dev7);

	std::vector<uint8_t> buffer;
	buffer.reserve(2 + count);
	buffer.push_back(static_cast<uint8_t>(index >> 8));
	buffer.push_back(static_cast<uint8_t>(index & 0xFF));
	buffer.insert(buffer.end(), data, data + count);

	auto err = this->write(buffer.data(), buffer.size());
	this->set_i2c_address(prev);
	return err == i2c::ERROR_OK;
}

bool VL53L1XSensor::api_read_multi(uint16_t dev, uint16_t index, uint8_t *data, uint32_t count) {
	uint8_t dev7 = dev > 0x7F ? static_cast<uint8_t>(dev >> 1) : static_cast<uint8_t>(dev);
	uint8_t prev = this->address_;
	this->set_i2c_address(dev7);

	uint8_t reg_addr[2] = {static_cast<uint8_t>(index >> 8), static_cast<uint8_t>(index & 0xFF)};
	auto err = this->write_read(reg_addr, sizeof(reg_addr), data, count);

	this->set_i2c_address(prev);
	return err == i2c::ERROR_OK;
}

void VL53L1XSensor::dump_config() {
	LOG_SENSOR("", "VL53L1X", this);
	LOG_UPDATE_INTERVAL(this);
	LOG_I2C_DEVICE(this);
	if (this->enable_pin_ != nullptr) {
		LOG_PIN("  Enable Pin: ", this->enable_pin_);
	}
	ESP_LOGCONFIG(TAG, "  Timeout: %u%s", this->timeout_us_, this->timeout_us_ > 0 ? "us" : " (no timeout)");
	ESP_LOGCONFIG(TAG, "  Timing Budget: %u us", this->measurement_timing_budget_us_);
	ESP_LOGCONFIG(TAG, "  Distance Mode: %s", this->long_range_ ? "long" : "short");
	if (this->id_read_ok_) {
		ESP_LOGCONFIG(TAG, "  Model ID 0x%04X 0x%02X", VL53L1_IDENTIFICATION__MODEL_ID, this->model_id_hi_);
		ESP_LOGCONFIG(TAG, "  Module type 0x%04X 0x%02X", VL53L1_IDENTIFICATION__MODULE_TYPE, this->module_type_);
		ESP_LOGCONFIG(TAG, "  Mask revision 0x%04X 0x%02X", VL53L1_IDENTIFICATION__REVISION_ID, this->revision_id_);
	} else {
		ESP_LOGCONFIG(TAG, "  Identification registers: unavailable");
	}
}

void VL53L1XSensor::setup() {
	VL53L1XSensor::set_active_sensor(this);
	if (!esphome::vl53l1x::VL53L1XSensor::enable_pin_setup_complete) {
		for (auto &vl53_sensor : vl53_sensors) {
			if (vl53_sensor->enable_pin_ != nullptr) {
				vl53_sensor->enable_pin_->setup();
				vl53_sensor->enable_pin_->digital_write(false);
			}
		}
		esphome::vl53l1x::VL53L1XSensor::enable_pin_setup_complete = true;
	}

	if (this->enable_pin_ != nullptr) {
		this->enable_pin_->digital_write(true);
		delayMicroseconds(100);
	}

	uint8_t final_address = address_;
	this->set_i2c_address(0x29);

	uint16_t model_id = 0;
	if (!this->read_reg16_(VL53L1_IDENTIFICATION__MODEL_ID, &model_id)) {
		ESP_LOGE(TAG, "'%s' - failed reading model id", this->name_.c_str());
		this->mark_failed();
		return;
	}
	uint8_t module_type = 0;
	if (!this->read_reg_(VL53L1_IDENTIFICATION__MODULE_TYPE, &module_type)) {
		ESP_LOGE(TAG, "'%s' - failed reading module type", this->name_.c_str());
		this->mark_failed();
		return;
	}
	uint8_t revision_id = 0;
	if (!this->read_reg_(VL53L1_IDENTIFICATION__REVISION_ID, &revision_id)) {
		ESP_LOGE(TAG, "'%s' - failed reading mask revision", this->name_.c_str());
		this->mark_failed();
		return;
	}
	ESP_LOGCONFIG(TAG, "  Model ID 0x%04X 0x%02X", VL53L1_IDENTIFICATION__MODEL_ID,
						 static_cast<uint8_t>(model_id >> 8));
	ESP_LOGCONFIG(TAG, "  Module type 0x%04X 0x%02X", VL53L1_IDENTIFICATION__MODULE_TYPE, module_type);
	ESP_LOGCONFIG(TAG, "  Mask revision 0x%04X 0x%02X", VL53L1_IDENTIFICATION__REVISION_ID, revision_id);
	ESP_LOGI(TAG, "Model ID 0x%04X 0x%02X", VL53L1_IDENTIFICATION__MODEL_ID, static_cast<uint8_t>(model_id >> 8));
	ESP_LOGI(TAG, "Module type 0x%04X 0x%02X", VL53L1_IDENTIFICATION__MODULE_TYPE, module_type);
	ESP_LOGI(TAG, "Mask revision 0x%04X 0x%02X", VL53L1_IDENTIFICATION__REVISION_ID, revision_id);
	this->model_id_hi_ = static_cast<uint8_t>(model_id >> 8);
	this->module_type_ = module_type;
	this->revision_id_ = revision_id;
	this->id_read_ok_ = true;
	if (model_id != 0xEACC) {
		ESP_LOGE(TAG, "'%s' - unexpected model id 0x%04X", this->name_.c_str(), model_id);
		this->mark_failed();
		return;
	}

	if (!this->write_reg_(VL53L1_SOFT_RESET, 0x00)) {
		ESP_LOGE(TAG, "'%s' - failed to write soft reset 0", this->name_.c_str());
		this->mark_failed();
		return;
	}
	delayMicroseconds(100);
	if (!this->write_reg_(VL53L1_SOFT_RESET, 0x01)) {
		ESP_LOGE(TAG, "'%s' - failed to write soft reset 1", this->name_.c_str());
		this->mark_failed();
		return;
	}
	delay(1);

	uint32_t start_us = micros();
	uint8_t boot_state = 0;
	while ((boot_state & 0x01) == 0) {
		if (!this->read_reg_(VL53L1_FIRMWARE__SYSTEM_STATUS, &boot_state)) {
			ESP_LOGE(TAG, "'%s' - failed reading boot state", this->name_.c_str());
			this->mark_failed();
			return;
		}

		if (this->timeout_us_ > 0 && ((micros() - start_us) > this->timeout_us_)) {
			ESP_LOGE(TAG, "'%s' - setup timeout waiting for boot", this->name_.c_str());
			this->mark_failed();
			return;
		}
		delay(2);
		yield();
	}

	uint8_t ext_sup = 0;
	if (!this->read_reg_(VL53L1_PAD_I2C_HV__EXTSUP_CONFIG, &ext_sup)) {
		ESP_LOGE(TAG, "'%s' - failed reading PAD_I2C_HV__EXTSUP_CONFIG", this->name_.c_str());
		this->mark_failed();
		return;
	}
	if (!this->write_reg_(VL53L1_PAD_I2C_HV__EXTSUP_CONFIG, ext_sup | 0x01)) {
		ESP_LOGE(TAG, "'%s' - failed enabling 2V8 I/O mode", this->name_.c_str());
		this->mark_failed();
		return;
	}

	if (VL53L1X_SensorInit(0x29) != 0) {
		ESP_LOGE(TAG, "'%s' - VL53L1X_SensorInit failed", this->name_.c_str());
		this->mark_failed();
		return;
	}

	if (VL53L1X_SetDistanceMode(0x29, this->long_range_ ? 2 : 1) != 0) {
		ESP_LOGE(TAG, "'%s' - failed to set distance mode", this->name_.c_str());
		this->mark_failed();
		return;
	}

	if (this->measurement_timing_budget_us_ == 0) {
		this->measurement_timing_budget_us_ = 50000;
	}
	uint16_t timing_budget_ms = static_cast<uint16_t>(this->measurement_timing_budget_us_ / 1000U);
	if (timing_budget_ms == 0)
		timing_budget_ms = 50;
	if (VL53L1X_SetTimingBudgetInMs(0x29, timing_budget_ms) != 0) {
		ESP_LOGE(TAG, "'%s' - invalid timing budget %u us", this->name_.c_str(), this->measurement_timing_budget_us_);
		this->mark_failed();
		return;
	}
	this->measurement_timing_budget_us_ = static_cast<uint32_t>(timing_budget_ms) * 1000U;

	if (VL53L1X_SetInterMeasurementInMs(0x29, timing_budget_ms) != 0) {
		ESP_LOGE(TAG, "'%s' - failed to set intermeasurement", this->name_.c_str());
		this->mark_failed();
		return;
	}

	const uint32_t min_timeout_us = this->measurement_timing_budget_us_ * 2U;
	if (this->timeout_us_ > 0 && this->timeout_us_ < min_timeout_us) {
		ESP_LOGW(TAG, "'%s' - timeout %u us too low for timing budget %u us, clamping to %u us", this->name_.c_str(),
					 this->timeout_us_, this->measurement_timing_budget_us_, min_timeout_us);
		this->timeout_us_ = min_timeout_us;
	}

	uint16_t signal_threshold_kcps = static_cast<uint16_t>(this->signal_rate_limit_ * 1000.0f);
	if (VL53L1X_SetSignalThreshold(0x29, signal_threshold_kcps) != 0) {
		ESP_LOGW(TAG, "'%s' - failed to set signal threshold", this->name_.c_str());
	}

	if (VL53L1X_SetI2CAddress(0x29, static_cast<uint8_t>(final_address << 1)) != 0) {
		ESP_LOGE(TAG, "'%s' - failed to set I2C address", this->name_.c_str());
		this->mark_failed();
		return;
	}
	this->set_i2c_address(final_address);

	VL53L1XSensor::set_active_sensor(this);
	if (VL53L1X_StartRanging(final_address) != 0) {
		ESP_LOGE(TAG, "'%s' - failed to start measurement", this->name_.c_str());
		this->mark_failed();
		return;
	}
	this->measurement_started_ = true;
}

void VL53L1XSensor::update() {
	if (!this->measurement_started_) {
		VL53L1XSensor::set_active_sensor(this);
		if (VL53L1X_StartRanging(this->address_) != 0) {
			ESP_LOGW(TAG, "'%s' - failed to start ranging", this->name_.c_str());
			this->publish_state(NAN);
			this->status_momentary_warning("start", 5000);
			return;
		}
		this->measurement_started_ = true;
	}

	if (this->initiated_read_) {
		this->publish_state(NAN);
		this->status_momentary_warning("update", 5000);
		ESP_LOGW(TAG, "%s - update called before prior reading complete - initiated:%d", this->name_.c_str(),
						 this->initiated_read_);
		return;
	}

	this->initiated_read_ = true;
	this->timeout_start_us_ = micros();
}

void VL53L1XSensor::loop() {
	if (!this->initiated_read_)
		return;
	VL53L1XSensor::set_active_sensor(this);

	const uint32_t elapsed_us = micros() - this->timeout_start_us_;
	if (elapsed_us < this->measurement_timing_budget_us_) {
		return;
	}
	uint8_t ready = 0;
	if (VL53L1X_CheckForDataReady(this->address_, &ready) != 0) {
		this->initiated_read_ = false;
		this->publish_state(NAN);
		this->status_momentary_warning("read", 5000);
		return;
	}

	if (ready == 0) {
		if (this->timeout_us_ > 0 && (elapsed_us > this->timeout_us_)) {
			ESP_LOGW(TAG, "'%s' - timeout waiting for data-ready", this->name_.c_str());
			this->initiated_read_ = false;
			this->publish_state(NAN);
			this->status_momentary_warning("timeout", 5000);
		}
		return;
	}

	uint16_t range_mm = 0;
	uint8_t range_status = 255;
	if (VL53L1X_GetRangeStatus(this->address_, &range_status) != 0 ||
			VL53L1X_GetDistance(this->address_, &range_mm) != 0) {
		VL53L1X_ClearInterrupt(this->address_);
		this->initiated_read_ = false;
		this->publish_state(NAN);
		this->status_momentary_warning("read", 5000);
		return;
	}

	VL53L1X_ClearInterrupt(this->address_);
	this->initiated_read_ = false;

	const bool soft_phase_error = (range_status == 4 && range_mm > 0 && range_mm < 4000);
	if ((!soft_phase_error && range_status != 0) || range_mm >= 4000) {
		ESP_LOGD(TAG, "'%s' - invalid range: raw_status=%u distance=%u mm", this->name_.c_str(), range_status, range_mm);
		this->publish_state(NAN);
		return;
	}

	float range_m = range_mm / 1e3f;
	if (soft_phase_error) {
		ESP_LOGD(TAG, "'%s' - soft phase error accepted: status=%u distance=%.3f m", this->name_.c_str(), range_status,
					 range_m);
	}
	ESP_LOGD(TAG, "'%s' - Got distance %.3f m", this->name_.c_str(), range_m);
	this->publish_state(range_m);
}

bool VL53L1XSensor::write_reg_(uint16_t reg, uint8_t value) {
	uint8_t buffer[3] = {static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xFF), value};
	auto err = this->write(buffer, sizeof(buffer));
	if (err == i2c::ERROR_OK) {
		ESP_LOGVV(TAG, "I2C WR8  dev=0x%02X reg=0x%04X val=0x%02X ok", this->address_, reg, value);
		return true;
	}
	ESP_LOGV(TAG, "I2C WR8  dev=0x%02X reg=0x%04X val=0x%02X err=%d", this->address_, reg, value, err);
	return false;
}

bool VL53L1XSensor::read_reg_(uint16_t reg, uint8_t *value) {
	uint8_t reg_addr[2] = {static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xFF)};
	auto err = this->write_read(reg_addr, sizeof(reg_addr), value, 1);
	if (err == i2c::ERROR_OK) {
		ESP_LOGVV(TAG, "I2C RD8  dev=0x%02X reg=0x%04X val=0x%02X ok", this->address_, reg, *value);
		return true;
	}
	ESP_LOGV(TAG, "I2C RD8  dev=0x%02X reg=0x%04X err=%d", this->address_, reg, err);
	return false;
}

bool VL53L1XSensor::read_reg16_(uint16_t reg, uint16_t *value) {
	uint8_t reg_addr[2] = {static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xFF)};
	uint8_t data[2];
	auto err = this->write_read(reg_addr, sizeof(reg_addr), data, sizeof(data));
	if (err != i2c::ERROR_OK) {
		ESP_LOGV(TAG, "I2C RD16 dev=0x%02X reg=0x%04X err=%d", this->address_, reg, err);
		return false;
	}
	*value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
	ESP_LOGVV(TAG, "I2C RD16 dev=0x%02X reg=0x%04X val=0x%04X ok", this->address_, reg, *value);
	return true;
}
}  // namespace vl53l1x
}  // namespace esphome

extern "C" int8_t VL53L1_WriteMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
	auto *sensor = esphome::vl53l1x::VL53L1XSensor::get_active_sensor();
	if (sensor == nullptr || !sensor->api_write_multi(dev, index, pdata, count)) {
		return 1;
	}
	return 0;
}

extern "C" int8_t VL53L1_ReadMulti(uint16_t dev, uint16_t index, uint8_t *pdata, uint32_t count) {
	auto *sensor = esphome::vl53l1x::VL53L1XSensor::get_active_sensor();
	if (sensor == nullptr || !sensor->api_read_multi(dev, index, pdata, count)) {
		return 1;
	}
	return 0;
}

extern "C" int8_t VL53L1_WrByte(uint16_t dev, uint16_t index, uint8_t data) {
	return VL53L1_WriteMulti(dev, index, &data, 1);
}

extern "C" int8_t VL53L1_WrWord(uint16_t dev, uint16_t index, uint16_t data) {
	uint8_t buffer[2] = {static_cast<uint8_t>(data >> 8), static_cast<uint8_t>(data & 0xFF)};
	return VL53L1_WriteMulti(dev, index, buffer, 2);
}

extern "C" int8_t VL53L1_WrDWord(uint16_t dev, uint16_t index, uint32_t data) {
	uint8_t buffer[4] = {static_cast<uint8_t>(data >> 24), static_cast<uint8_t>(data >> 16),
										 static_cast<uint8_t>(data >> 8), static_cast<uint8_t>(data & 0xFF)};
	return VL53L1_WriteMulti(dev, index, buffer, 4);
}

extern "C" int8_t VL53L1_RdByte(uint16_t dev, uint16_t index, uint8_t *data) {
	return VL53L1_ReadMulti(dev, index, data, 1);
}

extern "C" int8_t VL53L1_RdWord(uint16_t dev, uint16_t index, uint16_t *data) {
	uint8_t buffer[2];
	int8_t status = VL53L1_ReadMulti(dev, index, buffer, 2);
	if (status == 0) {
		*data = (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
	}
	return status;
}

extern "C" int8_t VL53L1_RdDWord(uint16_t dev, uint16_t index, uint32_t *data) {
	uint8_t buffer[4];
	int8_t status = VL53L1_ReadMulti(dev, index, buffer, 4);
	if (status == 0) {
		*data = (static_cast<uint32_t>(buffer[0]) << 24) | (static_cast<uint32_t>(buffer[1]) << 16) |
						(static_cast<uint32_t>(buffer[2]) << 8) | buffer[3];
	}
	return status;
}

extern "C" int8_t VL53L1_WaitMs(uint16_t, int32_t wait_ms) {
	esphome::delay(wait_ms > 0 ? static_cast<uint32_t>(wait_ms) : 0);
	return 0;
}
