#include "dht.h"

#include "bcm2835.h"

#include <sched.h>
#include <sys/mman.h>

#include <stdio.h>
#include <string.h>

// The GPIO pin of the DHT22 connected to the Raspberry Pi
uint8_t pin = DHT_PIN;

// Poll pin for timeout_cycles until it changes to level or times out
// Returns 0 after pin changes to and then away from level,
// ETIME if timeout_cycles has passed without the level changing
static int level_or_error(uint8_t pin, uint16_t level,
                          uint32_t timeout_cycles) {
  for (int i = 0; i < timeout_cycles; i++) {
    if (bcm2835_gpio_lev(pin) == level) {
      while (bcm2835_gpio_lev(pin) == level) { }
      return 0;
    }
  }
  return ETIME;
}

// Counts number of cycles that pin is at level
// Returns number of cycles
static int level_cycles(uint8_t pin, uint16_t level) {
  int count = 0;
  while (bcm2835_gpio_lev(pin) == level && count < TIMEOUT_CYCLES) {
    ++count;
  }
  return count;
}

// Communicate with DHT22 to get data
// OUT: cycles, containing number of cycles at low and high
// Responsibility of caller to allocate/free 80-int array
static int dht_get_data(int pin, int *cycles) {
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
    return ETIME;
  }
  if (level_or_error(pin, HIGH, SENSOR_WAIT_TIME_CYCLES)) {
    debug_print(stderr, "%s\n", "Timed out waiting for sensor response high\n");
    return ETIME;
  }

  // Now, start reading data
  // There are 40 bits of data
  // The DHT22 transmits a bit with GPIO set low, then set high
  // So get the # of cycles that it's set low, then # of cycles set high
  for (int i = 0; i < 80; i += 2) {
    cycles[i] = level_cycles(pin, LOW);
    cycles[i+1] = level_cycles(pin, HIGH);
  }

  for (int i = 0; i < 80; i += 2) {
    debug_print(stdout, "%d: Low: %d; high: %d\n", i / 2, cycles[i], cycles[i+1]);
    if (cycles[i] == TIMEOUT_CYCLES || cycles[i+1] == TIMEOUT_CYCLES) {
      return ETIME;
    }
  }

  return NO_ERROR;
}

// Process data
// IN: cycles, containing number of cycles at low/high
// OUT: data, containing five uint8_t that are the bytes received
// It is the responsibility of the caller to manage both these arrays
static int dht_process_data(int *cycles, uint8_t *data) {
  // Low is always held for 50us, high is held for 28us or 70us
  // 28us means zero transmitted, 70us means one transmitted
  // So if the number of cycles at high < cycles at low,
  // the bit received is 0; otherwise, is 1

  // The DHT22 transmits 5 bytes of data
  // So shift each bit of data into the data array
  for (int i = 0; i < 40; i++) {
    data[i/8] <<= 1;
    data[i/8] |= (cycles[i*2+1] > cycles[i*2]) ? 1 : 0;
  }

  // Check the parity; parity is fifth byte of transmitted data
  if (!(data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
    debug_print(stderr, "%s\n", "Checksum failed\n");
    return EPARITY;
  }

  return NO_ERROR;
}

// Convert data
// IN: data, containing five uint8_t that are the bytes received
// OUT: struct to put converted temperature and humidity in
// It is the caller's responsibility to manage both the parameters
static int dht_convert_data(uint8_t *data, dht22_data_t *out) {
  // First byte of transmitted data is MSB of humidity
  // Second byte is LSB of humidity
  // Conversion is humidity / 10
  uint16_t rel_hum = (data[0] & 0xFF) << 8;
  rel_hum |= data[1];
  double converted_hum = rel_hum / 10.0;

  // Third byte of transmitted data is MSB of temperature
  // Fourth byte is LSB of temperature
  // Conversion is temp / 10
  int temp = (data[2] & 0xFF) << 8;
  temp |= data[3];
  double converted_temp = temp / 10.0;

  debug_print(stdout, "Relative humidity: %f\n", converted_hum);
  debug_print(stdout, "Temperature: %f\n", converted_temp);
  
  out->hum = converted_hum;
  out->temp = converted_temp;

  return NO_ERROR;
}

int dht_init(int pin) {
  if (!bcm2835_init()) {
    debug_print(stderr, "%s\n", "Couldn't init bcm2835!\n");
    return EDRIVER;
  }

  // Set up the pin
  bcm2835_gpio_set_pud(pin, BCM2835_GPIO_PUD_UP); // Sets as having pull-up resistor

  return 0;
}

// Reads data
int dht_read_data(int pin, int max_retries, dht22_data_t *out) {
  // Check for valid arguments
  if (!out) {
    return EINVAL;
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

  do {
    err = NO_ERROR;
    memset(cycles, 0, sizeof(cycles));
    err |= dht_get_data(pin, cycles);

    if (err) {
      usleep(SENSOR_COOLDOWN_TIME_US);
      retries++;
      continue;
    }

    debug_print(stdout, "%s\n", "Got data from device!\n");

    memset(data, 0, sizeof(data));
    err |= dht_process_data(cycles, data);
    if (err) {
      usleep(SENSOR_COOLDOWN_TIME_US);
      retries++;
      continue;
    }

    debug_print(stdout, "%s\n", "Processed data from device!\n");
  }  while (retries < max_retries && err);

  if (err) {
    return err;
  }

  dht_convert_data(data, out);

  return NO_ERROR;
}
