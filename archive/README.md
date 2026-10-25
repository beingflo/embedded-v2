# Archive

Superseded sketches, kept for reference. Nothing here is flashed any more, and
nothing here is expected to build.

- `co2-sensor-esp-now/` — the 2023 gateway: read an SCD30 locally and received the
  two battery nodes over ESP-NOW, then uploaded all three as Prometheus text to
  three separate endpoints. Those endpoint URLs were never committed (all three
  strings are empty in every revision), so this sketch cannot be rebuilt as it ran.
- `light-sensor-esp-now/` — lux, soil moisture and battery voltage, sent over
  ESP-NOW every 30 s.
- `pressure-sensor-esp-now/` — pressure and temperature, sent over ESP-NOW every
  10 min.

These were replaced in late 2024 by three independent devices that upload over
WiFi themselves: `co2-sensor/`, `humidity-sensor/` and `brightness-sensor/`. The
evidence is in acta's history: the migrated rows carry one source per device, and
cutting power to the CO₂ device no longer stops the other two.
