#include "OPT3002.h"

OPT3002 sensor;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting up OPT3002...");

    Wire.begin();

    // Set configuration parameters.
    sensor.config.conversion_time = OPT3002_CONVERSION_TIME::ILLU_800MS;
    sensor.config.mode = OPT3002_MODE::CONTINUOUS;
    sensor.config.range = OPT3002_RANGE::OPT3002_RANGE_AUTO;
    sensor.begin();
}

void loop() {
    uint32_t reading = sensor.get_optical_power();
    Serial.print("Reading: ");
    Serial.print(reading);
    Serial.println(" nW/cm2");
    delay(1000);
}
