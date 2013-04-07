#include <linux/module.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/pagemap.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artur Guletzky <hatless.fox@gmail.com>");
MODULE_DESCRIPTION("Mimicry FS");

//hope nobody writes her FS on the same day.
#define MIMFS_MAGIC        0x03072013

#define MIMFS_DEFAULT_MODE 0777

static struct file_operations mimfs_file_ops;

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
        
        //TODO maybe we require some counter to store in i_private
        switch (mode & S_IFMT) {
        case S_IFREG: //regular file
                //inode->i_op
                inode->i_fop = &mimfs_file_ops;
                break;
        case S_IFDIR: //directory
                inode->i_op  = &simple_dir_inode_operations;
                inode->i_fop = &simple_dir_operations;
                //link number is 2.
                inc_nlink(inode);
                break;
        }

	return inode;
}

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


//empty now
static struct file_operations mimfs_file_ops = {
//	.open	= mimfs_open,
//	.read 	= mimfs_read_file,
//	.write  = mimfs_write_file,
};


//******************************************************************************
// Superblock related operations

static struct super_operations mimfs_super_ops = {
        .statfs = simple_statfs,           //standard
        .drop_inode = generic_delete_inode, //standard
};

//init super block
static int mimfs_fill_super(struct super_block *sb, void *data, int silent) {
        struct inode *root_node;

        //** sb->s_maxbytes    = do we need it?
        sb->s_blocksize    = PAGE_CACHE_SIZE;
        sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
        sb->s_magic         = MIMFS_MAGIC;
        sb->s_op            = &mimfs_super_ops;
        sb->s_time_gran     = 1;
        
        //create root diratory
        root_node = mimfs_get_inode(sb, NULL, S_IFDIR | MIMFS_DEFAULT_MODE, 0);

        //put root directory in cache
        sb->s_root = d_make_root(root_node);
        if (!sb->s_root) { return -1; }
        
	//smoketest
	mimfs_create_file(root_node, sb->s_root, "ITS_ALIVE!!!");
	
        return 0;
}

struct dentry* mimfs_mount(struct file_system_type *fst, int flags,
			   const char *dev_name, void *data) {
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
