#include <linux/module.h>
#include <linux/kernel.h>

ssize_t add_pages_range(const char * pathname, pgoff_t index_start, pgoff_t index_end);
ssize_t force_cache(const char * pathname);
ssize_t enum_cached_pages(const char * pathname);


static unsigned long rstart; 
static unsigned long rend;
static char *pathname = "";
module_param(rstart, ulong, S_IRUSR);
module_param(rend, ulong, S_IRUSR);
module_param(pathname, charp, S_IRUSR);

static int __init init(void)
{
        ssize_t ret;
        printk(KERN_INFO "init\n");
//        enum_cached_pages("./file.txt");
//        ret = force_cache("./file.txt");
//        enum_cached_pages("./file.txt");
//        ret = add_pages_range("./file.txt",0,60);
//        enum_cached_pages("./file.txt");
        
        enum_cached_pages(pathname);
        ret = force_cache(pathname);
        enum_cached_pages(pathname);
        ret = add_pages_range(pathname, rstart, rend);
        enum_cached_pages(pathname);


        if (ret < 0) {
                printk(KERN_INFO "errno init:%d\n", ret);
        }

        return 0;
}

static void __exit exit(void)
{
	    printk(KERN_INFO "exit\n");
}

module_init(init);
module_exit(exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vadim.lomshakov@gmail.com");
MODULE_DESCRIPTION("Utility functions for io perfomance test: enumeration page into cache, force pages from cache, add range pages into cache.");

