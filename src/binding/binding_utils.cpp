#include <napi.h>

namespace BindingUtils {

Napi::Object errFactory(const Napi::Env env,
                        const int errcode, const char *errmsg) {
  Napi::Object errorObject = Napi::Object::New(env);
  errorObject.Set(Napi::String::New(env, "errcode"), Napi::Number::New(env, errcode));
  errorObject.Set(Napi::String::New(env, "errmsg"), Napi::String::New(env, errmsg));
  return errorObject;
}

}
