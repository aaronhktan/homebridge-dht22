#ifndef BINDING_UTILS
#define BINDING_UTILS

#include <napi.h>

namespace BindingUtils {

// Generates an error object for Javascript
Napi::Object errFactory(const Napi::Env env,
                        const int errcode, const char *errmsg);

}

#endif
