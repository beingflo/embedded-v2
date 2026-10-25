// Find which I2C pins the SCD30 actually answers on.
//
// `Wire`'s default pins come from the *board definition*, and the answer is not
// the same board to board: the generic ESP32-C6 variant wires it to SDA=23 /
// SCL=22, the XIAO ESP32C6 to 22/23, and the DFRobot FireBeetle 2 ESP32-C6 to
// **19/20**. Picking the wrong FQBN therefore leaves a correctly wired sensor
// invisible ("Sensor not detected!").
//
//   ./flash.sh i2c-scan && ./monitor.sh
//
// It needs no credentials and no sensor library: just Wire.
//
// GPIO12/13 are the ESP32-C6's native USB pins. Driving them as an I2C bus drops
// the USB device off the host for as long as the firmware keeps that muxing, so
// they are deliberately not in the list below — the same goes for 18/19 on C3/S3.

#include <Wire.h>

#define SCD30_ADDRESS 0x61

struct Pair {
  int sda;
  int scl;
};

// Most likely first: FireBeetle 2 C6 (19/20), XIAO C6 (22/23), generic C6
// (23/22), classic ESP32 (21/22), then the remaining safe pairs.
static const Pair PAIRS[] = {
    {19, 20}, {22, 23}, {23, 22}, {21, 22}, {22, 21},
    {20, 21}, {18, 19}, {6, 7},   {8, 9},   {4, 5},
    {10, 11}, {14, 15}, {16, 17}, {2, 3},   {0, 1},
};

static bool present(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

// A bus that is not really driven — a stuck-low SDA, a pin shared with a crystal
// or a display line — answers *every* address, because the master reads the held
// line as an ACK. Requiring a known-absent address to NACK is what separates a
// real hit from that artefact; without it the scan reports the SCD30 on several
// pairs at once.
#define ABSENT_ADDRESS 0x7F

static bool usableBus() {
  return !present(ABSENT_ADDRESS);
}

// A full sweep, so a device at an unexpected address still shows up.
static void sweep(int sda, int scl) {
  Wire.end();
  Wire.begin(sda, scl, 100000);
  Wire.setTimeOut(50);

  if (!usableBus()) {
    Serial.printf("[i2c] SDA=%d SCL=%d -> bus stuck low, skipping\n", sda, scl);
    return;
  }
  for (uint8_t address = 0x08; address <= 0x77; address++) {
    if (present(address)) {
      Serial.printf("[i2c] SDA=%d SCL=%d -> device at 0x%02X\n", sda, scl, address);
    }
  }
}

void setup() {
  Serial.begin(921600);
  delay(2000);  // the USB CDC endpoint needs a moment before the monitor attaches

  Serial.println();
  Serial.printf("[i2c] this build's board definition says SDA=%d SCL=%d\n", SDA, SCL);

  Serial.println("[i2c] sweeping every address on those default pins");
  sweep(SDA, SCL);

  Serial.println("[i2c] probing 0x61 on the other pin pairs");
  bool found = false;
  for (size_t i = 0; i < sizeof(PAIRS) / sizeof(PAIRS[0]); i++) {
    Wire.end();
    if (!Wire.begin(PAIRS[i].sda, PAIRS[i].scl, 100000)) {
      continue;
    }
    Wire.setTimeOut(50);
    if (usableBus() && present(SCD30_ADDRESS)) {
      Serial.printf("[i2c] SCD30 at 0x61 with SDA=%d SCL=%d\n", PAIRS[i].sda, PAIRS[i].scl);
      found = true;
    }
  }

  if (found) {
    Serial.println("[i2c] done: pick the board definition with that pair, or pass it");
    Serial.println("      explicitly: Wire.begin(sda, scl) before scd30.begin()");
  } else {
    Serial.println("[i2c] done: no SCD30 on any pair. Check 3V3, ground and the");
    Serial.println("      pull-ups — the sensor does not idle the bus high on its own.");
  }
}

void loop() {
  delay(10000);
  Serial.println("[i2c] flash the sensor sketch again once you know the pair");
}
