#ifndef NODE_COROUTINE_COROUTINE_H
#define NODE_COROUTINE_COROUTINE_H


#include "context.h"
#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>
#include <list>
using namespace v8;
using namespace node;
using namespace std;

extern Isolate* globalIsolate;

static struct coroutine_info{
    Persistent<Function> cb;
    stCoRoutine_t* coroutine;
    Persistent<Value> params;
    Persistent<Context> context;
};



static list<coroutine_info*> co_list;

static uv_loop_t* loop;



//func

void* coroutine_func( void * args);

void New(const FunctionCallbackInfo<Value> &args);

void create_coroutine(const FunctionCallbackInfo<Value> &args);

void swap_ctx(const FunctionCallbackInfo<Value> &args);

void destroy_coroutine(const FunctionCallbackInfo<Value> &args);

void work_done(const FunctionCallbackInfo<Value> &args);


#endif //NODE_COROUTINE_COROUTINE_H
