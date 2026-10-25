// Shared upload path for the sensors: WiFi bring-up and one POST per reading to
// acta's `/api/ingest`.
//
// Header-only on purpose. The Arduino build only compiles sources inside the
// sketch folder, so anything shared between sketches has to live in a header;
// every function here is `inline`, so including it from several sketches cannot
// produce duplicate symbols.
//
// The endpoint takes a JSON array of `{source, timestamp?, data}`, where `source`
// is the series name acta stores and `data` is any JSON object. `timestamp` is
// deliberately left out: these devices have no clock, so acta stamps arrival
// time, which for an always-on sensor posting every 30 s is the measurement time
// to within a second.
#pragma once

#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <WiFi.h>

#if !__has_include("config.h")
#error "common/config.h is missing: cp common/config.example.h common/config.h and fill it in"
#endif
#include "config.h"

// ISRG Root X1, valid until 2035: the root acta's Caddy certificate chains to.
static const char ACTA_ROOT_CA[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n"
    "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
    "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n"
    "WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n"
    "ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n"
    "MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n"
    "h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n"
    "0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n"
    "A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+3u3dpq+sW\n"
    "T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n"
    "B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n"
    "B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n"
    "KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n"
    "OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n"
    "jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n"
    "qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n"
    "rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n"
    "HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n"
    "hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n"
    "ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n"
    "3BebYhtF8GaV0nxvwuo77x/Pz9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n"
    "NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n"
    "ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n"
    "TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n"
    "jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n"
    "oyi3B43njTOQ5yOf+1CceWxG4bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n"
    "4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n"
    "mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n"
    "emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n"
    "-----END CERTIFICATE-----\n";

// Bring WiFi up, waiting at most 15 s. Returns false on timeout, so the caller
// can skip the upload instead of hanging forever: the sketches used to spin in a
// `while (WiFi.status() != WL_CONNECTED)` loop with no way out.
inline bool connectWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }
  // Stations only: `WIFI_AP_STA` was there for ESP-NOW coexistence, which none
  // of the uploading sketches use.
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("[wifi] connecting");
  for (int i = 0; i < 75 && WiFi.status() != WL_CONNECTED; i++) {  // 75 × 200 ms
    delay(200);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[wifi] connect timed out");
    return false;
  }
  Serial.print("[wifi] connected ");
  Serial.println(WiFi.localIP());
  return true;
}

// One attempt: build `[{"source":…,"data":…}]`, POST it, log the outcome.
inline bool actaPostOnce(const char *source, const String &payload) {
  if (!connectWifi()) {
    return false;
  }

  NetworkClientSecure client;
  client.setCACert(ACTA_ROOT_CA);

  HTTPClient https;
  https.setTimeout(10000);
  if (!https.begin(client, ACTA_URL)) {
    Serial.println("[acta] could not open the connection");
    return false;
  }
  https.addHeader("Content-Type", "application/json");
  https.setAuthorization(ACTA_SENSOR_USER, ACTA_SENSOR_PASSWORD);

  String body;
  body.reserve(payload.length() + 64);
  body = "[{\"source\":\"";
  body += source;
  body += "\",\"data\":";
  body += payload;
  body += "}]";

  const int code = https.POST(body);
  // 202 is the accepted path; anything else carries a JSON `error` worth logging,
  // which is the difference between "401 wrong credential" and "400 bad row".
  const bool ok = code == 202;
  if (ok) {
    Serial.printf("[acta] %s accepted\n", source);
  } else if (code > 0) {
    Serial.printf("[acta] %s rejected: %d %s\n", source, code, https.getString().c_str());
  } else {
    Serial.printf("[acta] %s failed: %s\n", source, https.errorToString(code).c_str());
  }
  https.end();
  return ok;
}

// Two attempts, two seconds apart, with a fresh WiFi connection for the second.
inline bool actaPost(const char *source, const String &payload) {
  if (actaPostOnce(source, payload)) {
    return true;
  }
  Serial.println("[acta] retrying once");
  delay(2000);
  return actaPostOnce(source, payload);
}
