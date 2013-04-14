#include <linux/syscalls.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/export.h>
#include <linux/compiler.h>
#include "enumpages.h"

// From do_generic_file_read:
void enum_pages(struct file *filp)
{
	struct address_space *mapping = filp->f_mapping;
	pgoff_t count = (mapping->host->i_size + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
	pgoff_t index;
	printk(KERN_ALERT "File has %ld pages. Cached:\n", count);
	for(index = 0; index != count; ++index) {
		struct page *page = find_get_page(mapping, index);
		if(!IS_ERR(page) && page != NULL)
			printk(KERN_ALERT "   %ld", index);
	}
}

long enum_cached_pages(const char *filename)
{
	struct file *filp;
	filp = filp_open(filename, 0, 0);
	if(IS_ERR(filp)) filp = 0;
	if(!filp) {
		printk(KERN_ALERT "Unresolved filename %s\n", filename);
		return -EBADF;
	}
	enum_pages(filp);
	filp_close(filp, NULL); // Drivers use NULL here.
	return 0;
}
EXPORT_SYMBOL(enum_cached_pages);

SYSCALL_DEFINE1(enum_cached_pages, const char __user *, filename)
{
	char* kname = kmalloc(strlen(filename), GFP_KERNEL);
	strncpy_from_user(kname, filename, strlen(filename));
	long res =  enum_cached_pages(filename);
	kfree(kname);
	return res;
}
