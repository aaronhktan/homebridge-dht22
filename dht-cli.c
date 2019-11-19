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

  dht_init(pin);
  dht22_data_t data;
  int err = dht_read_data(pin, retries, &data);
  if (!err) {
    printf("Relative humidity: %f\n", data.hum);
    printf("Temperature: %f\n", data.temp);
  }
}