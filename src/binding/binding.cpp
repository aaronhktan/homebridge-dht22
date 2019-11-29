extern "C" {
#include "dht.h"
}

#include "binding_utils.h"

#include <napi.h>

Napi::Object getData(const Napi::CallbackInfo &info) {
  // Get arguments
  int pin = info[0].As<Napi::Number>();
  int retries = info[1].As<Napi::Number>();
  Napi::Env env = info.Env();

  // Variables to hold humidity and temperature
  double humidity, temperature;

  // Init pin
  int err = DHT_init(pin);
  if (err) {
    return BindingUtils::errFactory(env, err, "Could not initialize pin");
  }

  // Read data
  err = DHT_read_data(pin, retries, &humidity, &temperature);
  if (err) {
    return BindingUtils::errFactory(env, err, "Could not read data");
  }

  // Close pin
  DHT_deinit();

  // Put return values into an object
  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "temp"), Napi::Number::New(env, temperature));
  returnObject.Set(Napi::String::New(env, "hum"), Napi::Number::New(env, humidity));
  return returnObject;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "getData"),
              Napi::Function::New(env, getData));
  return exports;
}

NODE_API_MODULE(homebridgedht22, Init)