# Re-export CONFIG_SCHEMA and to_code from __init__.py so that
# both `sensor: - platform: vl53l1x` and top-level `vl53l1x:` work.
from . import CONFIG_SCHEMA, to_code  # noqa: F401
