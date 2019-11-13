
// Errors
#define NO_ERROR  0
#define ETIME     1 // Timeout
#define EDRIVER   2 // Driver failed to init

// Pin defines
#define DHT_PIN 4

// Timing defines
#define HOST_STARTSIG_LOW_TIME_US 1000 // Start signal, typical 1ms
#define HOST_STARTSIG_WAIT_TIME_US 30 // Wait time for response from sensor

#define SENSOR_PULL_WAIT_TIME_US 45 // Wait time for sensor to pull data line up/down
#define SENSOR_RESPONSE_WAIT_TIME_US 80 // Wait time for response signal

#define SENSOR_START_BIT_TIME_US 50 // Time sensor holds data line low to read
#define SENSOR_ZERO_BIT_TIME_US 28 // Time sensor holds data high to send zero
#define SENSOR_ONE_BIT_TIME_US 70 // Time sensor holds data high to send one

// Defines for reading data
#define NUM_BITS 40