//腾讯libco 寄存器交换相关逻辑使用需要使用的部分

#include "coroutine.h"

extern stCoRoutineEnv_t *env;
extern "C" {
    extern void context_init(coctx_t *) asm("context_init");
}

uintptr_t min_stack = 0;

void queue_work(uv_work_t *req){
    if(!co_list.empty() && req->data == NULL){
        coroutine_info* co_arg = co_list.front();
        co_list.pop_front();
        req->data = (void*) co_arg;
    }
    return;
}


void after_queue_work(uv_work_t *req, int status){
    if(env->iCallStackSize < 1){
        return;
    }

    HandleScope scope(globalIsolate);
    coroutine_info* co_arg = (coroutine_info*) req->data;
    env->pCallStack[ env->iCallStackSize++ ] = co_arg->coroutine;
    stCoRoutine_t* curr_co = env->pCallStack[ env->iCallStackSize - 2];
    stCoRoutine_t* pending_co = co_arg->coroutine;
    co_swap( curr_co, pending_co, 0);
    free(req);
    return;
}

void New(const FunctionCallbackInfo<Value> &args){
    if(!args.IsConstructCall()){
        globalIsolate->ThrowException(Exception::Error (String::NewFromUtf8(globalIsolate,"Coroutine must be called by a Constructor")));
        return;
    }
    Local<Object> self = args.This();
    loop = uv_default_loop();
    init_thread_env();
    stCoRoutine_t* coroutine;
    co_create(&coroutine,NULL,NULL,NULL);
    memset( &coroutine->ctx,0,sizeof(coctx_t));
    env->pCallStack[ env->iCallStackSize++ ] = coroutine;
    args.GetReturnValue().Set(self);
}

void NewCo(const FunctionCallbackInfo<Value> &args){
    Local<Object> self = args.This();
    Local<Boolean> coTag = Boolean::New(globalIsolate,true);
    PropertyDescriptor desc(coTag,false);
    desc.set_enumerable(false);
    desc.set_configurable(false);
    Local<Name> coName = String::NewFromUtf8(globalIsolate,"Coroutine");
    Local<Context> context = globalIsolate->GetCurrentContext();
    self->DefineProperty(context,coName,desc);
    args.GetReturnValue().Set(self);
}

void* coroutine_func( void * args){
    coroutine_info* co_arg = (coroutine_info*) args;
    co_list.push_back(co_arg);
    HandleScope scope(globalIsolate);
    Local<Value> params[1] = {Local<Value>::New(globalIsolate,co_arg->params)};
    Local<Function> callback = Local<Function>::New(globalIsolate,co_arg->cb);
    Local<Object> global =  globalIsolate->GetCurrentContext()->Global();
    uintptr_t limit = reinterpret_cast<uintptr_t>(&limit) - (co_arg->coroutine->ctx).ss_size;
    callback->Call(global,1,params);
    co_yield();
    return NULL;
}

void set_stack_limit(coctx_t* ctx){
    uintptr_t sp = reinterpret_cast<uintptr_t>(ctx->ss_sp);
    if( (min_stack != 0 && sp < min_stack) || min_stack == 0){
        min_stack = sp;
        globalIsolate->SetStackLimit(min_stack); //重设stack限制
    }
}

void create_coroutine(const FunctionCallbackInfo<Value> &args){
    HandleScope scope(globalIsolate);
    Local<FunctionTemplate> coft =
            FunctionTemplate::New(globalIsolate, NewCo);
    coft->InstanceTemplate()->SetInternalFieldCount(1);
    coft->SetClassName(v8::String::NewFromUtf8(globalIsolate, "CoObj"));
    Local<Object> coObject = coft->InstanceTemplate()->NewInstance();
    coroutine_info* co_arg = new coroutine_info;
    if(!args[0]->IsFunction()){
        globalIsolate->ThrowException(Exception::Error (String::NewFromUtf8(globalIsolate,"Callback must be a function!")));
        return;
    }
    Local<Function> callback = Local<Function>::Cast(args[0]);
    (co_arg->cb).Reset(globalIsolate,callback);
    co_create(&co_arg->coroutine,NULL,coroutine_func,(void*)co_arg);
    coctx_init(&co_arg->coroutine->ctx,coroutine_func,co_arg);
    set_stack_limit(&co_arg->coroutine->ctx);
    coObject->SetAlignedPointerInInternalField(0, co_arg);
    args.GetReturnValue().Set(coObject);
}

void swap_ctx(const FunctionCallbackInfo<Value> &args){
    HandleScope scope(globalIsolate);
    Local<Value> nextFunc = args[0];
    uv_work_t* req = (uv_work_t*) malloc(sizeof(uv_work_t));
    if(!nextFunc->IsUndefined()){
        Local<Object> yieldObj = Local<Object>::Cast(nextFunc);
        Local<Value> params = yieldObj->Get(v8::String::NewFromUtf8(globalIsolate, "arguments"));
        coroutine_info* co_arg = (coroutine_info*) yieldObj->GetAlignedPointerFromInternalField(0);
        (co_arg->params).Reset(globalIsolate,params);
        req->data = (void*) co_arg;
    }
    uv_queue_work(loop, req, queue_work, after_queue_work);
    co_yield();
}

void work_done(const FunctionCallbackInfo<Value> &args){
    if(args.Length() < 1 || args[0]->IsUndefined()){
        globalIsolate->ThrowException(Exception::Error (String::NewFromUtf8(globalIsolate,"must have a argument!")));
        return;
    }
    Local<Object> yieldObj = Local<Object>::Cast(args[0]);
    coroutine_info* co_arg = (coroutine_info*) yieldObj->GetAlignedPointerFromInternalField(0);
    co_list.remove(co_arg);
}

void destroy_coroutine(const FunctionCallbackInfo<Value> &args){
    if(args.Length() < 1 || args[0]->IsUndefined()){
        globalIsolate->ThrowException(Exception::Error (String::NewFromUtf8(globalIsolate,"must have a argument!")));
        return;
    }
    Local<Object> yieldObj = Local<Object>::Cast(args[0]);
    coroutine_info* co_arg = (coroutine_info*) yieldObj->GetAlignedPointerFromInternalField(0);
    co_list.remove(co_arg);
    (co_arg->cb).MarkIndependent();
    (co_arg->cb).SetWeak();
    (co_arg->params).MarkIndependent();
    (co_arg->params).SetWeak();
    free(co_arg->coroutine);
    delete co_arg;
    yieldObj->SetAlignedPointerInInternalField(0, NULL);
}