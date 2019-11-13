#include "bcm2835.h"
#include "dht.h"

#include <stdio.h>
#include <string.h>

// uint8_t init = NO_ACTION;
uint8_t pin = DHT_PIN;

// Poll pin for timeout_cycles until it changes to level or times out
// Returns 0 if level is found, ETIME otherwise
int level_or_error(uint8_t pin, uint16_t level, uint32_t timeout_cycles) {
  for (int i = 0; i < timeout_cycles; i++) {
    if (bcm2835_gpio_lev(pin) == level) {
      while (bcm2835_gpio_lev(pin) == level) {
      }
      return 0;
    }
  }
  return ETIME;
}

// Counts number of cycles that pin is at level
// Returns number of cycles
int level_cycles(uint8_t pin, uint16_t level) {
  int count = 0;
  while (bcm2835_gpio_lev(pin) == level && count < 50000) {
    ++count;
  }
  return count;
}

int main(int argc, char **argv) {
  // Start up BCM2835 driver
  if (!bcm2835_init()) {
    fprintf(stderr, "Couldn't init bcm2835!\n");
    return EDRIVER;
  }

  // Create array to hold data
  int cycles[80];

  // Set up Pin 7 (GPIO pin 4)
  bcm2835_gpio_set_pud(pin, BCM2835_GPIO_PUD_UP); // Sets as having pull-up resistor
  bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP); // Sets as being output

  // To start communicating, set GPIO low, then reset high
  //bcm2835_gpio_set(pin);
  //usleep(HOST_STARTSIG_WAIT_TIME_US);
  bcm2835_gpio_clr(pin);
  usleep(HOST_STARTSIG_LOW_TIME_US);

  // Set pin to input mode to start reading
  // Since pin is configured to be pull up, sets the pin high
  // Delay by HOST_STARTSIG_WAIT_TIME_US to let sensor respond
  bcm2835_gpio_set(pin);
  usleep(HOST_STARTSIG_WAIT_TIME_US);
  bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);

  // Sensor responds with low, then high to start communication
  if (level_or_error(pin, LOW, SENSOR_RESPONSE_WAIT_TIME_US)) {
    fprintf(stderr, "Timed out waiting for sensor response low\n");
    return ETIME;
  }
  if (level_or_error(pin, HIGH, 1000000)) {
    fprintf(stderr, "Timed out waiting for sensor response high\n");
    return ETIME;
  }

  // Now, start reading data
  // Every bit is transmitted with low pulse, then high pulse
  // First, get number of cycles in the initial low pulse
  // Then, get the number of cycles in the high pulse
  for (int i = 0; i < 80; i += 2) {
    cycles[i] = level_cycles(pin, LOW);
    cycles[i+1] = level_cycles(pin, HIGH);
  }

  for (int i = 0; i < 80; i += 2) {
    printf("%d: Low: %d; high: %d\n", i / 2, cycles[i], cycles[i+1]);
  }

  printf("Data read!\n");

  // Low pulse is always 50us, high pulse is 28us or 70us
  // 28us means zero transmitted, 70us means one transmitted
  // So if the number of cycles at high < cycles at low, data is 0
  // Otherwise, is 1
  uint8_t data[5];
  memset(data, 0, sizeof(data));

  for (int i = 0; i < 40; i++) {
    data[i/8] <<= 1;
    data[i/8] |= (cycles[i*2+1] > cycles[i*2]) ? 1 : 0;
  }

  // Check parity
  if (!(data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
    printf("Checksum failed\n");
    return -1;
  }

  printf("Checksum succeeded!\n");
  for (int i = 0; i < 5; i++) {
    printf("data[%d]: 0x%x\n", i, data[i]);
  }

  int rel_hum = (data[0] & 0xFF) << 8;
  rel_hum |= data[1];

  double converted_hum = rel_hum / 10.0;
  printf("Relative humidity: %f\n", converted_hum);

  int temp = (data[2] & 0xFF) << 8;
  temp |= data[3];

  double converted_temp = temp / 10.0;
  printf("Temperature: %f\n", converted_temp);
  return 0;
}