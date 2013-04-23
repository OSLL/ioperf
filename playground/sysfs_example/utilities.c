#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pagemap.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/compiler.h>
#include <linux/swap.h>
#include <linux/slab.h>


/**
 * add_pages_range - first, forces all pages into cache, then adds range pages into cache.
 * @pathname       - path to specified file
 * @index_start    - index of page where range starts
 * @index_end      - index of page where range ends (inclusive)
 *
 */
ssize_t add_pages_range(const char * pathname, pgoff_t index_start, pgoff_t index_end)
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
                printk(KERN_INFO "filemap_write_and_wait is failed:%zd\n", ret);
                goto close;
        }

        if (mapping->nrpages) {
                ret = invalidate_inode_pages2(mapping);
                if (ret) {
                        printk(KERN_INFO "invalidate_inode_pages2 is failed:%zd\n", ret);
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
             	        printk(KERN_INFO "add_to_page_cahe_lru is failed: %zd\n", ret);
                        goto close;
                }

                /* unlock_page into readpage*/
                ret = mapping->a_ops->readpage(filp, page);
                if (unlikely(ret)) {
             	        printk(KERN_INFO "readpage is failed: %zd\n", ret);
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

/**
 * force_cache     - force pages from cache for specified file
 * @pathname       - path to specified file
 *
 */
ssize_t force_cache(const char * pathname)
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
                printk(KERN_INFO "filemap_write_and_wait is failed:%zd\n", ret);
                goto close;
        }

        if (mapping->nrpages) {
                ret = invalidate_inode_pages2(mapping);
                if (ret) {
                        printk(KERN_INFO "invalidate_inode_pages2 is failed:%zd\n", ret);
                        goto close;
                }
        }



close:
        mutex_unlock(&inode->i_mutex);
        filp_close(filp, NULL);
out:
        return ret;
}


/**
 * enum_cached_pages    - enumeration page into cache
 * @pathname            - path to specified file
 *
 */
ssize_t enum_cached_pages(const char * pathname, char ** log)
{
        ssize_t ret = 0;
        struct file * filp;
        struct address_space *mapping;
        pgoff_t count;
        pgoff_t index;
        size_t digits;
        pgoff_t i;
        int written = 0;
        char * pstr; 

        filp = filp_open(pathname, 0, 0);

        if(IS_ERR(filp)) {
                printk(KERN_INFO "filp_open is failed\n");
                ret = -EBADF;
                goto out;
        }

        mapping = filp->f_mapping;
        count = (mapping->host->i_size + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
        
        kfree(*log);

        digits = 0;
        i = count;
        while (i) {
                i /= 10;
                ++digits;
        }

/* size = (top_bound_number + 1 space )*count + len_header + top_bound_number*/
        *log = kmalloc((digits+1)*count + 25 + digits, GFP_KERNEL);
        if (!(*log)) {
            ret = -EINVAL;
            printk(KERN_INFO "kmalloc is failed\n");
            goto close;
        }
        pstr = *log; 
        written = sprintf(pstr, "File has %ld pages. Cached:\n", count);
        pstr += written;
    	for(index = 0; index != count; ++index) {
        		struct page *page = find_get_page(mapping, index);
        		if(!IS_ERR(page) && page != NULL) {
        		    	written = sprintf(pstr, " %ld", index);
                        pstr += written;
                }
    	}
        
        //printk( KERN_INFO "%s\n",*log);
close:
        filp_close(filp, NULL);
out:
        return ret;
}
