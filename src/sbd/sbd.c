/*
 * A sample, extra-simple block driver. Updated for kernel 2.6.31.
 *
 * (C) 2003 Eklektix, Inc.
 * (C) 2010 Pat Patterson <pat at superpat dot com>
 * (C) 2013 Aleksey Pivovarov, SPBAU
 * Redistributable under the terms of the GNU GPL.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h> /* printk() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>

MODULE_LICENSE("Dual BSD/GPL");
// static char *Version = "1.4";

static int major_num = 0;
module_param(major_num, int, 0);
static int logical_block_size = 512;
module_param(logical_block_size, int, 0);
static int nsectors = 1024; /* How big the drive is */
module_param(nsectors, long, 0);

enum {
        RM_SIMPLE  = 0, /* The extra-simple request function */
        RM_NOQUEUE = 1, /* Use make_request */
};
static int request_mode = RM_SIMPLE;
module_param(request_mode, int, 0);

/*
 * We can tweak our hardware sector size, but the kernel talks to us
 * in terms of small sectors, always.
 */
#define KERNEL_SECTOR_SIZE 512

/*
 * Our request queue.
 */
static struct request_queue *Queue;

/*
 * The internal representation of our device.
 */
static struct sbd_device {
    unsigned long size;
    spinlock_t lock;
    u8 *data;
    struct gendisk *gd;
} Device;

/*
 * Handle an I/O request.
 */
static void sbd_transfer(struct sbd_device *dev, sector_t sector,
        unsigned long nsect, char *buffer, int write) {
    // unsigned long offset = sector * logical_block_size;
    // unsigned long nbytes = nsect * logical_block_size;
    unsigned long offset = sector * KERNEL_SECTOR_SIZE;
    unsigned long nbytes = nsect * KERNEL_SECTOR_SIZE;
    
    if ((offset + nbytes) > dev->size) {
        printk (KERN_NOTICE "sbd: Beyond-end write (%ld %ld)\n", offset, nbytes);
        return;
    }
    if (write)
        memcpy(dev->data + offset, buffer, nbytes);
    else
        memcpy(buffer, dev->data + offset, nbytes);
}

static void sbd_request(struct request_queue *q) {
    struct request *req;

    req = blk_fetch_request(q);
    while (req != NULL) {
        // blk_fs_request() was removed in 2.6.36 - many thanks to
        // Christian Paro for the heads up and fix...
        //if (!blk_fs_request(req)) {
        if (req == NULL || (req->cmd_type != REQ_TYPE_FS)) {
            printk (KERN_NOTICE "Skip non-CMD request\n");
            __blk_end_request_all(req, -EIO);
            continue;
        }
        sbd_transfer(&Device, blk_rq_pos(req), blk_rq_cur_sectors(req),
                req->buffer, rq_data_dir(req));
        if ( ! __blk_end_request_cur(req, 0) ) {
            req = blk_fetch_request(q);
        }
    }
}

static inline unsigned int bio_cur_sectors(struct bio *bio) {
    return bio_cur_bytes(bio) >> 9;
}
/*
 * Transfer a single BIO.
 */
static int sbd_xfer_bio(struct sbd_device *dev, struct bio *bio)
{
    int i;
    struct bio_vec *bvec;
    sector_t sector = bio->bi_sector;

    /* Do each segment independently. */
    bio_for_each_segment(bvec, bio, i) {
        char *buffer = __bio_kmap_atomic(bio, i, KM_USER0);
        sbd_transfer(dev, sector, bio_cur_sectors(bio),
                        buffer, bio_data_dir(bio) == WRITE);
        sector += bio_cur_sectors(bio);
        __bio_kunmap_atomic(bio, KM_USER0);
    }
    return 0; /* Always "succeed" */
}

/*
 * The direct make request version.
 */
static void sbd_make_request(struct request_queue *q, struct bio *bio)
{
    struct sbd_device *dev = &Device;
    int status;

    status = sbd_xfer_bio(dev, bio);

    bio_endio(bio, status);
}

/*
 * The HDIO_GETGEO ioctl is handled in blkdev_ioctl(), which
 * calls this. We need to implement getgeo, since we can't
 * use tools such as fdisk to partition the drive otherwise.
 */
int sbd_getgeo(struct block_device * block_device, struct hd_geometry * geo) {
    long size;

    /* We have no real geometry, of course, so make something up. */
    size = Device.size * (logical_block_size / KERNEL_SECTOR_SIZE);
    geo->cylinders = (size & ~0x3f) >> 6;
    geo->heads = 4;
    geo->sectors = 16;
    geo->start = 0;
    return 0;
}

/*
 * The device operations structure.
 */
static struct block_device_operations sbd_ops = {
        .owner  = THIS_MODULE,
        .getgeo = sbd_getgeo
};

static int __init sbd_init(void) {
    /*
     * Set up our internal device.
     */
    Device.size = nsectors * logical_block_size;
    spin_lock_init(&Device.lock);
    Device.data = vzalloc(Device.size);
    if (Device.data == NULL)
        return -ENOMEM;
    /*
     * Get a request queue.
     */

    switch (request_mode) {
        case RM_NOQUEUE:
            Queue = blk_alloc_queue(GFP_KERNEL);
            if (Queue == NULL)
                goto out;
            blk_queue_make_request(Queue, sbd_make_request);
            break;

        default:
            printk(KERN_NOTICE "Bad request mode %d, using simple\n", request_mode);
            /* fall into.. */
    
        case RM_SIMPLE:
            Queue = blk_init_queue(sbd_request, &Device.lock);
            if (Queue == NULL)
                goto out;
            break;
    }

    blk_queue_logical_block_size(Queue, logical_block_size);
    /*
     * Get registered.
     */
    major_num = register_blkdev(major_num, "sbd");
    if (major_num < 0) {
        printk(KERN_WARNING "sbd: unable to get major number\n");
        goto out;
    }
    /*
     * And the gendisk structure.
     */
    Device.gd = alloc_disk(16);
    if (!Device.gd)
        goto out_unregister;
    Device.gd->major = major_num;
    Device.gd->first_minor = 0;
    Device.gd->fops = &sbd_ops;
    Device.gd->private_data = &Device;
    strcpy(Device.gd->disk_name, "sbd0");
    // set_capacity(Device.gd, nsectors);
    set_capacity(Device.gd, nsectors*(logical_block_size/KERNEL_SECTOR_SIZE));
    Device.gd->queue = Queue;
    add_disk(Device.gd);

    return 0;

out_unregister:
    unregister_blkdev(major_num, "sbd");
out:
    vfree(Device.data);
    return -ENOMEM;
}

static void __exit sbd_exit(void)
{
    del_gendisk(Device.gd);
    put_disk(Device.gd);
    unregister_blkdev(major_num, "sbd");
    blk_cleanup_queue(Queue);
    vfree(Device.data);
}

module_init(sbd_init);
module_exit(sbd_exit);
