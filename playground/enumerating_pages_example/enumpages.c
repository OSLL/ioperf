#include <linux/module.h>
#include "enumpages.h"
MODULE_LICENSE("GPL");

static int __init enumpages_init(void)
{
	enum_cached_pages("/root/log");
	enum_cached_pages("/root/linux-3.7.10.tar.bz2");
	return 0;
}

static void __exit enumpages_exit(void)
{
}

module_init(enumpages_init);
module_exit(enumpages_exit);
