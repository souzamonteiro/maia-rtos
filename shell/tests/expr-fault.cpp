#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
extern "C" {
void* bre_fault_malloc(size_t);
void* bre_fault_calloc(size_t,size_t);
void* bre_fault_realloc(void*,size_t);
void bre_fault_free(void*);
void bre_fault_reset(unsigned int);
unsigned int bre_fault_count(void);
unsigned int bre_fault_bytes(void);
unsigned int bre_fault_invalid(void);
}
#define malloc bre_fault_malloc
#define calloc bre_fault_calloc
#define realloc bre_fault_realloc
#define free bre_fault_free
#include "expr-probe.hpp"
#undef malloc
#undef calloc
#undef realloc
#undef free
static int sweep(const char* source, unsigned int length, const char* expected)
{
    memcpy(gExprProbeInput,source,length);gExprProbeInput[1200]=90;
    bre_fault_reset(0);
    if(expr_run(length)!=0 || strcmp(expr_result(),expected)!=0) return 10;
    const unsigned int calls=bre_fault_count();
    if(!calls || bre_fault_bytes() || bre_fault_invalid()) return 11;
    for(unsigned int failure=1;failure<=calls;++failure){
        bre_fault_reset(failure);
        if(expr_run(length)!=3 || expr_result()[0] || bre_fault_bytes()
            || bre_fault_invalid() || gExprProbeInput[1200]!=90
            || memcmp(gExprProbeInput,source,length)!=0){
            fprintf(stderr,"expr fault site %u/%u failed\n",failure,calls);return 12;
        }
        bre_fault_reset(0);
        if(expr_run(length)!=0 || strcmp(expr_result(),expected)!=0
            || bre_fault_bytes() || bre_fault_invalid()) return 13;
    }
    printf("Native expression allocation sweep: %u sites, status3/atomic result/no leaks/recovery\n",calls);
    return 0;
}
int main(void)
{
    const char arithmetic[]="2\0+\0" "3\0";
    const char matching[]="abc\0:\0" "\\(a.*\\)\0";
    int status=sweep(arithmetic,sizeof(arithmetic)-1,"5");
    if(status)return status;
    return sweep(matching,sizeof(matching)-1,"abc");
}
