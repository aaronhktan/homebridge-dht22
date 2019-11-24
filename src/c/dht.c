#include "dht.h"

#include "bcm2835.h"

#include <sched.h>
#include <sys/mman.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

// Poll pin for timeout_cycles until it changes to level or times out
// Returns 0 after pin changes to and then away from level,
// ERROR_TIME if timeout_cycles has passed without the level changing
static int level_or_error(const uint8_t pin, const uint16_t level,
                          const uint32_t timeout_cycles) {
  for (int i = 0; i < timeout_cycles; i++) {
    if (bcm2835_gpio_lev(pin) == level) {
      while (bcm2835_gpio_lev(pin) == level) { }
      return 0;
    }
  }
  return ERROR_TIME;
}

// Counts number of cycles that pin is at level
// Returns number of cycles
static int level_cycles(const uint8_t pin, const uint16_t level) {
  int count = 0;
  while (bcm2835_gpio_lev(pin) == level && count < TIMEOUT_CYCLES) {
    ++count;
  }
  return count;
}

// Communicate with DHT22 to get data
// OUT: cycles, containing number of cycles at low and high
// Responsibility of caller to allocate/free 80-int array
static int DHT_get_data(int pin, int *cycles) {
  // To start communicating, set GPIO low, then set GPIO high
  // Hold high for WAIT_TIME, then relinquish control to device
  bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_clr(pin);
  bcm2835_delayMicroseconds(HOST_STARTSIG_LOW_TIME_US);
  bcm2835_gpio_set(pin);
  bcm2835_delayMicroseconds(HOST_STARTSIG_WAIT_TIME_US);
  bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);

  // Sensor should respond with low then high to acknowledge
  // start of communication
  if (level_or_error(pin, LOW, SENSOR_WAIT_TIME_CYCLES)) {
    debug_print(stderr, "%s\n", "Timed out waiting for sensor response low\n");
    return ERROR_TIME;
  }
  if (level_or_error(pin, HIGH, SENSOR_WAIT_TIME_CYCLES)) {
    debug_print(stderr, "%s\n", "Timed out waiting for sensor response high\n");
    return ERROR_TIME;
  }

  // Now, start reading data
  // There are 40 bits of data
  // The DHT22 transmits a bit by setting GPIO low for some time, then set high
  // (See DHT_process_data for more details)
  // So get the # of cycles that it's set low, then # of cycles set high
  for (int i = 0; i < 80; i += 2) {
    cycles[i] = level_cycles(pin, LOW);
    cycles[i+1] = level_cycles(pin, HIGH);
  }

  // Return an error if any of the cycles timed out
  for (int i = 0; i < 80; i += 2) {
    debug_print(stdout, "%d: Low: %d; high: %d\n", i / 2, cycles[i], cycles[i+1]);
    if (cycles[i] == TIMEOUT_CYCLES || cycles[i+1] == TIMEOUT_CYCLES) {
      return ERROR_TIME;
    }
  }

  return NO_ERROR;
}

// Process data
// IN: cycles, containing number of cycles at low/high
// OUT: data, containing five uint8_t that are the bytes received
// It is the responsibility of the caller to manage both these arrays
static int DHT_process_data(const int *cycles, uint8_t *data) {
  // The device transmits a 0 by holding the line low for 50us, then high for 28us
  // The device transmits a 1 by holding the line low for 50us, then high for 70us
  // So if the number of cycles at high < cycles at low,
  // the bit received is 0; otherwise, is 1

  // The DHT22 transmits 5 bytes of data
  // Shift each bit of data into the data array
  for (int i = 0; i < 40; i++) {
    data[i/8] <<= 1;
    data[i/8] |= (cycles[i*2+1] > cycles[i*2]) ? 1 : 0;
  }

  // Check the parity; parity is fifth byte of transmitted data
  if (!(data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
    debug_print(stderr, "%s\n", "Checksum failed\n");
    return ERROR_PARITY;
  }

  return NO_ERROR;
}

// Convert data
// IN: data, containing five uint8_t that are the bytes received
// OUT: doubles for temperature and humidity
// It is the caller's responsibility to manage all memory
static int DHT_convert_data(const uint8_t *data,
                            double *hum_out, double *temp_out) {
  // First byte of transmitted data is MSB of humidity
  // Second byte is LSB of humidity
  // Conversion is humidity / 10
  uint16_t rel_hum = (data[0] & 0xFF) << 8;
  rel_hum |= data[1];
  *hum_out = rel_hum / 10.0;

  // Third byte of transmitted data is MSB of temperature
  // Fourth byte is LSB of temperature
  // Conversion is temp / 10
  uint16_t temp = (data[2] & 0xFF) << 8;
  temp |= data[3];
  *temp_out = temp / 10.0;

  debug_print(stdout, "Relative humidity: %f\n", *hum_out);
  debug_print(stdout, "Temperature: %f\n", *temp_out);

  return NO_ERROR;
}

// Sets up the BCM2835 driver and GPIO pin
int DHT_init(const int pin) {
  if (!bcm2835_init()) {
    debug_print(stderr, "%s\n", "Couldn't init bcm2835!\n");
    return ERROR_DRIVER;
  }

  // Set up the pin as having a pull-up resistor
  bcm2835_gpio_set_pud(pin, BCM2835_GPIO_PUD_UP);

  return 0;
}

// Gets data from device
int DHT_read_data(const int pin, const int max_retries,
                  double *humidity, double *temperature) {
  // Check for valid arguments
  if (!humidity || !temperature) {
    return ERROR_INVAL;
  }

  // Prevent swapping
  struct sched_param sp;
  memset(&sp, 0, sizeof(sp));
  sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
  sched_setscheduler(0, SCHED_FIFO, &sp);
  mlockall(MCL_CURRENT | MCL_FUTURE);

  int retries = 0;
  int err = NO_ERROR;
  int cycles[NUM_BITS * 2];
  uint8_t data[NUM_BYTES];

  // Try to contact device until we've exceeded max_retries
  do {
    // Reset err and cycles, and fetch info from device
    err = NO_ERROR;
    memset(cycles, 0, sizeof(cycles));
    err |= DHT_get_data(pin, cycles);

    if (err) {
      struct timespec sleep_time = {0, SENSOR_COOLDOWN_TIME_NS};
      nanosleep(&sleep_time, NULL);
      retries++;
      continue;
    }

    debug_print(stdout, "%s\n", "Got data from device!\n");

    // Reset array of data, and process data
    memset(data, 0, sizeof(data));
    err |= DHT_process_data(cycles, data);

    if (err) {
      struct timespec sleep_time = {0, SENSOR_COOLDOWN_TIME_NS};
      nanosleep(&sleep_time, NULL);
      retries++;
      continue;
    }

    debug_print(stdout, "%s\n", "Processed data from device!\n");
  }  while (retries < max_retries && err);

  // Max retries exceeded and there is still an error, so return it
  if (err) {
    return err;
  }

  // Convert the data and put it in the out structure
  DHT_convert_data(data, humidity, temperature);

  return NO_ERROR;
}
