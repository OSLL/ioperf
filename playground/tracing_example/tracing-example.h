#undef TRACE_SYSTEM
#define TRACE_SYSTEM sample

#if !defined(_TRACING_EXAMPLE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACING_EXAMPLE_H

#include <linux/tracepoint.h>

typedef unsigned long long ticks;

#ifdef CREATE_TRACE_POINTS
#define EXPORT_TRACING(name) EXPORT_TRACEPOINT_SYMBOL(tracing_##name);
#else
#define EXPORT_TRACING(name)
#endif

#define TRACING(tname) \
   TRACE_EVENT(tracing_##tname, TP_PROTO(int ignore), TP_ARGS(ignore), \
         TP_STRUCT__entry(__field(ticks, time)), \
         TP_fast_assign( \
            unsigned a, d; \
            __asm__ volatile("rdtscp" : "=a"(a), "=d"(d)); \
            __entry->time = (((ticks)a) | (((ticks)d) << 32)); \
         ), \
         TP_printk("Tracing " #tname " %llu", __entry->time)); \
   EXPORT_TRACING(tname)

#define TRACING_TAP(tname) trace_tracing_##tname(0)

// Your traces go to this header
#include "custom-traces.h"

#endif

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH .

#define TRACE_INCLUDE_FILE tracing-example
#include <trace/define_trace.h>
