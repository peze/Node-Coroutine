//
// Created by hehe on 18/9/28.
//
#include "coroutine.h"

stCoRoutineEnv_t *env;
Isolate* globalIsolate;

void init(Local<Object> exports) {
    /*注册api函数*/

    globalIsolate = exports->GetIsolate();
    Local<FunctionTemplate> CoroutineClass = FunctionTemplate::New(globalIsolate, New);
    CoroutineClass->SetClassName(v8::String::NewFromUtf8(globalIsolate, "Coroutine"));
    v8::Local<ObjectTemplate> CoroutineClassPrototype = CoroutineClass->PrototypeTemplate();
    CoroutineClassPrototype->Set(String::NewFromUtf8(globalIsolate, "create"), FunctionTemplate::New(globalIsolate, create_coroutine));
    CoroutineClassPrototype->Set(String::NewFromUtf8(globalIsolate, "swap"), FunctionTemplate::New(globalIsolate, swap_ctx));
    CoroutineClassPrototype->Set(String::NewFromUtf8(globalIsolate, "done"), FunctionTemplate::New(globalIsolate, work_done));
    CoroutineClassPrototype->Set(String::NewFromUtf8(globalIsolate, "destroy"), FunctionTemplate::New(globalIsolate, destroy_coroutine));
    Local<ObjectTemplate> clientInstance = CoroutineClass->InstanceTemplate();
    clientInstance->SetInternalFieldCount(1);
    exports->Set(String::NewFromUtf8(globalIsolate, "Coroutine"), CoroutineClass->GetFunction());

}

NODE_MODULE(binding, init)