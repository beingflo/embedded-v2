// Shared acta path for the sketches: WiFi bring-up, one POST per reading to
// acta's `/api/ingest`, and one GET for the epaper picture.
//
// Header-only on purpose. The Arduino build only compiles sources inside the
// sketch folder, so anything shared between sketches has to live in a header;
// every function here is `inline`, so including it from several sketches cannot
// produce duplicate symbols.
//
// The ingest endpoint takes a JSON array of `{source, timestamp?, data}`, where
// `source` is the series name acta stores and `data` is any JSON object.
// `timestamp` is deliberately left out: these devices have no clock, so acta
// stamps arrival time, which for an always-on sensor posting every 30 s is the
// measurement time to within a second.
//
// The two directions are guarded separately, each by the config defines it
// actually reads: a reporting sketch needs `ACTA_URL` and the sensor credential,
// the panel needs `ACTA_DISPLAY_URL` and the home credential. One `config.h`
// serves both, but a machine that only holds one credential can still build the
// sketches that use it — and a sketch that asks for the other one fails at its
// own call site instead of on an undefined macro somewhere in here.
#pragma once

#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <WiFi.h>

#if !__has_include("config.h")
#error "common/config.h is missing: cp common/config.example.h common/config.h and fill it in"
#endif
#include "config.h"

// ISRG roots, both valid to 2035. acta's Caddy chain ends at X1 for RSA leaves
// and at X2 for the ECDSA intermediates Let's Encrypt now issues by default; the
// X1 cross-signature of X2 expired in September 2024, so a client trusting only
// X1 fails outright when the server presents an X2-anchored chain. Shipping both
// costs 800 bytes and removes the guesswork.
static const char ACTA_ROOT_CA[] =
    "-----BEGIN CERTIFICATE-----\n"
    "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\n"
    "-----END CERTIFICATE-----\n"
    "-----BEGIN CERTIFICATE-----\n"
    "MIICGzCCAaGgAwIBAgIQQdKd0XLq7qeAwSxs6S+HUjAKBggqhkjOPQQDAzBPMQsw\n"
    "CQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJuZXQgU2VjdXJpdHkgUmVzZWFyY2gg\n"
    "R3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBYMjAeFw0yMDA5MDQwMDAwMDBaFw00\n"
    "MDA5MTcxNjAwMDBaME8xCzAJBgNVBAYTAlVTMSkwJwYDVQQKEyBJbnRlcm5ldCBT\n"
    "ZWN1cml0eSBSZXNlYXJjaCBHcm91cDEVMBMGA1UEAxMMSVNSRyBSb290IFgyMHYw\n"
    "EAYHKoZIzj0CAQYFK4EEACIDYgAEzZvVn4CDCuwJSvMWSj5cz3es3mcFDR0HttwW\n"
    "+1qLFNvicWDEukWVEYmO6gbf9yoWHKS5xcUy4APgHoIYOIvXRdgKam7mAHf7AlF9\n"
    "ItgKbppbd9/w+kHsOdx1ymgHDB/qo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0T\n"
    "AQH/BAUwAwEB/zAdBgNVHQ4EFgQUfEKWrt5LSDv6kviejM9ti6lyN5UwCgYIKoZI\n"
    "zj0EAwMDaAAwZQIwe3lORlCEwkSHRhtFcP9Ymd70/aTSVaYgLXTWNLxBo1BfASdW\n"
    "tL4ndQavEi51mI38AjEAi/V3bNTIZargCyzuFJ0nN6T5U6VR5CmD1/iQMVtCnwr1\n"
    "/q4AaOeMSQ+2b1tbFfLn\n"
    "-----END CERTIFICATE-----\n"
;

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

// Host part of `url`, so a failure can say what actually happened to the name.
inline String actaHost(const char *url) {
  String full = url;
  const int scheme = full.indexOf("://");
  const int start = scheme < 0 ? 0 : scheme + 3;
  const int slash = full.indexOf('/', start);
  String host = full.substring(start, slash < 0 ? full.length() : slash);
  const int colon = host.indexOf(':');
  return colon < 0 ? host : host.substring(0, colon);
}

// Printed once per boot and per endpoint, and again on every failure:
// "connection refused" from the ESP32 client covers both a refused port and a
// name that did not resolve, so the address is what turns the message into a
// diagnosis.
inline void actaLogTarget(const char *url) {
  static const char *logged = nullptr;
  if (logged != nullptr && strcmp(logged, url) == 0) {
    return;
  }
  logged = url;
  Serial.printf("[acta] endpoint %s\n", url);
  Serial.printf("[acta] local %s gw %s dns %s\n", WiFi.localIP().toString().c_str(),
                WiFi.gatewayIP().toString().c_str(), WiFi.dnsIP().toString().c_str());
}

// --- reporting -------------------------------------------------------------
// Only compiled into sketches that have somewhere to post to and a credential to
// post with; see the guards in config.h's documentation.
#if defined(ACTA_URL) && defined(ACTA_SENSOR_USER) && defined(ACTA_SENSOR_PASSWORD)

// One attempt: build `[{"source":…,"data":…}]`, POST it, log the outcome.
inline bool actaPostOnce(const char *source, const String &payload) {
  if (!connectWifi()) {
    return false;
  }
  actaLogTarget(ACTA_URL);

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
    // HTTPClient folds every failed connect into "connection refused", TLS
    // aborts included, so the mbedTLS reason and the clock go out with it: a
    // chain that does not validate and a 1970 clock both land here.
    char tls[128] = {0};
    const int tls_error = client.lastError(tls, sizeof(tls));
    IPAddress ip;
    const String host = actaHost(ACTA_URL);
    const bool resolved = WiFi.hostByName(host.c_str(), ip);
    Serial.printf("[acta] %s failed: %s (%s), tls=%d %s, time=%ld\n", source,
                  https.errorToString(code).c_str(),
                  resolved ? ip.toString().c_str() : "does not resolve", tls_error, tls,
                  (long)time(nullptr));
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

#endif  // reporting

// --- displaying ------------------------------------------------------------
// Only compiled into sketches that have a picture to fetch and a credential to
// fetch it with.
#if defined(ACTA_DISPLAY_URL) && defined(ACTA_HOME_USER) && defined(ACTA_HOME_PASSWORD)

// One picture from acta, for the sketch that displays instead of reporting.
//
// Reads the body into a caller-owned buffer and returns the byte count, or -1 on
// any failure. A body that is not exactly `size` bytes is refused rather than
// drawn: the panel's frame is a fixed 99 bytes times 272 rows, and half a frame
// would leave the previous picture's remains on the screen.
//
// The credential is a restricted one (`ACTA_AUTH_HOME_*` in acta), so a stolen
// panel only ever gets the home readings, and `*refreshSeconds` comes back from
// the `X-Refresh-Seconds` header so the interval can be changed without
// reflashing the device.
inline int actaFetch(const char *url, const char *user, const char *password, uint8_t *buffer,
                     size_t size, long *refreshSeconds) {
  if (!connectWifi()) {
    return -1;
  }
  actaLogTarget(url);

  NetworkClientSecure client;
  client.setCACert(ACTA_ROOT_CA);

  HTTPClient https;
  // Longer than a sensor's: the picture is drawn on request, so the service
  // spends a query and a render before the first byte leaves.
  https.setTimeout(20000);
  if (!https.begin(client, url)) {
    Serial.println("[acta] could not open the connection");
    return -1;
  }
  https.setAuthorization(user, password);
  const char *headers[] = {"X-Refresh-Seconds"};
  https.collectHeaders(headers, 1);

  const int code = https.GET();
  if (code != HTTP_CODE_OK) {
    if (code > 0) {
      // A refusal carries the reason in the body: 401 is the credential, 503 is
      // "no readings yet", and both are worth printing verbatim.
      Serial.printf("[acta] picture rejected: %d %s\n", code, https.getString().c_str());
    } else {
      // Same reasoning as the upload path: the client folds a refused connect
      // and a failed handshake into one message, so the TLS reason and the
      // resolved address go out with it.
      char tls[128] = {0};
      const int tls_error = client.lastError(tls, sizeof(tls));
      IPAddress ip;
      const String host = actaHost(url);
      const bool resolved = WiFi.hostByName(host.c_str(), ip);
      Serial.printf("[acta] picture failed: %s (%s), tls=%d %s, time=%ld\n",
                    https.errorToString(code).c_str(),
                    resolved ? ip.toString().c_str() : "does not resolve", tls_error, tls,
                    (long)time(nullptr));
    }
    https.end();
    return -1;
  }

  const long refresh = https.header("X-Refresh-Seconds").toInt();
  if (refreshSeconds != nullptr && refresh > 0) {
    *refreshSeconds = refresh;
  }

  const int length = https.getSize();
  if (length != (int)size) {
    Serial.printf("[acta] picture is %d bytes, panel frame is %u: not drawing it\n", length,
                  (unsigned)size);
    https.end();
    return -1;
  }

  WiFiClient *stream = https.getStreamPtr();
  size_t got = 0;
  unsigned long lastData = millis();
  while (got < size) {
    if (stream->available()) {
      got += stream->readBytes(buffer + got, size - got);
      lastData = millis();
    } else if (millis() - lastData > 10000) {
      Serial.println("[acta] download timed out");
      https.end();
      return -1;
    }
    delay(1);
  }
  https.end();

  Serial.printf("[acta] picture %u bytes\n", (unsigned)got);
  return (int)got;
}

#endif  // displaying
