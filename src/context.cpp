//腾讯libco 寄存器交换相关逻辑使用需要使用的部分

#include "context.h"
#include <string.h>

extern stCoRoutineEnv_t *env;

#define ESP 0
#define EIP 1
#define EAX 2
#define ECX 3
// -----------
#define RSP 0
#define RIP 1
#define RBX 2
#define RDI 3
#define RSI 4

#define RBP 5
#define R12 6
#define R13 7
#define R14 8
#define R15 9
#define RDX 10
#define RCX 11
#define R8 12
#define R9 13


//----- --------
// 32 bit
// | regs[0]: ret |
// | regs[1]: ebx |
// | regs[2]: ecx |
// | regs[3]: edx |
// | regs[4]: edi |
// | regs[5]: esi |
// | regs[6]: ebp |
// | regs[7]: eax |  = esp
enum {
    kEIP = 0,
    kESP = 7,
};

//-------------
// 64 bit
//low | regs[0]: r15 |
//    | regs[1]: r14 |
//    | regs[2]: r13 |
//    | regs[3]: r12 |
//    | regs[4]: r9  |
//    | regs[5]: r8  |
//    | regs[6]: rbp |
//    | regs[7]: rdi |
//    | regs[8]: rsi |
//    | regs[9]: ret |  //ret func addr
//    | regs[10]: rdx |
//    | regs[11]: rcx |
//    | regs[12]: rbx |
//hig | regs[13]: rsp |
enum {
    kRDI = 7,
    kRSI = 8,
    kRETAddr = 9,
    kRSP = 13,
};

//64 bit
extern "C" {
extern void coctx_swap( coctx_t *,coctx_t*, int main) asm("coctx_swap");
};

int coctx_init( coctx_t *ctx, pfn_co_routine_t pfn,const void *s) {
#if defined(__i386__)
    char *sp = ctx->ss_sp + ctx->ss_size - sizeof(coctx_param_t);
	sp = (char*)((unsigned long)sp & -16L);


	coctx_param_t* param = (coctx_param_t*)sp ;
	param->s1 = s;

	memset(ctx->regs, 0, sizeof(ctx->regs));

	ctx->regs[ kESP ] = (char*)(sp) - sizeof(void*);
	ctx->regs[ kEIP ] = (char*)pfn;
#elif defined(__x86_64__)
    char *sp = ctx->ss_sp + ctx->ss_size;
    sp = (char*) ((unsigned long)sp & -16LL  );

    memset(ctx->regs, 0, sizeof(ctx->regs));

    ctx->regs[ kRSP ] = sp - 8;

    ctx->regs[ kRETAddr] = (char*)pfn;

    ctx->regs[ kRDI ] = (char*)s;
    return 0;
#endif
}



void init_thread_env() {
    env = (stCoRoutineEnv_t*)malloc( sizeof(stCoRoutineEnv_t) );
    env->iCallStackSize = 0;
    env->pending_co = NULL;
    env->occupy_co = NULL;
}

int co_create( stCoRoutine_t **pco,const stCoRoutineAttr_t *attr,pfn_co_routine_t pfn,void *arg ) {
    stCoRoutine_t* co;
    stCoRoutineAttr_t at;
    if( attr ) {
        memcpy( &at,attr,sizeof(at) );
    }
    if( at.stack_size <= 0 ) {
        at.stack_size = 128  * 1024 * 8;
    } else if( at.stack_size > 1024 * 1024 * 8 ) {
        at.stack_size = 1024 * 1024 * 8;
    }

    if( at.stack_size & 0xFFF ) {
        at.stack_size &= ~0xFFF;
        at.stack_size += 0x1000;
    }

    co = (stCoRoutine_t*)malloc( sizeof(stCoRoutine_t) );

    memset( co,0,(long)(sizeof(stCoRoutine_t)));


    co->env = env;
    co->pfn = pfn;
    co->arg = arg;

    stStackMem_t* stack_mem = NULL;
    if( at.share_stack ) {
        stack_mem = co_get_stackmem( at.share_stack);
        at.stack_size = at.share_stack->stack_size;
    } else {
        stack_mem = co_alloc_stackmem(at.stack_size);
    }
    co->stack_mem = stack_mem;

    co->ctx.ss_sp = stack_mem->stack_buffer;
    co->ctx.ss_size = at.stack_size;

    co->cStart = 0;
    co->cEnd = 0;
    co->cIsMain = 0;
    co->cEnableSysHook = 0;
    co->cIsShareStack = at.share_stack != NULL;

    co->save_size = 0;
    co->save_buffer = NULL;
    *pco = co;
    return 0;
}

void co_swap(stCoRoutine_t* curr, stCoRoutine_t* pending_co,int main) {
    //get curr stack sp
    char c;
    curr->stack_sp= &c;

    if (!pending_co->cIsShareStack) {
        env->pending_co = NULL;
        env->occupy_co = NULL;
    } else {
        env->pending_co = pending_co;
        //get last occupy co on the same stack mem
        stCoRoutine_t* occupy_co = pending_co->stack_mem->occupy_co;
        //set pending co to occupy thest stack mem;
        pending_co->stack_mem->occupy_co = pending_co;

        env->occupy_co = occupy_co;
        if (occupy_co && occupy_co != pending_co) {
            save_stack_buffer(occupy_co);
        }
    }

    //swap context
    coctx_swap(&(curr->ctx),&(pending_co->ctx),main);

    //stack buffer may be overwrite, so get again;
    stCoRoutine_t* update_occupy_co =  env->occupy_co;
    stCoRoutine_t* update_pending_co = env->pending_co;

    if (update_occupy_co && update_pending_co && update_occupy_co != update_pending_co) {
        //resume stack buffer
        if (update_pending_co->save_buffer && update_pending_co->save_size > 0) {
            memcpy(update_pending_co->stack_sp, update_pending_co->save_buffer, update_pending_co->save_size);
        }
    }
}

void co_yield() {
    if(env->iCallStackSize < 2){
        return;
    }
    stCoRoutine_t *last = env->pCallStack[ env->iCallStackSize - 2 ];
    stCoRoutine_t *curr = env->pCallStack[ env->iCallStackSize - 1 ];

    env->iCallStackSize--;

    co_swap( curr, last,1);
}

void co_free( stCoRoutine_t *co ) {
    if (!co->cIsShareStack) {
        free(co->stack_mem->stack_buffer);
        free(co->stack_mem);
    }
    free( co );
}


//func
void save_stack_buffer(stCoRoutine_t* occupy_co) {
    ///copy out
    stStackMem_t* stack_mem = occupy_co->stack_mem;
    int len = stack_mem->stack_bp - occupy_co->stack_sp;

    if (occupy_co->save_buffer)
    {
        free(occupy_co->save_buffer), occupy_co->save_buffer = NULL;
    }

    occupy_co->save_buffer = (char*)malloc(len); //malloc buf;
    occupy_co->save_size = len;

    memcpy(occupy_co->save_buffer, occupy_co->stack_sp, len);
}

stStackMem_t* co_get_stackmem(stShareStack_t* share_stack) {
    if (!share_stack)
    {
        return NULL;
    }
    int idx = share_stack->alloc_idx % share_stack->count;
    share_stack->alloc_idx++;

    return share_stack->stack_array[idx];
}

stStackMem_t* co_alloc_stackmem(unsigned int stack_size) {
    stStackMem_t* stack_mem = (stStackMem_t*)malloc(sizeof(stStackMem_t));
    stack_mem->occupy_co= NULL;
    stack_mem->stack_size = stack_size;
    stack_mem->stack_buffer = (char*)malloc(stack_size);
    stack_mem->stack_bp = stack_mem->stack_buffer + stack_size;
    return stack_mem;
}

stShareStack_t* co_alloc_sharestack(int count, int stack_size) {
    stShareStack_t* share_stack = (stShareStack_t*)malloc(sizeof(stShareStack_t));
    share_stack->alloc_idx = 0;
    share_stack->stack_size = stack_size;

    //alloc stack array
    share_stack->count = count;
    stStackMem_t** stack_array = (stStackMem_t**)calloc(count, sizeof(stStackMem_t*));
    for (int i = 0; i < count; i++)
    {
        stack_array[i] = co_alloc_stackmem(stack_size);
    }
    share_stack->stack_array = stack_array;
    return share_stack;
}
