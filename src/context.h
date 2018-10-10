//腾讯libco 寄存器交换相关逻辑使用需要使用的部分

#ifndef NODE_COROUTINE_CONTEXT_H
#define NODE_COROUTINE_CONTEXT_H
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>


typedef void *(*pfn_co_routine_t)( void * );


struct stCoRoutine_t;
struct coctx_param_t
{
    const void *s1;
    const void *s2;
};
struct coctx_t
{
#if defined(__i386__)
    void *regs[ 8 ];
#else
    void *regs[ 14 ];
#endif
    size_t ss_size;
    char *ss_sp;

};



struct stCoRoutineEnv_t
{
    stCoRoutine_t *pCallStack[ 128 ];
    int iCallStackSize;

    //for copy stack log lastco and nextco
    stCoRoutine_t* pending_co;
    stCoRoutine_t* occupy_co;
};



struct stCoSpec_t
{
    void *value;
};

struct stStackMem_t
{
    stCoRoutine_t* occupy_co;
    int stack_size;
    char* stack_bp; //stack_buffer + stack_size
    char* stack_buffer;

};

struct stShareStack_t
{
    unsigned int alloc_idx;
    int stack_size;
    int count;
    stStackMem_t** stack_array;
};


struct stCoRoutine_t
{
    stCoRoutineEnv_t *env;
    pfn_co_routine_t pfn;
    void *arg;
    coctx_t ctx;

    char cStart;
    char cEnd;
    char cIsMain;
    char cEnableSysHook;
    char cIsShareStack;

    void *pvEnv;

    //char sRunStack[ 1024 * 128 ];
    stStackMem_t* stack_mem;


    //save satck buffer while confilct on same stack_buffer;
    char* stack_sp;
    unsigned int save_size;
    char* save_buffer;

    stCoSpec_t aSpec[1024];

};


struct stCoRoutineAttr_t
{
    int stack_size;
    stShareStack_t*  share_stack;
    stCoRoutineAttr_t()
    {
        stack_size = 128 * 1024;
        share_stack = NULL;
    }
}__attribute__ ((packed));


//1.env
void init_thread_env();

//2.context
int coctx_init( coctx_t *ctx ,pfn_co_routine_t pfn,const void *s);
//3.coroutine
int co_create( stCoRoutine_t **co,const stCoRoutineAttr_t *attr,pfn_co_routine_t pfn,void *arg );
void co_swap(stCoRoutine_t* curr, stCoRoutine_t* pending_co,int main);
void co_yield();
void co_free( stCoRoutine_t * co );


//4.func
void save_stack_buffer(stCoRoutine_t* occupy_co);
stStackMem_t* co_get_stackmem(stShareStack_t* share_stack);
stStackMem_t* co_alloc_stackmem(unsigned int stack_size);
stShareStack_t* co_alloc_sharestack(int count, int stack_size);

#endif //NODE_COROUTINE_CONTEXT_H