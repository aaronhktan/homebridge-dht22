const DHT22 = require('bindings')('homebridge-dht22');

console.log(DHT22.getData(4, 50));
