/**********************************************************************
 * Copyright ( C ) 2002-2005 XIMETA, Inc.
 * All rights reserved.
 **********************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <intrman.h>
#include "sal/libc.h"
#include "sal/debug.h"
#include "sal/sync.h"
#include "sal/types.h"

#define DEBUG_USE_SCR 1

#define DEBUG_LOG_SIZE 64 * 1024

static char v_debug_log[DEBUG_LOG_SIZE+1];
static volatile int v_debug_log_head = 0;
static volatile int v_debug_log_tail = 0;
static volatile int v_debug_log_len = 0;

int SalPrintToLog(char* buf, int len)
{
    int bufroom;
    int remain = len;
    char* ptr = buf;
    int copy;
    CpuDisableIntr();
    while(remain>0) {
        bufroom = DEBUG_LOG_SIZE - v_debug_log_tail;
        if (bufroom < remain) {
            copy = bufroom;
        } else {
            copy = remain;    
        }
        memcpy(v_debug_log+v_debug_log_tail, ptr, copy);
        v_debug_log_tail += copy;
        v_debug_log_len += copy;
        remain -= copy;
        ptr += copy;
        if (v_debug_log_tail == DEBUG_LOG_SIZE) {
            v_debug_log_tail = 0;
        }
        if (v_debug_log_tail > DEBUG_LOG_SIZE) {
            printf("Bug in SalPrintToLog!\n");    
        }
        if (v_debug_log_len >=DEBUG_LOG_SIZE) {
            printf("Debug log buffer overflow\n");
        }
    }
    CpuEnableIntr();
    return (ptr-buf);
}

int SalGetLog(char* buf, int len)
{
    int remain = len;
    int copy; 
    char* ptr= buf;
    int copied=0;

    CpuDisableIntr();
    while(v_debug_log_len>0 && remain>0) {
        // copy = min(v_debug_log_len, remain, DEBUG_LOG_SIZE-v_debug_log_head)
        copy = (v_debug_log_len<remain)?v_debug_log_len:remain;
        copy = (copy<DEBUG_LOG_SIZE-v_debug_log_head)?copy:DEBUG_LOG_SIZE-v_debug_log_head;

        memcpy(ptr, v_debug_log + v_debug_log_head, copy);
        v_debug_log_head += copy;
        ptr+=copy;
        v_debug_log_len -= copy;
        remain -= copy;
        copied += copy;
        if (v_debug_log_head == DEBUG_LOG_SIZE) {
            v_debug_log_head=0;
        }
    }    
    CpuEnableIntr();
    return copied;
}

int SalErrorVprint(char* fmt, va_list varg)
{
    char buf[256];
    int ret;
    ret = vsnprintf(buf, 256, fmt, varg);
    if (ret) {
#if DEBUG_USE_SCR
        SalPrintToLog(buf, ret);
#else
        printf("%s", buf);
#endif
    }
    return ret;    
}

int sal_error_print(char* fmt, ...)
{
    va_list varg;
    int ret;
    va_start(varg, fmt);
    ret = SalErrorVprint(fmt, varg);
    va_end(varg);
   return 0;
}

#ifdef DEBUG

int SalDebugVprint(char* fmt, va_list varg)
{
    char buf[256];
    int ret;
    ret = vsnprintf(buf, 256, fmt, varg);
    if (ret) {
#if DEBUG_USE_SCR
        SalPrintToLog(buf, ret);
#else
        printf("%s", buf);
#endif
    }
    return ret;
}
int sal_debug_print(char* fmt, ...)
{
    va_list varg;
    int ret;
    va_start(varg, fmt);
    ret = SalDebugVprint(fmt, varg);
    va_end(varg);
   return 0;
//    return ret;
}
int sal_debug_println_impl(const char* file, 
    int line, const char* function,const char* fmt, ...)
{
    va_list varg;
    int ret;
    va_start(varg, fmt);
    ret = SalDebugVprint(fmt, varg);
    va_end(varg);
    ret += sal_debug_print("|%s,%d,%s\n", file,line,function);
   return 0;
//    return ret;
}
void sal_debug_hexdump(char* buf, int len)
{
    int i;
    for(i=0;i<len;i++) {
        if (i!=0 && i%32==0)
            sal_debug_print("\n");
        sal_debug_print("%02x", 0x0ff & buf[i]);
        if (i%4==3)
            sal_debug_print(" ");
    }
    sal_debug_print("\n");
}
#endif