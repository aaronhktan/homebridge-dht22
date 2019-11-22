#include "dht.h"

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"

int main(int argc, char **argv) {
  int pin = DHT_PIN;
  int retries = RETRIES;

  // Get argument for pin
  int c;
  while ((c = getopt(argc, argv, "p:r:")) != -1) {
    switch (c) {
      case 'p':
        pin = atoi(optarg);
        break;
      case 'r':
        retries = atoi(optarg);
        break;
    }
  }

  double humidity, temperature;

  DHT_init(pin);
  int err = DHT_read_data(pin, retries, &humidity, &temperature);
  if (err) {
    printf("Couldn't read temperature!\n");
  }
  printf("Relative humidity: %f\n", humidity);
  printf("Temperature: %f\n", temperature);
}