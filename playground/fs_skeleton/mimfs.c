#include <linux/module.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/pagemap.h>
#include <linux/blkdev.h>
#include <linux/mpage.h>
#include <linux/buffer_head.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artur Guletzky <hatless.fox@gmail.com>");
MODULE_DESCRIPTION("Mimicry FS");

//hope nobody writes her FS on the same day.
#define MIMFS_MAGIC        0x03072013

#define MIMFS_DEFAULT_MODE 0777

static const struct file_operations mimfs_file_ops;
static const struct inode_operations mimfs_dir_inode_ops;
static const struct address_space_operations mimfs_aops;

//******************************************************************************
// Heart of Mim FS:
// file block number --> logic block number (device block number)

static int mimfs_get_block(struct inode *inode,    //file's inode
			   sector_t iblock,        //file's block number
			   struct buffer_head *bh, //bh to initalize
			   int create) {           //XX what is precise meaning?
        //TODO null check
  
        sector_t total_device_blcks = get_capacity(inode->i_sb->s_bdev->bd_disk); 
	//printk("Total device block count is %lu\n ", total_device_blcks);
        printk("MIMFS: Block with number %lu is required\n", iblock); 
  	
	//now we assuming that file is single.
	if (iblock > total_device_blcks) { return -ENOSPC; }
	//	bh = sb_getblk(inode->i_sb, total_device_blcks - iblock);
	map_bh(bh, inode->i_sb, total_device_blcks - iblock);
	if (!buffer_mapped(bh)) {
	    printk("MIMFS: Buffer head hasn't been mapped for block with number %lu\n", iblock); 
	    return -1;
	}

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

static int mimfs_write_end(struct file *file, struct address_space *mapping,
			  loff_t pos, unsigned len, unsigned copied,
			  struct page *page, void *fsdata) {
        return generic_write_end(file, mapping, pos, len, copied, page, fsdata);
 }

// direct io logic
static ssize_t mimsf_direct_IO(int rw, struct kiocb *iocb,
			       const struct iovec *iov,
			       loff_t offset, unsigned long nr_segs) {
        return blockdev_direct_IO(rw, iocb, 
				  iocb->ki_filp->f_mapping->host,
				  iov, offset, nr_segs, mimfs_get_block);
}

static const struct address_space_operations mimfs_aops = {
        .readpage    = mimfs_readpage,
        .readpages   = mimfs_readpages,
        .writepage   = mimfs_writepage,
        .writepages  = mimfs_writepages,
        .write_begin = mimfs_write_begin,
        .write_end   = mimfs_write_end,
        .direct_IO   = mimsf_direct_IO,
};

//******************************************************************************
// inode related operations

static struct inode *mimfs_get_inode(struct super_block *sb,
                                     const struct inode *dir,
                                     umode_t mode,
                                     dev_t dev) {

        struct inode *inode = new_inode(sb); // @ inode.c:931
	if (!inode) { return inode; }

	//TODO setup with some configurable values
	//inode->i_mapping->a_ops
        //inode->i_mapping->backing_dev_info

	inode_init_owner(inode, dir, mode);
	//TODO make configurable
	inode->i_blocks = 0;
	// ** cant we use same time for measurements?
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	inode->i_mapping->a_ops = &mimfs_aops;
        //TODO maybe we require some counter to store in i_private
        switch (mode & S_IFMT) {
        case S_IFREG: //regular file
                //inode->i_op
                inode->i_fop = &mimfs_file_ops;
		printk("MIMFS: File node was created\n");
                break;
        case S_IFDIR: //directory
       	        printk("MIMFS: Directory node was created\n");
                inode->i_op  = &mimfs_dir_inode_ops;
                inode->i_fop = &simple_dir_operations;
                //link number is 2. for "." entry
                inc_nlink(inode);
                break;
        }

	return inode;
}

//file creation
static int mimfs_inode_mknode(struct inode *parent_dir, struct dentry *dentry, umode_t mode, dev_t dev) {
        //create new inode
        struct inode *inode = 
	    mimfs_get_inode(parent_dir->i_sb, parent_dir, mode, dev);
	if (!inode) { return -ENOSPC; }
	
	d_instantiate(dentry, inode);
	dget(dentry); //from ramfs, TODO investigate why?
	//TODO doest caller attaches created node to parental dentry? 
        return 0;
}


static int mimfs_inode_create(struct inode *parent_dir, struct dentry *dentry, umode_t mode, bool excl) {
       return mimfs_inode_mknode(parent_dir, dentry, mode | S_IFREG, 0);
}

static const struct inode_operations mimfs_dir_inode_ops = {
        .create = mimfs_inode_create,
        .mknod  = mimfs_inode_mknode,
	.lookup = simple_lookup,
};

//******************************************************************************
// File related operations

static void setup_qname(const char * name, struct qstr * qname) {
        qname->name = name;
	qname->len  = strlen(name);
	qname->hash = full_name_hash(name, qname->len);
}

static int mimfs_create_file(struct inode *parent, struct dentry *pdentry, 
			     const char *name) {
        struct inode  *file_inode;
	struct qstr    file_name;
	struct dentry *file_dentry;

	setup_qname(name, &file_name);
	if (!(file_dentry = d_alloc(pdentry, &file_name))) { return -1; }

	file_inode = mimfs_get_inode(parent->i_sb, parent,
				     S_IFREG | MIMFS_DEFAULT_MODE, 0);
	if (file_inode) {
	  d_add(file_dentry, file_inode);
	}
	return file_inode ? 0 : -1;
}


static const struct file_operations mimfs_file_ops = {
        .read           = do_sync_read,
        .aio_read       = generic_file_aio_read,
        .write          = do_sync_write,
        .aio_write      = generic_file_aio_write,
  /*	.open	= mimfs_open,
	.read 	= mimfs_read_file,
	.write  = mimfs_write_file,
  */
};

//******************************************************************************
// Superblock related operations

static struct super_operations mimfs_super_ops = {
        .statfs = simple_statfs,           //standard
        .drop_inode = generic_delete_inode, //standard
};

//#define MIMFS_BLOCK_SIZE       PAGE_CACHE_SIZE

//PAGE_CACHE_SHIFT default is 12 (x86), so page size is 1 << 12 = 4kB 
// set 4 bytes file block size
#define MIMFS_BLOCK_SIZE_BITS  2 /*PAGE_CACHE_SHIFT*/

//init super block
static int mimfs_fill_super(struct super_block *sb, void *data, int silent) {
        struct inode *root_node;

	printk("MIMFS: Super block parsing started\n");
        //** sb->s_maxbytes    = max file size, do we need it? 
        sb->s_blocksize      = 1 << MIMFS_BLOCK_SIZE_BITS;
        sb->s_blocksize_bits = MIMFS_BLOCK_SIZE_BITS; //how many bits in block size
        sb->s_magic          = MIMFS_MAGIC;
        sb->s_op             = &mimfs_super_ops;
        sb->s_time_gran      = 1;
        
        //create root diretory
        root_node = mimfs_get_inode(sb, NULL, S_IFDIR | MIMFS_DEFAULT_MODE, 0);

        //put root directory in cache
        sb->s_root = d_make_root(root_node);
        if (!sb->s_root) { return -1; }
        
	printk("MIMFS: Root dir was initalized\n");
	//smoketest
	mimfs_create_file(root_node, sb->s_root, "ITS_ALIVE!!!");
	
        return 0;
}

struct dentry* mimfs_mount(struct file_system_type *fst, int flags,
			   const char *dev_name, void *data) {
        printk("MIMFS: Mount Started\n");
        // @ super.c:952
        return mount_bdev(fst, flags, dev_name, data, mimfs_fill_super);
}

//******************************************************************************
// FS initalization logic and data structores

static struct file_system_type mimfs_fs_type = {
        .owner   = THIS_MODULE,
        .name    = "mimfs",
	.mount   = mimfs_mount,
	.kill_sb = kill_litter_super, //VFS default
};

static int __init fs_init(void) { 
        return register_filesystem(&mimfs_fs_type);
}

static void __exit fs_exit(void) {
        unregister_filesystem(&mimfs_fs_type); 
}

module_init(fs_init);
module_exit(fs_exit);
