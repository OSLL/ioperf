/**
 * Used as sample kset and ktype implementation by Greg Kroah-Hartman 
 * 
 * Copyright (C) 2004-2007 Greg Kroah-Hartman <greg@kroah.com>
 * Copyright (C) 2007 Novell Inc.
 *
 * Released under the GPL version 2 only.
 *
 */
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/string.h>

#define PATH_LEN 4096
ssize_t add_pages_range(const char * pathname, pgoff_t index_start, pgoff_t index_end);
ssize_t force_cache(const char * pathname);
ssize_t enum_cached_pages(const char * pathname, char ** log);

/*----------------------------------------------------------------*/

struct range {
    unsigned long rstart;
    unsigned long rend;
};

/* Object of utility in sysfs */
struct utility_obj {
        struct kobject kobj;
        char pathname[PATH_LEN];
        union {
            struct range * r;
            char * enum_log;
        };
};
#define to_utility_obj(x) container_of(x, struct utility_obj, kobj)


static struct kset *utility_kset;
static struct utility_obj *force_cache_obj;
static struct utility_obj *enumer_page_obj;
static struct utility_obj *add_range_obj;


/*----------------------------------------------------------------*/

/* Attribute for utility_obj */
struct utility_attribute {
        struct attribute attr;
        ssize_t (*show)(struct utility_obj *foo, struct utility_attribute *attr, char *buf);
        ssize_t (*store)(struct utility_obj *foo, struct utility_attribute *attr, const char *buf, size_t count);
};
#define to_utility_attr(x) container_of(x, struct utility_attribute, attr)

static ssize_t utility_attr_show(struct kobject *kobj,
			     struct attribute *attr,
			     char *buf)
{
        struct utility_attribute *attribute;
        struct utility_obj *foo;

        attribute = to_utility_attr(attr);
        foo = to_utility_obj(kobj);

        if (!attribute->show)
                return -EIO;
        
        return attribute->show(foo, attribute, buf);
}


static ssize_t utility_attr_store(struct kobject *kobj,
			      struct attribute *attr,
			      const char *buf, size_t len)
{
        struct utility_attribute *attribute;
        struct utility_obj *foo;
        
        attribute = to_utility_attr(attr);
        foo = to_utility_obj(kobj);
        
        if (!attribute->store)
       	        return -EIO;
        
        return attribute->store(foo, attribute, buf, len);
}

static const struct sysfs_ops utility_sysfs_ops = {
        .show = utility_attr_show,
        .store = utility_attr_store,
};

/*----------------------------------------------------------------*/

static void utility_release(struct kobject *kobj)
{
        struct utility_obj *util;
        
        util = to_utility_obj(kobj);
        if (util->r) {
            kfree(util->r);
        }
        kfree(util);
}


static ssize_t pathname_show(struct utility_obj *util_obj, struct utility_attribute *attr,
                    char *buf)
{
        return sprintf(buf, "%s\n", util_obj->pathname);
}

static ssize_t pathname_store(struct utility_obj *util_obj, struct utility_attribute *attr,
			 const char *buf, size_t count)
{
        sscanf(buf, "%s", util_obj->pathname);
        return count;
}

static ssize_t start_show(struct utility_obj *util_obj, struct utility_attribute *attr,
                    char *buf)
{
        return sprintf(buf, "\n");
}

static ssize_t start_store(struct utility_obj *util_obj, struct utility_attribute *attr,
			 const char *buf, size_t count)
{
        ssize_t ret = 0;
        int fl;
        sscanf(buf, "%d", &fl);
        if (fl) {
                if (util_obj == add_range_obj && util_obj->r) {
                        printk (KERN_INFO "range %s\n", util_obj->pathname); 
                        ret = add_pages_range(util_obj->pathname,
                                        util_obj->r->rstart,  util_obj->r->rend);
                        if (ret < 0) {
                                printk (KERN_ALERT "add_pages_range is failed:%zd\n", ret);
                        }
                } else if (util_obj == enumer_page_obj && util_obj->enum_log){
                        printk (KERN_INFO "enum %s\n", util_obj->pathname);            
                        ret = enum_cached_pages(util_obj->pathname, &util_obj->enum_log);
                        if (ret < 0) {
                                printk (KERN_ALERT "enum_cached_pages is failed:%zd\n", ret);
                        }
                } else if (util_obj == force_cache_obj) {
                        printk (KERN_INFO "force_cache %s\n", util_obj->pathname);
                        ret = force_cache(util_obj->pathname);
                        if (ret < 0) {
                                printk (KERN_ALERT "force_cache is failed:%zd\n", ret);
                        }
                }            
        }
        return count;
}


static struct utility_attribute pathname_attribute =
        __ATTR(pathname, 0666, pathname_show, pathname_store);

static struct utility_attribute start_attribute =
        __ATTR(start, 0222, start_show, start_store);


static struct attribute *utility_default_attrs[] = {
        &pathname_attribute.attr,
        &start_attribute.attr,
        NULL,
}; 

static struct kobj_type utility_ktype = {
        .sysfs_ops = &utility_sysfs_ops,
        .release = utility_release,
        .default_attrs = utility_default_attrs,
};

/*----------------------------------------------------------------*/
/* Specific attributes */
static ssize_t range_show(struct utility_obj *util_obj, struct utility_attribute *attr,
                    char *buf)
{
        return sprintf(buf, "%lu %lu\n", util_obj->r->rstart, util_obj->r->rend);
}

static ssize_t range_store(struct utility_obj *util_obj, struct utility_attribute *attr,
			 const char *buf, size_t count)
{
        sscanf(buf, "%lu %lu", &util_obj->r->rstart, &util_obj->r->rend);
        return count;
}

static struct utility_attribute range_attribute =
        __ATTR(range, 0666, range_show, range_store);




static ssize_t log_show(struct utility_obj *util_obj, struct utility_attribute *attr,
                    char *buf)
{
        return sprintf(buf, "%s\n", util_obj->enum_log);
}

static ssize_t log_store(struct utility_obj *util_obj, struct utility_attribute *attr,
			 const char *buf, size_t count)
{
        return count;
}

static struct utility_attribute log_attribute =
        __ATTR(log, 0444, log_show, log_store);

/**
 * Creates object with specified name in sysfs
 * @name    - specified name
 */
static struct utility_obj *create_utility_obj(const char *name)
{
        struct utility_obj *util;
        int ret;

        util = kzalloc(sizeof(*util), GFP_KERNEL);
        if (!util)
                return NULL;
        
        util->kobj.kset = utility_kset;
        
        ret = kobject_init_and_add(&util->kobj, &utility_ktype, NULL, "%s", name);
        if (ret) {
                kobject_put(&util->kobj);
                return NULL;
        }

        kobject_uevent(&util->kobj, KOBJ_ADD);

        return util;
}

/**
 * Adds specific attribute - range for specified utility_obj
 * @obj     -  specified utility_obj
 */
static ssize_t init_add_range_obj(struct utility_obj * obj)
{
        int ret;
    
        obj->r = kzalloc(sizeof(*(obj->r)), GFP_KERNEL);
        if (!obj->r)
                return -EINVAL;
        ret = sysfs_create_file(&obj->kobj, &range_attribute.attr);
        return ret;
}

/**
 * Adds specific attribute - log for specified utility_obj
 * @obj     -  specified utility_obj
 */
static ssize_t init_enumer_page_obj(struct utility_obj * obj)
{
        int ret;
        
        obj->enum_log = kzalloc(1, GFP_KERNEL);
        if (!obj->r)
                return -EINVAL;
        ret = sysfs_create_file(&obj->kobj, &log_attribute.attr);
        return ret;    
}

/**
 * Release count ref of specified utility_obj
 * @obj     -  specified utility_obj
 */
static void destroy_utility_obj(struct utility_obj *util)
{
        kobject_put(&util->kobj);
}


static int __init init(void)
{
        printk(KERN_INFO "init\n");
        
        utility_kset = kset_create_and_add("ioperf_cache_utility", NULL, NULL);
        if (!utility_kset) {
                return -ENOMEM;
        }
        
        force_cache_obj = create_utility_obj("force_cache");
        if (!force_cache_obj)
                goto force_error;
        
        enumer_page_obj = create_utility_obj("enumerate_cached_pages");        
        if (!enumer_page_obj)
                goto enum_error;
        if (init_enumer_page_obj(enumer_page_obj) < 0)
                goto init_enum_error;
       


        add_range_obj = create_utility_obj("add_page_range");
        if (!add_range_obj)
                goto add_range_error;
      
        if (init_add_range_obj(add_range_obj) < 0)
                goto init_range_error;
       
        return 0;

init_range_error:
        destroy_utility_obj(add_range_obj);
init_enum_error:
        ;
add_range_error:
        destroy_utility_obj(enumer_page_obj);        
enum_error:
        destroy_utility_obj(force_cache_obj);
force_error:
        return -EINVAL;

}

static void __exit exit(void)
{
        printk(KERN_INFO "exit\n");
        destroy_utility_obj(force_cache_obj);
        destroy_utility_obj(enumer_page_obj);
        destroy_utility_obj(add_range_obj);
        kset_unregister(utility_kset);
}

module_init(init);
module_exit(exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vadim.lomshakov@gmail.com");
MODULE_DESCRIPTION("Utility functions for io perfomance test: enumeration page into cache, force pages from cache, add range pages into cache.");

