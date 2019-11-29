const DHT22 = require('bindings')('homebridge-dht22');

const moment = require('moment'); // Time formatting
const os = require('os'); // Hostname

// Services represent a service provided by a device,
// Characteristics represent an aspect of that service
var Service, Characteristic;

// Used for storing historical data
var FakeGatoHistoryService;

module.exports = homebridge => {
  Service = homebridge.hap.Service;
  Characteristic = homebridge.hap.Characteristic;
  FakeGatoHistoryService = require('fakegato-history')(homebridge);

  // Name of plugin, name of accessory, constructor
  homebridge.registerAccessory("homebridge-dht22", "DHT22", DHTAccessory);
}

// DHT constructor
// Must instantiate: service, characteristics
// AccessoryInformation service, with manufacturer/model/serialnumber
// TemperatureSensor service, with current temperature and name
// HumiditySensor service, with current humidity and name
function DHTAccessory(log, config) {
  // Basic setup from config file
  this.log = log;
  this.displayName = config['name'];
  this.pin = config['pin'] || 4; // Use BCM pin numbers
  this.refreshPeriod = config['refresh'] || 60;
  this.maxRetries = config['maxRetries'] || 50;
  this.maxTempDelta = config['maxTempDelta'] || 5;
  this.maxHumDelta = config['maxHumDelta'] || 5;
  this.tempOffset = config['tempOffset'] || 0;
  this.humOffset = config['humOffset'] || 0;
  this.fakeGatoStoragePath = config['fakeGatoStoragePath'];

  // Internal variables to keep track of current temperature and humidity
  this._currentTemp = null;
  this._currentHum = null;

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

  // Start FakeGato for logging historical data
  this.fakeGatoHistoryService = new FakeGatoHistoryService("weather", this, {
      storage: 'fs',
      filename: `DHT22-${os.hostname}-${this.pin}.json`,
      folder: this.fakeGatoStoragePath
  });

  // Periodically update the values
  this.refreshData();
  setInterval(() => this.refreshData(),
              this.refreshPeriod * 1000);
}

// Getters and setters for temperature and humidity
Object.defineProperty(DHTAccessory.prototype, "temp", {
  set: function(temperatureReading) {
    // Check if read data is out of bounds of max delta
    if (Math.abs(temperatureReading + this.tempOffset - this._currentTemperature)
          > this.maxTempDelta
        && this._currentTemperature) {
      this.log(`Error: Temperature reading out of max delta: ` +
               `Reading: ${temperatureReading}, Current: ${this._currentTemperature}`);
      return;
    }

    // If it's not, then update the value of the characteristic
    this._currentTemperature = temperatureReading + this.tempOffset;
    this.temperatureService.getCharacteristic(Characteristic.CurrentTemperature)
      .updateValue(this._currentTemperature);
    this.fakeGatoHistoryService.addEntry({
      time: moment().unix(),
      temp: this._currentTemperature,
    });
  },

  get: function() {
    return this._currentTemperature;
  }
});

Object.defineProperty(DHTAccessory.prototype, "hum", {
  set: function(humidityReading) {
    if (Math.abs(humidityReading + this.humOffset - this._currentHumidity)
          > this.maxHumDelta
        && this._currentHumidity) {
      this.log(`Error: Humidity reading out of max delta: ` + 
               `Reading: ${humidityReading}, Current: ${this._currentHumidity}`);
      return;
    }

    this._currentHumidity = humidityReading + this.humOffset;
    this.humidityService.getCharacteristic(Characteristic.CurrentRelativeHumidity)
      .updateValue(this._currentHumidity);
    this.fakeGatoHistoryService.addEntry({
      time: moment().unix(),
      humidity: this._currentHumidity,
    });
  },

  get: function() {
    return this._currentHumidity;
  }
});


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
  this.log(`${moment().format("YYYY-MM-DD HH:mm:ss")} Temp: ${data.temp}, Hum: ${data.hum}`);
  this.temp = data.temp;
  this.hum = data.hum;
 }

DHTAccessory.prototype.getServices = function() {
  return [this.informationService,
          this.temperatureService,
          this.humidityService,
          this.fakeGatoHistoryService];
}
