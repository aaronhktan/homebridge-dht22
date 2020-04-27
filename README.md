# Homebridge Plugin for DHT22

This is a Homebridge plugin for DHT22 temperature and humidity sensor, working on the Raspberry Pi 3.

It uses the BCM2835 GPIO library.

<img src="/docs/eve.png?raw=true" style="margin: 5px"> <img src="/docs/home.png?raw=true" style="margin: 5px">

## Configuration

| Field name           | Description                                                   | Type / Unit    | Default value       | Required? |
| -------------------- |:--------------------------------------------------------------|:--------------:|:-------------------:|:---------:|
| name                 | Name of the accessory                                         | string         | —                   | Y         |
| pin                  | DHT22 data pin number, with BCM naming scheme                 | int            | 4                   | N         |
| refreshPeriod        | Time between each refresh of data                             | int / seconds  | 60                  | N         |
| maxRetries           | Number of times to retry if failed to fetch data from sensor  | int            | 50                  | N         |
| maxTempDelta         | Maximum allowable temperature change between each reading     | float / deg C  | 5                   | N         |
| maxHumDelta          | Maximum allowable humidity change between each reading        | float / %      | 5                   | N         |
| tempOffset           | Number of degrees C to add to each temperature reading        | float / deg C  | 0                   | N         |
| humOffset            | Percentage to add to each humidity reading                    | float / %      | 0                   | N         |
| enableFakeGato       | Enable storing data in Eve Home app                           | bool           | false               | N         |
| fakeGatoStoragePath  | Path to store data for Eve Home app                           | string         | (fakeGato default)  | N         |
| enableMQTT           | Enable sending data to MQTT server                            | bool           | false               | N         |
| mqttConfig           | Object containing some config for MQTT                        | object         | —                   | N         |

The mqttConfig object is **only required if enableMQTT is true**, and is defined as follows:

| Field name           | Description                                      | Type / Unit  | Default value       | Required? |
| -------------------- |:-------------------------------------------------|:------------:|:-------------------:|:---------:|
| url                  | URL of the MQTT server, must start with mqtt://  | string       | —                   | Y         |
| temperatureTopic     | MQTT topic to which temperature data is sent     | string       | dht22/temeprature   | N         |
| humidityTopic        | MQTT topic to which humidity data is sent        | string       | dht22/humidity      | N         |

### Example Configuration

```
{
  "bridge": {
    "name": "Homebridge",
    "username": "XXXX",
    "port": XXXX
  },

  "accessories": [
    {
      "accessory": "DHT22",
      "name": "Office DHT22",
      "pin": 4,
      "refresh": 60,
      "maxRetries": 50,
      "enableFakeGato": true,
      "enableMQTT": true,
      "mqtt": {
        "url": "mqtt://192.168.0.38",
        "temperatureTopic": "dht22/temperature",
        "humidityTopic": "dht22/humidity"
      }
    },
  ]
}
```

## Project Layout

- All things required by Node are located at the root of the repository (i.e. package.json and index.js).
- The rest of the code is in `src`, further split up by language.
  - `c` contains the C code that runs on the device to communicate with the sensor. It also contains a simple program to check that the sensor is connected and attempt to read from it.
  - `binding` contains the C++ code using node-addon-api to communicate between C and the Node.js runtime.
  - `js` contains a simple project that tests that the binding between C/Node.js is correctly working.
