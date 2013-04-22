#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pagemap.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/compiler.h>
#include <linux/string.h>
#include <linux/swap.h>

static size_t pagecount = 5;
static struct file * filp;
static loff_t old_file_size;

static char buf[PAGE_CACHE_SIZE];

static ssize_t add_page(const char * buf, size_t count)
{
    struct inode *inode = filp->f_mapping->host;
    struct address_space *mapping = filp->f_mapping;
    ssize_t ret = 0;
    loff_t pos = filp->f_pos;
    
    ssize_t written = 0;
    
    do {
        char * kaddr;
        struct page *page;
        unsigned long offset = (pos & (PAGE_CACHE_SIZE - 1));
        unsigned long bytes = min_t(unsigned long, PAGE_CACHE_SIZE - offset, count);
        int status;
        pgoff_t index = pos >> PAGE_CACHE_SHIFT;
        
        cond_resched();
        
        page = find_get_page(mapping, index);
        //                if (page) {
        //                        int err = filemap_write_and_wait_range(mapping, pos, pos + bytes - 1);                        if (err == 0) {
        //                                invalidate_mapping_pages(mapping,
        //                                                         index,
        //                                                         index);
        //                                page = NULL;
        //                        } else {
        //
        //                        }
        //
        //                }
        if (!page) {
            
            //                        printk (KERN_INFO "alloc page\n");
            
            page =  page_cache_alloc_cold(mapping);
            status = add_to_page_cache_lru(page, mapping, index, GFP_KERNEL);
            if (unlikely(status)) {
                page_cache_release(page);
                ret = status;
                printk(KERN_INFO "add_to_page_cahe_lru failed: %d\n", ret);
                break;
            }
            
        }
        
        
        kaddr = kmap_atomic(page);
        memcpy(kaddr + offset, buf, bytes);
        kunmap_atomic(kaddr);
        
        SetPageUptodate(page);
        mark_page_accessed(page);
        
        unlock_page(page);
        page_cache_release(page);
        
        buf += bytes;
        count -= bytes;
        pos += bytes;
        written += bytes;
    } while (count);
    
    /**
     * not sure what all logic is executed, in ext3 need update inode's i_disksize
     * see update_old_file_sizes(inode, filp->f_pos, written);
     */
    if (pos > i_size_read(inode) ) {
        i_size_write(inode, pos);
    }
    
    filp->f_pos = filp->f_pos + written;
    
    return ret;
}

static ssize_t direct_add_page_to_cache(char * buf, const char * pathname, size_t count_page)
{
    ssize_t ret = 0;
    filp = filp_open(pathname, O_WRONLY | O_APPEND, S_IWUSR );
    
    if(IS_ERR(filp)) { filp = 0; ret = -EBADF;}
    
    if (filp) {
        char tmp[]="*deadbeef*";
        int size_tmp = strlen(tmp);
        int cnt = PAGE_CACHE_SIZE/size_tmp;
        char * ptr = buf;
        int tail = 0;
        int i = cnt;
        
        while(i--) {
            memcpy(ptr, tmp, size_tmp);
            ptr += size_tmp;
        }
        tail = PAGE_CACHE_SIZE - cnt*size_tmp;
        if (tail > 0) memcpy(ptr, tmp, tail);
        
        old_file_size = i_size_read(filp->f_mapping->host);
        
        while (count_page-- > 0) {
            ret = add_page(buf, PAGE_CACHE_SIZE);
            if (unlikely(ret)) break;
        }
        
    }
    
    return ret;
}

static void clear_cache(size_t count_page)
{
    struct inode *inode = filp->f_mapping->host;
    
    /**
     * not sure what all logic is executed, in ext3 need update inode's i_disksize
     * see update_old_file_sizes(inode, filp->f_pos, written);
     */
    i_size_write(inode, old_file_size);
    
    filp->f_pos = 0;
    
    /* NULL, because calles direct filp_open without using files_struct */
    filp_close(filp, NULL);
}

static int __init init(void)
{
	ssize_t ret;
	printk(KERN_INFO "init\n");
    
    ret = direct_add_page_to_cache(buf, "./file.txt", pagecount);
	if (ret < 0) {
		printk(KERN_INFO "errno:%d\n", ret);
	}
    
	return 0;
}

static void __exit exit(void)
{
    clear_cache(pagecount);
	printk(KERN_INFO "exit\n");
}

module_init(init);
module_exit(exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("V.Lomshakov");
MODULE_DESCRIPTION("Adds directly page filled *deadbeef* to cache after inserting this module and recoveries file's state after removing its");