#ifndef DHT
#define DHT

// Print function that only prints if DEBUG is defined
#ifdef DEBUG
#define DEBUG_PRINT 1
#else
#define DEBUG_PRINT 0
#endif
#define debug_print(fd, fmt, ...) \
            do { if (DEBUG_PRINT) fprintf(fd, fmt, __VA_ARGS__); } while (0)

// Errors
enum Error {
  NO_ERROR,
  ERROR_TIME,        // Timeout
  ERROR_DRIVER,      // Driver failed to init
  ERROR_PARITY,      // Checksum failed
  ERROR_INVAL,       // Invalid argument
};

// Pin defines
#define DHT_PIN 4

// Defaults
#define RETRIES 30
#define NUM_BITS 40 // Sensor returns 40 bits of data
#define NUM_BYTES 5 // Which is 5 bytes

// Timing defines
#define HOST_STARTSIG_LOW_TIME_US 1000  // Start signal, typical 1ms
#define HOST_STARTSIG_WAIT_TIME_US 25   // Wait time for response from sensor, 20-30us

#define SENSOR_WAIT_TIME_US 45          // Wait time for sensor to pull data line up/down
#define SENSOR_WAIT_TIME_CYCLES 10000   // Number of iterations in loop to wait for sensor to pull data line up/down
#define TIMEOUT_CYCLES 50000            // Max number of iterations in loop to wait after sensor has pulled data line up/down

#define SENSOR_COOLDOWN_TIME_US 500000  // Trial and error magic number to reset sensor
#define SENSOR_COOLDOWN_TIME_NS 500000000

// Sets up BCM2835 driver
int DHT_init(const int pin);

// Reads data from DHT22
int DHT_read_data(const int pin,
                  const int max_retries,
                  double *humidity,
                  double *temperature);

#endif