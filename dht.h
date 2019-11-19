#ifndef DHT
#define DHT

// Struct for DHT22 data
typedef struct {
  double hum;
  double temp;
} dht22_data_t;

// Debug printing
#ifdef DEBUG
#define DEBUG_PRINT 1
#else
#define DEBUG_PRINT 0
#endif
#define debug_print(fd, fmt, ...) \
            do { if (DEBUG_PRINT) fprintf(fd, fmt, __VA_ARGS__); } while (0)

// Errors
#define NO_ERROR  0
#define ETIME     1 // Timeout
#define EDRIVER   2 // Driver failed to init
#define EPARITY   3 // Checksum failed
#define EINVAL    4 // Invalid argument

// Pin defines
#define DHT_PIN 4

// Defaults
#define RETRIES 30
#define NUM_BITS 40 // Sensor returns 40 bits of data
#define NUM_BYTES 5

// Timing defines
#define HOST_STARTSIG_LOW_TIME_US 1000 // Start signal, typical 1ms
#define HOST_STARTSIG_WAIT_TIME_US 25 // Wait time for response from sensor

#define SENSOR_PULL_WAIT_TIME_US 45 // Wait time for sensor to pull data line up/down
#define SENSOR_RESPONSE_WAIT_TIME_US 80 // Wait time for response signal

#define SENSOR_WAIT_TIME_CYCLES 10000
#define TIMEOUT_CYCLES 50000

#define SENSOR_COOLDOWN_TIME_US 500000 // Trial and error magic number to reset sensor

// Sets up BCM2835 driver
int dht_init(int pin);

// Reads data from DHT22
int dht_read_data(int pin, int max_retries, dht22_data_t *out);

#endif