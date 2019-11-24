const DHT22 = require('bindings')('homebridge-dht22');
const moment = require('moment');
const os = require('os');

// Services represent a service provided by a device,
// Characteristics represent an aspect of that device
var Service, Characteristic;

module.exports = homebridge => {
  // Service and Characteristic from hap-nodejs
  Service = homebridge.hap.Service;
  Characteristic = homebridge.hap.Characteristic;

  // Name of plugin, name of accessory, constructor
  homebridge.registerAccessory("homebridge-dht22", "DHT22", DHTAccessory);
}

// DHT constructor
// Must instantiate: service, characteristics
// AccessoryInformation service, with manufacturer/model/serialnumber
// TemperatureSensor service, with current temperature and name
// HumiditySensor service, with current humidity and name
function DHTAccessory(log, config) {
  this.log = log;
  this.pin = config['pin'] || 4;
  this.maxRetries = config['maxRetries'] || 50;
  this.refreshPeriod = config['refresh'] || 60;

  // Override some information about the accessory
  let informationService = new Service.AccessoryInformation();
  informationService
    .setCharacteristic(Characteristic.Manufacturer, "Adafruit")
    .setCharacteristic(Characteristic.Model, "DHT22")
    .setCharacteristic(Characteristic.SerialNumber, `${os.hostname}-${this.pin}`)
    .setCharacteristic(Characteristic.FirmwareRevision, require('./package.json').version);

  // Add temperature and humidity sensor services
  let temperatureService = new Service.TemperatureSensor();
  let humidityService = new Service.HumiditySensor();

  // Make these services available to class
  this.informationService = informationService;
  this.temperatureService = temperatureService;
  this.humidityService = humidityService;

  // Periodically update the values
  this.refreshData();
  setInterval(() => this.refreshData(),
              this.refreshPeriod * 1000);
}

DHTAccessory.prototype.refreshData = function() {
  // Get data from the sensor
  let data;
  data = DHT22.getData(this.pin, this.maxRetries);

  // If error, set to error state
  if (data.hasOwnProperty('errcode')) {
    this.log(`${moment().format("YYYY-MM-DD HH:mm:ss")} Error: ${data.errmsg}`);
    // Updating a value with Error class sets status in HomeKit to 'Not responding'
    this.temperatureService.getCharacteristic(Characteristic.CurrentTemperature)
      .updateValue(Error(data.errmsg));
    this.humidityService.getCharacteristic(Characteristic.CurrentRelativeHumidity)
      .updateValue(Error(data.errmsg));
    return;
  }
  
  // Set temperature and humidity from what we polled
  // We could use the standard Homebridge way, callback(err, valueToSet)
  // But that would make the setInterval look weird
  // Instead, use service.getCharacteristic.updateValue() to set the values.
  this.log(`${moment().format("YYYY-MM-DD HH:mm:ss")} Temp: ${data.temp}, Hum: ${data.hum}`);
  this.temperatureService.getCharacteristic(Characteristic.CurrentTemperature)
    .updateValue(data.temp);
  this.humidityService.getCharacteristic(Characteristic.CurrentRelativeHumidity)
    .updateValue(data.hum);
 }

DHTAccessory.prototype.getServices = function() {
  return [this.informationService, this.temperatureService, this.humidityService];
}
