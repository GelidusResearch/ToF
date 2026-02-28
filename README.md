# VL53L1X ESPHome External Component

ESPHome external component for the ST VL53L1X time-of-flight distance sensor.

## Features

- Distance output in meters
- Configurable `long_range` / short mode
- Configurable `timing_budget`
- Configurable measurement timeout
- Optional `enable_pin` support for multi-sensor bring-up and re-addressing

## Installation

Use this repository as an external component in your ESPHome config.

```yaml
external_components:
  - source: github://GelidusResearch/ToF
    components: [vl53l1x]
```

## Basic Configuration

```yaml
i2c:
  sda: GPIO21
  scl: GPIO22
  scan: true

sensor:
  - platform: vl53l1x
    name: "Distance"
    address: 0x29
    update_interval: 60s
```

## Configuration Options

- `address` (optional, default: `0x29`): I2C address
- `signal_rate_limit` (optional, default: `0.25`): valid range `(0.0, 512.0)`
- `long_range` (optional, default: `true`): `true` for long mode, `false` for short mode
- `timeout` (optional, default: `100ms`): max `60s`
- `timing_budget` (optional, default: `50ms`): one of `20ms`, `33ms`, `50ms`, `100ms`, `200ms`, `500ms`
- `enable_pin` (optional): GPIO pin used to power-gate/enable the sensor during staged initialization

## Multiple Sensors on One I2C Bus

If you use multiple VL53L1X sensors on the same I2C bus, define `enable_pin` for **every** VL53L1X sensor.

Also, any sensor configured with a non-default `address` requires `enable_pin` so the component can safely bring sensors up one-by-one and assign addresses.

Example:

```yaml
sensor:
  - platform: vl53l1x
    name: "Front Distance"
    address: 0x29
    enable_pin: GPIO16

  - platform: vl53l1x
    name: "Rear Distance"
    address: 0x30
    enable_pin: GPIO17
```

## Notes

- Distances are published in meters.
- Readings can publish `NaN` when a measurement is invalid or times out.
- Recommended: keep `update_interval` at or above your chosen `timing_budget`.
