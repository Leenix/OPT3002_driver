#include "OPT3002.h"

/**
 * Set the address of the sensor.
 * The address is set with hardware, depending on the configuration of the ADDR pin.
 *
 * ADDR -> GND = 0x44
 * ADDR -> VDD = 0x45
 * ADDR -> SDA = 0x46
 * ADDR -> SCL = 0x47
 *
 * @param address: Address to set the driver to.
 */
void OPT3002::set_address(uint8_t address) {
    // Restrict the input address to the valid range
    address |= 0b1000100;
    address = address & 0b1000111;
    _device_address = address;
}

/**
 * Write a value to a register using I2C
 *
 * @param input: Byte to write to the register.
 * @param address: Address of register to write to.
 * @return: Success/error result of the write.
 */
bool OPT3002::write(uint8_t *input, opt3002_reg_t address) {
    bool result = true;
    Wire.beginTransmission(_device_address);
    Wire.write(address);
    Wire.write(input[1]);
    Wire.write(input[0]);

    if (Wire.endTransmission() != 0) {
        result = false;
    }
    return result;
}

/**
 * Read a specified number of bytes using the I2C bus.
 * @param output: The buffer in which to store the read values.
 * @param address: Register address to read (or starting address in burst reads)
 * @param length: Number of bytes to read.
 */
bool OPT3002::read(uint8_t *output, opt3002_reg_t address) {
    bool result = true;
    Wire.beginTransmission(_device_address);
    Wire.write(address);
    if (Wire.endTransmission() != 0)
        result = false;

    else  // OK, all worked, keep going
    {
        Wire.requestFrom(_device_address, 2);
        for (size_t i = 0; (i < 2) and Wire.available(); i++) {
            uint8_t c = Wire.read();
            output[1 - i] = c;
        }
    }
    return result;
}

/**
 *
 */
void OPT3002::write(opt3002_config_t config) { write((uint8_t *)&config, OPT3002_REGISTER::CONFIG); }
void OPT3002::read(opt3002_config_t &config) { read((uint8_t *)&config, OPT3002_REGISTER::CONFIG); }

/**
 *
 */
opt3002_config_t OPT3002::get_config() {
    opt3002_config_t current_config;
    this->read(current_config);
    return current_config;
}

/**
 * Check that things work // TODO - documentation
 */
bool OPT3002::check_comms() {
    uint8_t buffer[2];
    read(buffer, OPT3002_REGISTER::MANUFACTURER_ID);
    uint16_t manufacturer_id = uint16_t(buffer[1]) << 8 | buffer[0];

    // Make sure the manufacturer's ID matches the expected value ('TI')
    bool success = false;
    if (manufacturer_id == OPT3002_MANUFACTURER_ID) success = true;

    return success;
}

/**
 * Calculate the optical power measured by the sensor.
 * @return: Optical power of incident light in nW/cm^2
 */
uint32_t OPT3002::get_optical_power() {
    opt3002_result_t result;
    read((uint8_t *)&result, OPT3002_REGISTER::RESULT);

    float optical_power = convert_measurement(result);
    return (uint32_t)optical_power;
}

bool OPT3002::begin(uint8_t address) {
    set_address(address);
    return check_comms();
}

void OPT3002::set_high_limit(opt3002_result_t high_limit) { write((uint8_t *)&high_limit, OPT3002_REGISTER::HIGH_LIMIT); }
void OPT3002::set_high_limit(float high_limit) { set_high_limit(convert_measurement(high_limit)); }

opt3002_result_t OPT3002::get_high_limit() {
    opt3002_result_t limit;
    read((uint8_t *)&limit, OPT3002_REGISTER::HIGH_LIMIT);
    return limit;
}

void OPT3002::set_low_limit(opt3002_result_t low_limit) { write((uint8_t *)&low_limit, OPT3002_REGISTER::LOW_LIMIT); }
void OPT3002::set_low_limit(float low_limit) { set_low_limit(convert_measurement(low_limit)); }

/**
 * Get the low limit level from the sensor.
 * The default low-level after reset is 0.
 */
opt3002_result_t OPT3002::get_low_limit() {
    opt3002_result_t limit;
    read((uint8_t *)&limit, OPT3002_REGISTER::LOW_LIMIT);
    return limit;
}

float OPT3002::convert_measurement(opt3002_result_t input) {
    // Calculate optical power [ref: Equation 1, OPT3002 Datasheet]
    // Optical_Power = R[11:0] * 2^(E[3:0]) * 1.2 nW/cm^2
    uint32_t exponent = 1 << input.exponent;
    uint32_t output = input.reading * exponent * 1.2;
    return output;
}

opt3002_result_t OPT3002::convert_measurement(float input) {
    uint8_t exponent = 0;
    uint16 fractional = input / 1.2;
    while (fractional >= (1 << 12) and exponent < (1 << 4)) {
        fractional /= 2;
        exponent++;
    }
    opt3002_result_t output;
    output.exponent = exponent;
    output.reading = fractional;

    return output;
}
