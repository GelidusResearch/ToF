# VL53L1X ESPHome External Component

ESPHome external component for the ST VL53L1X time-of-flight distance sensor.

## Features

- Distance output in meters
- Configurable `long_range` / short mode
- Configurable `timing_budget`
- Configurable measurement timeout
- Optional ROI size and center configuration
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
- `signal_rate_limit` (optional, default: `0.25`): minimum return signal threshold in MCPS, valid range `(0.0, 512.0)`
- `long_range` (optional, default: `true`): `true` for long mode, `false` for short mode
- `timeout` (optional, default: `500ms`): max `60s`
- `timing_budget` (optional, default: `50ms`): one of `20ms`, `33ms`, `50ms`, `100ms`, `200ms`, `500ms`
- `roi_width` (optional): ROI width in SPADs, range `4..16` (defaults to `16` if `roi_height` is set alone)
- `roi_height` (optional): ROI height in SPADs, range `4..16` (defaults to `16` if `roi_width` is set alone)
- `roi_center` (optional, default: `199`): ROI center SPAD index, range `0..255`
- `enable_pin` (optional): GPIO pin used to power-gate/enable the sensor during staged initialization

## ROI Example

```yaml
sensor:
  - platform: vl53l1x
    name: "Center ROI Distance"
    address: 0x29
    roi_width: 16
    roi_height: 16
    roi_center: 199
```

This maps to the ST API calls during setup:

- `VL53L1X_SetROI(dev, roi_width, roi_height)`
- `VL53L1X_SetROICenter(dev, roi_center)`
(dev is a data structure used to access the chip via the I2C address member)

```
SPAD location index map
Pin 1
┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
│ 128 │ 136 │ 144 │ 152 │ 160 │ 168 │ 176 │ 184 │ 192 │ 200 │ 208 │ 216 │ 224 │ 232 │ 240 │ 248 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 129 │ 137 │ 145 │ 153 │ 161 │ 169 │ 177 │ 185 │ 193 │ 201 │ 209 │ 217 │ 225 │ 233 │ 241 │ 249 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 130 │ 138 │ 146 │ 154 │ 162 │ 170 │ 178 │ 186 │ 194 │ 202 │ 210 │ 218 │ 226 │ 234 │ 242 │ 250 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 131 │ 139 │ 147 │ 155 │ 163 │ 171 │ 179 │ 187 │ 195 │ 203 │ 211 │ 219 │ 227 │ 235 │ 243 │ 251 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 132 │ 140 │ 148 │ 156 │ 164 │ 172 │ 180 │ 188 │ 196 │ 204 │ 212 │ 220 │ 228 │ 236 │ 244 │ 252 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 133 │ 141 │ 149 │ 157 │ 165 │ 173 │ 181 │ 189 │ 197 │ 205 │ 213 │ 221 │ 229 │ 237 │ 245 │ 253 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 134 │ 142 │ 150 │ 158 │ 166 │ 174 │ 182 │ 190 │ 198 │ 206 │ 214 │ 222 │ 230 │ 238 │ 246 │ 254 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 135 │ 143 │ 151 │ 159 │ 167 │ 175 │ 183 │ 191 │ 199 │ 207 │ 215 │ 223 │ 231 │ 239 │ 247 │ 255 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 127 │ 119 │ 111 │ 103 │  95 │  87 │  79 │  71 │  63 │  55 │  47 │  39 │  31 │  23 │  15 │   7 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 126 │ 118 │ 110 │ 102 │  94 │  86 │  78 │  70 │  62 │  54 │  46 │  38 │  30 │  22 │  14 │   6 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 125 │ 117 │ 109 │ 101 │  93 │  85 │  77 │  69 │  61 │  53 │  45 │  37 │  29 │  21 │  13 │   5 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 124 │ 116 │ 108 │ 100 │  92 │  84 │  76 │  68 │  60 │  52 │  44 │  36 │  28 │  20 │  12 │   4 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 123 │ 115 │ 107 │  99 │  91 │  83 │  75 │  67 │  59 │  51 │  43 │  35 │  27 │  19 │  11 │   3 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 122 │ 114 │ 106 │  98 │  90 │  82 │  74 │  66 │  58 │  50 │  42 │  34 │  26 │  18 │  10 │   2 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 121 │ 113 │ 105 │  97 │  89 │  81 │  73 │  65 │  57 │  49 │  41 │  33 │  25 │  17 │   9 │   1 │
├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
│ 120 │ 112 │ 104 │  96 │  88 │  80 │  72 │  64 │  56 │  48 │  40 │  32 │  24 │  16 │   8 │   0 │
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
```

Behavior:

- If either `roi_width` or `roi_height` is provided, `VL53L1X_SetROI` is called.
- Missing ROI size member falls back to `16`.
- `VL53L1X_SetROICenter` is always called, using default `199` when not specified.

## Signal Rate Limit Tuning

This component applies `signal_rate_limit` through `VL53L1X_SetSignalThreshold(...)`, which is the ST-driver equivalent of the common “signal-rate final range” limit check style APIs.

`MCPS` means *Mega Counts Per Second* (return photon/event rate). This threshold defines how strong a return must be to be accepted.

- Effective safe maximum in this implementation is about `65.535` MCPS (limited by `uint16_t` kcps conversion).
- Values above that can overflow conversion and should be avoided.

- Typical default: `0.25` MCPS
- Lower value (for example `0.1`): accepts weaker returns, improves long-range detectability, can increase noise
- Higher value (for example `0.3`): rejects weaker reflections, improves stability, can shorten effective range

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
