#include <linux/module.h>

#include "tracing-example.h"

static int worker(void* arg)
{
   int count = *(int*)arg;
   int i, sum;
   sum = 1;
   TRACING_TAP(start_free);
   for(i = 1; i != count; ++i) sum *= i;
   TRACING_TAP(stop_free);
   sum = 1;
   TRACING_TAP(start_instr);
   for(i = 1; i != count; ++i) {
      sum *= i;
      TRACING_TAP(go_instr);
   }
   TRACING_TAP(stop_instr);
   return 0;
}

static int __init tracing_example_init(void)
{
   int count = 1000;
   worker(&count);
   return 0;
}

static void __exit tracing_example_exit(void) { }

module_init(tracing_example_init);
module_exit(tracing_example_exit);
MODULE_LICENSE("GPL");
