// Local configuration for every sensor in this repository.
//
// Copy this file to `common/config.h` and fill in the values:
//
//     cp common/config.example.h common/config.h
//
// `config.h` is gitignored, so the credentials never enter the repository, and
// all three sketches read them from that single file. These are `#define`s, so
// the values are baked into the firmware image at compile time: changing one
// means reflashing every device that uses it.
#pragma once

// 2.4 GHz network only, the ESP32 cannot see 5 GHz ones.
#define WIFI_SSID "…"
#define WIFI_PASSWORD "…"

// acta's ingest endpoint, and the sensor credential: `ACTA_AUTH_SENSOR_USER`
// and `ACTA_AUTH_SENSOR_PASSWORD` from the acta deploy secrets. That role may
// only `POST /api/ingest`, so it can be rotated without touching the dashboard
// password. The URL is plain `https://` because the certificate is verified
// against the ISRG root in acta.h.
#define ACTA_URL "https://acta.marending.dev/api/ingest"
#define ACTA_SENSOR_USER "sensor"
#define ACTA_SENSOR_PASSWORD "…"
