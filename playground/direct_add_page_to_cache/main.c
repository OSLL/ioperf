#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pagemap.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/compiler.h>
#include <linux/string.h>
#include <linux/swap.h>


static struct file * filp;
static loff_t file_size;

static ssize_t add_page(/*struct file * filp,*/ const char * buf, size_t count)
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
        
        page = find_get_page(mapping, index);
        if (!page) {
            gfp_t gfp_mask;
            
            gfp_mask = mapping_gfp_mask(mapping);
            page = __page_cache_alloc(gfp_mask);
            status = add_to_page_cache_lru(page, mapping, index, GFP_KERNEL);
            if (unlikely(status)) {
                page_cache_release(page);
                ret = status;
                printk(KERN_INFO "add_to_page_cahe failed: %zd\n", ret);
                break;
            }
        }
        
        
        kaddr = kmap_atomic(page);
        memcpy(kaddr + offset, buf, bytes);
        kunmap_atomic(kaddr);
        
        SetPageUptodate(page);
        mark_page_accessed(page);
        
        buf += bytes;
        count -= bytes;
        pos += bytes;
        written += bytes;
    } while (count);
    
    /**
     * not sure what all logic is executed, in ext3 need update inode's i_disksize
     * see update_file_sizes(inode, filp->f_pos, written);
     */
    if (pos > inode->i_size) {
        i_size_write(inode, pos);
    }
    
    filp->f_pos = filp->f_pos + written;
    
    return ret;
}

static char buf[PAGE_CACHE_SIZE];

static ssize_t direct_add_page_to_cache(const char * pathname, size_t count_page)
{
    ssize_t ret = 0;
    /*struct file * */filp = filp_open(pathname, O_WRONLY | O_APPEND, S_IWUSR );
    
    if(IS_ERR(filp)) { filp = 0; ret = -EBADF;}
    
    if (filp) {
        file_size = filp->f_mapping->host->i_size;
        
        while (count_page-- > 0) {
            
            memset(buf,'0'+count_page,PAGE_CACHE_SIZE);
            
            ret = add_page(/*filp,*/ buf, PAGE_CACHE_SIZE);
            if (ret < 0) break;
        }
        
    }
    
    
    //filp_close(filp, NULL);
    return ret;
}

static ssize_t clear_cache(size_t count_page)
{
    struct inode *inode = filp->f_mapping->host;
    struct address_space *mapping = filp->f_mapping;
    ssize_t ret = 0;
    
    do {
        
        struct page *page;
        
        
        pgoff_t index = 0;
        
        page = find_get_page(mapping, index);
        if (page) {
            page_cache_release(page);
            ++ret;
        }
        
        count_page -= 1;
    } while (count_page);
    
    /**
     * not sure what all logic is executed, in ext3 need update inode's i_disksize
     * see update_file_sizes(inode, filp->f_pos, written);
     */
    i_size_write(inode, file_size);
    
    
    filp->f_pos = 0;
    
    return ret;
}


static int __init init(void)
{
	ssize_t ret;
	printk(KERN_INFO "init\n");
    
    ret = direct_add_page_to_cache("./file.txt", 5);
	if (ret < 0) {
		printk(KERN_INFO "errno:%zd\n", ret);
	}
    
	return 0;
}

static void __exit exit(void)
{
    printk(KERN_INFO "cleaned page:%zd\n", clear_cache(5));
    filp_close(filp, NULL);
	printk(KERN_INFO "exit\n");
}

module_init(init);
module_exit(exit);

MODULE_LICENSE("GPL");




