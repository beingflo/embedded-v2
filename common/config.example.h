// Local configuration for every sketch in this repository.
//
// Copy this file to `common/config.h` and fill in the values:
//
//     cp common/config.example.h common/config.h
//
// `config.h` is gitignored, so the credentials never enter the repository, and
// every sketch reads them from that single file. These are `#define`s, so
// the values are baked into the firmware image at compile time: changing one
// means reflashing every device that uses it.
//
// It holds two independent blocks: the reporting sketches (`co2-sensor`,
// `humidity-sensor`, `brightness-sensor`) need the ingest endpoint and the sensor
// credential, the epaper `dashboard` needs the display endpoint and the home
// credential. Leaving a block out only breaks the sketches that use it, so a
// machine that holds one credential can build that half.
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

// The epaper panel's picture, and the credential it reads with:
// `ACTA_AUTH_HOME_USER` and `ACTA_AUTH_HOME_PASSWORD` from the same deploy
// secrets. That role may only read the home readings, so the panel can show the
// dashboard and nothing else, and a stolen panel cannot write data either.
#define ACTA_DISPLAY_URL "https://acta.marending.dev/api/dashboard"
#define ACTA_HOME_USER "home"
#define ACTA_HOME_PASSWORD "…"
