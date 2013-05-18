#include <linux/module.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/pagemap.h>
#include <linux/blkdev.h>
#include <linux/mpage.h>
#include <linux/buffer_head.h>
#include <linux/kernel.h>
#include <linux/statfs.h>
#include <linux/writeback.h>
#include <linux/bitmap.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artur Guletzky <hatless.fox@gmail.com>");
MODULE_DESCRIPTION("Mimicry FS");

//PAGE_CACHE_SHIFT default is 12 (x86), so page size is 1 << 12 = 4kB 
static int blk_sz_bits = 12; // 4Kb by default
module_param(blk_sz_bits, int, S_IRUGO); //last param is visib in /sys/fs


//hope nobody writes her FS on the same day.
#define MIMFS_MAGIC        0x03072013

#define MIMFS_DEFAULT_MODE 0777

static const struct file_operations mimfs_file_ops;
static const struct inode_operations mimfs_dir_inode_ops;
static const struct address_space_operations mimfs_aops;

//******************************************************************************
// Heart of Mim FS:
// file block number --> logic block number (device block number)

#define MIN_FILE_BLOCK_BITS 9

static sector_t get_total_file_blocks(struct super_block *sb) {
        sector_t total_device_blcks = get_capacity(sb->s_bdev->bd_disk); 

	return (blk_sz_bits > MIN_FILE_BLOCK_BITS) ?
	  total_device_blcks >> (blk_sz_bits - MIN_FILE_BLOCK_BITS) :
	  total_device_blcks;
}

struct fs_meta {
        loff_t fsm_inode_size;
        sector_t fsm_nr_blocks;
        unsigned int fsm_bitmap_size;
        unsigned long * fsm_blocks_bitmap;
} fs_meta;

static void load_fs_meta(struct super_block *sb) {
       struct buffer_head *bh = sb_bread(sb, 0);
       struct fs_meta *meta = (struct fs_meta *) bh->b_data;

       fs_meta.fsm_inode_size = meta->fsm_inode_size;
       fs_meta.fsm_nr_blocks = meta->fsm_nr_blocks;

       //TODO move this to formating
       if (unlikely(fs_meta.fsm_nr_blocks == 0)) {
               fs_meta.fsm_nr_blocks = get_total_file_blocks(sb);
       }
       fs_meta.fsm_bitmap_size = fs_meta.fsm_nr_blocks / BITS_PER_LONG +
	       ((fs_meta.fsm_nr_blocks % BITS_PER_LONG) ? 1 : 0);

       fs_meta.fsm_bitmap_size *= sizeof(unsigned long);

       fs_meta.fsm_blocks_bitmap = vmalloc(fs_meta.fsm_bitmap_size);
       memcpy(fs_meta.fsm_blocks_bitmap, &meta->fsm_blocks_bitmap, fs_meta.fsm_bitmap_size); 
       bitmap_allocate_region(fs_meta.fsm_blocks_bitmap, 0, 0);

       brelse(bh);
}

static void store_fs_meta(struct super_block *sb) {
        struct buffer_head *bh = sb_bread(sb, 0);
	struct fs_meta *meta = (struct fs_meta *) bh->b_data;

	meta->fsm_inode_size  = fs_meta.fsm_inode_size;
	meta->fsm_nr_blocks   = fs_meta.fsm_nr_blocks;
	meta->fsm_bitmap_size = fs_meta.fsm_bitmap_size;
	memcpy(&meta->fsm_blocks_bitmap, fs_meta.fsm_blocks_bitmap, fs_meta.fsm_bitmap_size);
	vfree(fs_meta.fsm_blocks_bitmap);

	mark_buffer_dirty(bh);
	brelse(bh);
}

static int mimfs_get_block(struct inode *inode,    //file's inode
			   sector_t iblock,        //file's block number
			   struct buffer_head *bh, //bh to initalize
			   int create) {           //XX what is precise meaning?
	sector_t total_file_blcks   = get_total_file_blocks(inode->i_sb);
	sector_t mapped_sector      = 0;

	//now we assuming that file is single.
	if (iblock >= total_file_blcks) { return -ENOSPC; }

	mapped_sector = total_file_blcks - iblock - 1;

	map_bh(bh, inode->i_sb, mapped_sector);

	//printk("MIMFS: Block with number %lu is required out of %lu. Mapped to %lu\n",
        //    iblock, total_file_blcks, mapped_sector);
	if (!buffer_mapped(bh)) {
 	        printk("MIMFS: Buffer head hasn't been mapped for block with number %lu\n", iblock); 
	        return -1;
	}
	
	//assume every mapped blocks as busy. Probably there is a better way to track blocks
	// plain bit set would be OK, but it would add unnecessary ``arch'' dependency
	bitmap_allocate_region(fs_meta.fsm_blocks_bitmap, mapped_sector, 0);

        return 0;
}

//******************************************************************************
// Custom address space operations. mimfs_get_block callback is set here

//read cached logic 
static int mimfs_readpage(struct file *file, struct page *page) {
        return mpage_readpage(page, mimfs_get_block);
}

static int mimfs_readpages(struct file *file, struct address_space *mapping,
			   struct list_head *pages, unsigned nr_pages) {
        return mpage_readpages(mapping, pages, nr_pages, mimfs_get_block);
}

//write cached logic
static int mimfs_writepage(struct page *page, struct writeback_control *wbc) {
        return block_write_full_page(page, mimfs_get_block, wbc);
}

static int mimfs_writepages(struct address_space *mapping,
			    struct writeback_control *wbc) {
        return mpage_writepages(mapping, wbc, mimfs_get_block);
}

static int mimfs_write_begin(struct file *file, struct address_space *mapping,
			     loff_t pos, unsigned len, unsigned flags,
			     struct page **pagep, void **fsdata) {
        return block_write_begin(mapping, pos, len, flags,
				 pagep, mimfs_get_block);
}

static ssize_t mimsf_direct_IO(int rw, struct kiocb *iocb,
			       const struct iovec *iov,
			       loff_t offset, unsigned long nr_segs) {
        return blockdev_direct_IO(rw, iocb, 
				  iocb->ki_filp->f_mapping->host,
				  iov, offset, nr_segs, mimfs_get_block);
}

static sector_t mimfs_bmap(struct address_space *mapping, sector_t block) {
        return generic_block_bmap(mapping, block, mimfs_get_block);
}

static const struct address_space_operations mimfs_aops = {
        .readpage    = mimfs_readpage,
        .readpages   = mimfs_readpages,
        .writepage   = mimfs_writepage,
        .writepages  = mimfs_writepages,
        .write_begin = mimfs_write_begin,
        .write_end   = generic_write_end,
        .direct_IO   = mimsf_direct_IO,
	.bmap        = mimfs_bmap,
};

//******************************************************************************
// inode related operations

struct inode * data_file_inode;

static struct inode *mimfs_get_inode(struct super_block *sb,
                                     const struct inode *dir,
                                     umode_t mode) {

        struct inode *inode = new_inode(sb); // @ inode.c:931
	if (!inode) { return inode; }

	//TODO setup with some configurable values
	//inode->i_mapping->a_ops
        //inode->i_mapping->backing_dev_info

	inode_init_owner(inode, dir, mode);

	// ** cant we use same time for measurements?
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_mapping->a_ops = &mimfs_aops;

        switch (mode & S_IFMT) {
        case S_IFREG: //regular file
                //inode->i_op
	        data_file_inode = inode;
		inode->i_ino = 42;
		inode->i_size = fs_meta.fsm_inode_size;
                inode->i_fop = &mimfs_file_ops;
                break;
        case S_IFDIR: //directory
                inode->i_op  = &mimfs_dir_inode_ops;
		inode->i_fop = &simple_dir_operations;
                break;
        }

	return inode;
}

static int mimfs_file_create(struct inode *parent_dir, struct dentry *dentry, umode_t mode, bool excl) {
        //since we share data between inodes -- we use single inode.
        // To prevent having multiple inodes backed by same storage area, we just create a hard link to data inode

        inc_nlink(data_file_inode);
        ihold(data_file_inode);
 	
	//also transter inode reference to dentry
	d_instantiate(dentry, data_file_inode);
	dget(dentry);
        return 0;
}

static const struct inode_operations mimfs_dir_inode_ops = {
        .create = mimfs_file_create,
	.unlink = simple_unlink,
       	.lookup = simple_lookup, //TODO override 
};

//******************************************************************************
// File related operations

static const struct file_operations mimfs_file_ops = {
        .read           = do_sync_read,
        .aio_read       = generic_file_aio_read,
        .write          = do_sync_write,
        .aio_write      = generic_file_aio_write,
	.mmap           = generic_file_mmap,
	.fsync          = generic_file_fsync,
	.llseek         = generic_file_llseek,
  /*	.open	= mimfs_open,
	.read 	= mimfs_read_file,
	.write  = mimfs_write_file,
  */
};

//******************************************************************************
// Superblock related operations

static void printdbg_blocks_bitmask(void) {
        unsigned int long_i = 0;
	unsigned int bit = 0;

	//I know about ``bitmap_scnprintf'' but I'm not too smart to use it
	//both in code and in ``look through'' analysis
	printk("Blocks bitmap:\n");
	while (long_i < fs_meta.fsm_bitmap_size / sizeof(unsigned long)) {
	        printk("%c", (*(fs_meta.fsm_blocks_bitmap + long_i) & (1 << bit)) ? 'X' : 'O');
		bit++;
		if (bit == BITS_PER_LONG) {
		        bit = 0;
			long_i++;
		}
	}
	printk("\n");
}

static int mimfs_statfs (struct dentry * dentry, struct kstatfs * buf) {
        struct super_block *sb = dentry->d_sb;

	printdbg_blocks_bitmask();

        buf->f_type = MIMFS_MAGIC;
        buf->f_bsize = sb->s_blocksize;
        buf->f_blocks = get_total_file_blocks(sb);
	// one is for metadata block
        buf->f_bfree = buf->f_blocks - 
	  bitmap_weight(fs_meta.fsm_blocks_bitmap, fs_meta.fsm_nr_blocks);
        buf->f_bavail = buf->f_bfree;
        buf->f_files = data_file_inode->i_nlink;;
        //buf->f_ffree =
        //buf->f_namelen =

        return 0;
}
  
static struct super_operations mimfs_super_ops = {
  //        .statfs         = simple_statfs,           //standard
        .drop_inode     = generic_delete_inode, //standard
	.statfs         = mimfs_statfs,
};

static void setup_qname(const char * name, struct qstr * qname) {
        qname->name = name;
	qname->len  = strlen(name);
	qname->hash = full_name_hash(name, qname->len);
}

static int create_file(struct inode *parent, struct dentry *pdentry, 
	  	       const char *name) {
        struct inode  *file_inode;
	struct qstr    file_name;
	struct dentry *file_dentry;	

	setup_qname(name, &file_name);
	if (!(file_dentry = d_alloc(pdentry, &file_name))) { return -1; }

	file_inode = mimfs_get_inode(parent->i_sb, parent,
				     S_IFREG | MIMFS_DEFAULT_MODE);
	if (file_inode) {
	        d_add(file_dentry, file_inode);
	}

	return file_inode ? 0 : -1;
}

//init super block
static int mimfs_fill_super(struct super_block *sb, void *data, int silent) {
        struct inode *root_node;

	//printk("MIMFS: Super block parsing started\n");
        //** sb->s_maxbytes    = max file size, do we need it? 
        sb->s_blocksize      = 1 << blk_sz_bits;
        sb->s_blocksize_bits = blk_sz_bits; //how many bits in block size
        sb->s_magic          = MIMFS_MAGIC;
        sb->s_op             = &mimfs_super_ops;
        sb->s_time_gran      = 1; //nanoseconds is time granularity
        
	if (blk_sz_bits < MIN_FILE_BLOCK_BITS) {
	        printk(KERN_WARNING"FS block size is suppoused to be >= 512 bytes. Current values is %u", 1 << blk_sz_bits);
	 }
        //create root diretory
        root_node = mimfs_get_inode(sb, NULL, S_IFDIR | MIMFS_DEFAULT_MODE);

        //put root directory in cache
        sb->s_root = d_make_root(root_node);
        if (!sb->s_root) { return -1; }
        
	//printk("MIMFS: Root dir was initalized\n");
	load_fs_meta(sb);
	create_file(root_node, sb->s_root, "data");
	
        return 0;
}

struct dentry* mimfs_mount(struct file_system_type *fst, int flags,
			   const char *dev_name, void *data) {
        return mount_bdev(fst, flags, dev_name, data, mimfs_fill_super);
}

static void mimfs_kill_block_super(struct super_block *sb) {
        fs_meta.fsm_inode_size = data_file_inode->i_size; 
        store_fs_meta(sb);
        write_inode_now(data_file_inode, 1);
        kill_litter_super(sb);
}


//******************************************************************************
// FS initalization logic and data structores

static struct file_system_type mimfs_fs_type = {
        .owner   = THIS_MODULE,
        .name    = "mimfs",
	.mount   = mimfs_mount,
	.kill_sb = mimfs_kill_block_super,
};

static int __init fs_init(void) { 
        return register_filesystem(&mimfs_fs_type);
}

static void __exit fs_exit(void) {
        unregister_filesystem(&mimfs_fs_type); 
}

module_init(fs_init);
module_exit(fs_exit);
