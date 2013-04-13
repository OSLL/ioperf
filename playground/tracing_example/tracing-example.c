#include <linux/module.h>

#define CREATE_TRACE_POINTS
#include "tracing-example.h"

static int __init tracing_example_init(void)
{
   return 0;
}

static void __exit tracing_example_exit(void)
{
   return;
}

module_init(tracing_example_init);
module_exit(tracing_example_exit);
MODULE_LICENSE("GPL");
