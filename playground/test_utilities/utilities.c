#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pagemap.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/compiler.h>
#include <linux/swap.h>


static ssize_t add_pages_range(const char * pathname, pgoff_t index_start, pgoff_t index_end)
{
    ssize_t ret = 0;
    struct file * filp;
    struct address_space *mapping;
    struct inode *inode;
    loff_t isize;
    pgoff_t index;
    pgoff_t end;
    
    filp = filp_open(pathname, O_RDONLY | O_WRONLY | O_APPEND, S_IWUSR );
    
    if(IS_ERR(filp)) {
        printk(KERN_INFO "filp_open is failed\n");
        ret = -EBADF;
        goto out;
    }
    
    mapping = filp->f_mapping;
    inode = filp->f_mapping->host;
    
    mutex_lock(&inode->i_mutex);
    
    ret = filemap_write_and_wait(mapping);
    
    if (ret) {
        printk(KERN_INFO "filemap_write_and_wait is failed:%d\n", ret);
        goto close;
    }
    
    if (mapping->nrpages) {
        ret = invalidate_inode_pages2(mapping);
        if (ret) {
            printk(KERN_INFO "invalidate_inode_pages2 is failed:%d\n", ret);
            goto close;
        }
    }
    
    
    index = index_start;
    isize = i_size_read(inode);
    end = (isize - 1) >> PAGE_CACHE_SHIFT;
    
    if (unlikely(!isize || index > end)) {
        goto close;
    }
    while (index != index_end + 1) {
        struct page *page;
        
        cond_resched();
        
        page =  page_cache_alloc_cold(mapping);
        if (!page) {
            printk(KERN_INFO "page_cache_alloc_cold is failed");
            goto close;
        }
        ret = add_to_page_cache_lru(page, mapping, index, GFP_KERNEL);
        if (unlikely(ret)) {
            page_cache_release(page);
            printk(KERN_INFO "add_to_page_cahe_lru is failed: %d\n", ret);
            goto close;
        }
        
        /* ulock_page into readpage*/
        ret = mapping->a_ops->readpage(filp, page);
        if (unlikely(ret)) {
            printk(KERN_INFO "readpage is failed: %d\n", ret);
            goto close;
        }
        
        mark_page_accessed(page);
        page_cache_release(page);
        
        ++index;
        
        if (unlikely(index > end)) {
            goto close;
        }
        
    }
    
    
close:
    mutex_unlock(&inode->i_mutex);
    filp_close(filp, NULL);
out:
    return ret;
}


static ssize_t force_cache(const char * pathname)
{
    ssize_t ret = 0;
    struct file * filp;
    struct address_space *mapping;
    struct inode *inode;
    
    filp = filp_open(pathname, O_WRONLY | O_APPEND, S_IWUSR );
    
    if(IS_ERR(filp)) {
        printk(KERN_INFO "filp_open is failed\n");
        ret = -EBADF;
        goto out;
    }
    
    mapping = filp->f_mapping;
    inode = filp->f_mapping->host;
    
    mutex_lock(&inode->i_mutex);
    
    ret = filemap_write_and_wait(mapping);
    
    if (ret) {
        printk(KERN_INFO "filemap_write_and_wait is failed:%d\n", ret);
        goto close;
    }
    
    if (mapping->nrpages) {
        ret = invalidate_inode_pages2(mapping);
        if (ret) {
            printk(KERN_INFO "invalidate_inode_pages2 is failed:%d\n", ret);
            goto close;
        }
    }
    
    
    
close:
    mutex_unlock(&inode->i_mutex);
    filp_close(filp, NULL);
out:
    return ret;
}



ssize_t enum_cached_pages(const char * pathname)
{
    ssize_t ret = 0;
    struct file * filp;
    struct address_space *mapping;
    pgoff_t count;
    pgoff_t index;
    
    filp = filp_open(pathname, 0, 0);
    
    if(IS_ERR(filp)) {
        printk(KERN_INFO "filp_open is failed\n");
        ret = -EBADF;
        goto out;
    }
    
    mapping = filp->f_mapping;
    count = (mapping->host->i_size + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
    
    printk(KERN_ALERT "File has %ld pages. Cached:\n", count);
    for(index = 0; index != count; ++index) {
        struct page *page = find_get_page(mapping, index);
        if(!IS_ERR(page) && page != NULL)
            printk(KERN_ALERT "   %ld\n", index);
    }
    
    filp_close(filp, NULL);
out:
    return ret;
}



static int __init init(void)
{
    ssize_t ret;
    printk(KERN_INFO "init\n");
    enum_cached_pages("./file.txt");
    ret = force_cache("./file.txt");
    enum_cached_pages("./file.txt");
    ret = add_pages_range("./file.txt",0,60);
    enum_cached_pages("./file.txt");
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
MODULE_DESCRIPTION("Utility functions for io perfomance test: enumeration page into cache, force cache from adress space, add range pages into cache.");
