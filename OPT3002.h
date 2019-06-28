#include <Arduino.h>
#include <Wire.h>

const uint8_t OPT3002_DEFAULT_ADDRESS = 0x44;
const uint8_t OPT3002_MANUFACTURER_ID = 0x5449;

/**
 * Register addresses of the sensor.
 */
typedef enum OPT3002_REGISTER {
    ILLU_RESULT = 0x00,
    ILLU_CONFIG = 0x01,
    ILLU_LOW_LIMIT = 0x02,
    ILLU_HIGH_LIMIT = 0x03,
    ILLU_MANUFACTURER_ID = 0x7E
} opt3002_reg_t;

/**
 * Operation modes of the sensor
 */
typedef enum OPT3002_MODE {
    SHUTDOWN = 0b00,     // Default - low power state
    SINGLE_SHOT = 0b01,  // Shut down after a signle conversion
    CONTINUOUS = 0b10    // Continuous conversions
} opt3002_mode_t;

/**
 * Conversion/integration time for the sensor.
 * Longer integration time allows for a lower noise measurement.
 * Short integration time can also limit the effective full-scale range of
 * the sensor's measurements.
 */
typedef enum OPT3002_CONVERSION_TIME {
    ILLU_100MS = 0,  // 100ms conversion time
    ILLU_800MS = 1,  // 800ms conversion time
} opt3002_conv_time_t;

/**
 * Interrupt mode of the sensor.
 * Interrupts can either be latched, requiring the sensor be manually read
 * to clear the interrupt state, or self-clearing once the triggering event
 * passes.
 *
 * Interrupts are caused by sensor measurements falling outside the set
 * low and high limits. Such instances are referred to as 'fault' events
 * in the sensor's datasheet.
 */
typedef enum OPT3002_INTERRUPT_MODE {
    OPT_INT_HYSTERESIS = 0,  // Self-clearing after triggering condition passes
    OPT_INT_LATCHED = 1,     // User-cleared interrupts
} opt3002_interrupt_mode_t;

/**
 * Polarity of the sensor's interrupts.
 * Can be active-high or active low.
 * Active-low interrupts require a pull-up resistor on the INT pin.
 */
typedef enum OPT3002_INTERRUPT_POLARITY {
    ACTIVE_LOW = 0,
    ACTIVE_HIGH = 1,
} opt3002_interrupt_polarity_t;

/**
 * The full-scale range of the sensor in nW/cm^2
 * Full-scale ranges have been approximated in the following labels.
 * Refer to the sensor datasheet for the exact ranges.
 */
typedef enum OPT3002_RANGE {
    OPT3002_RANGE_AUTO = 0b1100,
    OPT3002_RANGE_5K = 0,
    OPT3002_RANGE_10K = 1,
    OPT3002_RANGE_20K = 2,
    OPT3002_RANGE_40K = 3,
    OPT3002_RANGE_80K = 4,
    OPT3002_RANGE_160K = 5,
    OPT3002_RANGE_320K = 6,
    OPT3002_RANGE_640K = 7,
    OPT3002_RANGE_1M2 = 8,
    OPT3002_RANGE_2M5 = 9,
    OPT3002_RANGE_5M = 10,
    OPT3002_RANGE_10M = 11,
} opt3002_range_t;

/**
 * Number of 'faults' required to trigger an interrupt.
 * A fault is described as an instance of the measured signal
 * being outside the user-set low or high limits.
 */
typedef enum OPT3002_FAULT_CONFIG {
    OPT3002_FAULT_1 = 0,
    OPT3002_FAULT_2 = 1,
    OPT3002_FAULT_4 = 2,
    OPT3002_FAULT_8 = 3
} opt3002_fault_count_t;

/**
 * Configuration options for the sensor.
 * The contents of the opt3002_config_t type reflect that of the sensor's
 * configuration register, which is 16 bits wide.
 *
 * Some values in the configuration register are read-only and reflect
 * the state of the sensor, rather than control its operating characteristics.
 */
typedef union {
    struct {
        uint8_t fault_interrupt_count : 2;  // Number of measurements outside set levels required to trigger interrupt
        uint8_t mask_exponent : 1;          // Not sure...
        uint8_t interrupt_polarity : 1;     // Polarity of interrupt signal [active high, active low]
        uint8_t interrupt_mode : 1;         // Interrupt latch mode [transient, latched]
        uint8_t flag_low : 1;               // Read-only. 1: Conversion lower than user's low limit
        uint8_t flag_high : 1;              // Read-only. 1: Conversion larger than user's high limit
        uint8_t conversion_ready : 1;       // Read-only. 1: Conversion complete
        uint8_t overflow : 1;               // Read-only. 1: ADC overflow
        uint8_t mode : 2;                   // Conversion mode. [shutdown, single conversion, continuous conversion]
        uint8_t conversion_time : 2;        //[100, 800] ms conversion time
        uint8_t range : 4;                  // Full-scale range of measurements
    };
    uint16_t raw;
} opt3002_config_t;

/**
 * Result format of the sensor's measurements
 * Measurements are split into a fractional reading, and an exponent.
 * The optical power of a reading can be calculated as:
 *  OP = fractional_reading * 2^exponent * 1.2 [nW/cm^2]
 *
 * The same result format is used when setting the upper or lower level limits
 * of the sensor.
 */
typedef union {
    struct {
        uint16_t reading : 12;
        uint8_t exponent : 4;
    };
    uint16_t raw;
} opt3002_result_t;

/**
 * The driver for the OPT3002 illuminance sensor.
 */
class OPT3002 {
   public:
    // Soft-managed configuration to be written to the sensor
    opt3002_config_t config;

    // Start the sensor if comms work
    bool begin(uint8_t address = OPT3002_DEFAULT_ADDRESS);

    // Set the device i2c address of the sensor
    void set_address(uint8_t address);

    // Check that the controller is able to communicate with the sensor over i2c
    bool check_comms();

    // Apply the soft configuration to the sensor
    void apply_config();

    // Read the sensor's current configuration
    opt3002_config_t read_config();

    // Get the optical power of the sensor's latest measurement
    uint32_t get_optical_power();

    // Set the high limit for sensor measurements before faults occur
    void set_high_limit(opt3002_result_t high_limit);

    // Get the sensor's current high limit level
    opt3002_result_t get_high_limit();

    // Set the low limit for sensor measurements before faults occur
    void set_low_limit(opt3002_result_t low_limit);

    // Get the sensor's current low limit level
    opt3002_result_t get_low_limit();

   private:
    // I2C address of the sensor
    uint8_t device_address;

    // Read from the sensor's registers
    bool read(uint8_t *output, opt3002_reg_t address, uint8_t length = 2);

    // Write to the sensor's registers
    bool write(uint8_t *input, opt3002_reg_t address, uint8_t length = 2);
};