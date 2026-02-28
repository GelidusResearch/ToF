#pragma once

#include <list>

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace vl53l1x {

class VL53L1XSensor : public sensor::Sensor, public PollingComponent, public i2c::I2CDevice {
 public:
  VL53L1XSensor();

  static void set_active_sensor(VL53L1XSensor *sensor);
  static VL53L1XSensor *get_active_sensor();

  bool api_write_multi(uint16_t dev, uint16_t index, const uint8_t *data, uint32_t count);
  bool api_read_multi(uint16_t dev, uint16_t index, uint8_t *data, uint32_t count);

  void setup() override;

  void dump_config() override;
  void update() override;

  void loop() override;

  void set_signal_rate_limit(float signal_rate_limit) { signal_rate_limit_ = signal_rate_limit; }
  void set_long_range(bool long_range) { long_range_ = long_range; }
  void set_timeout_us(uint32_t timeout_us) { this->timeout_us_ = timeout_us; }
  void set_enable_pin(GPIOPin *enable) { this->enable_pin_ = enable; }
  void set_timing_budget(uint32_t timing_budget) { this->measurement_timing_budget_us_ = timing_budget; }

 protected:
  bool write_reg_(uint16_t reg, uint8_t value);
  bool read_reg_(uint16_t reg, uint8_t *value);
  bool read_reg16_(uint16_t reg, uint16_t *value);

  float signal_rate_limit_{0.25f};
  bool long_range_{true};
  bool id_read_ok_{false};
  uint8_t model_id_hi_{0};
  uint8_t module_type_{0};
  uint8_t revision_id_{0};
  bool measurement_started_{false};
  GPIOPin *enable_pin_{nullptr};
  uint32_t measurement_timing_budget_us_{50000};
  bool initiated_read_{false};

  uint32_t timeout_start_us_;
  uint32_t timeout_us_{};

  static std::list<VL53L1XSensor *> vl53_sensors;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
  static bool enable_pin_setup_complete;           // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
  static VL53L1XSensor *active_sensor_;            // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
};

}  // namespace vl53l1x
}  // namespace esphome
