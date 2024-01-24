#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>

#define MODULE_NAME "networkfs"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vityaman");
MODULE_DESCRIPTION("A simple Network File System");
MODULE_VERSION("0.0.1");

#define log_info(fmt, ...) pr_info("[" MODULE_NAME "]: " fmt, ##__VA_ARGS__)

#define log_error(fmt, ...) pr_err("[" MODULE_NAME "]: " fmt, ##__VA_ARGS__)

#define NETWORKFS_ROOT_INODE 1000

static int __init nfs_init(void);

static void __exit nfs_exit(void);

struct dentry* nfs_mount(
    struct file_system_type* fs_type, int flags, const char* token, void* data
);

int nfs_fill_super(struct super_block* sb, void* data, int silent);

struct inode* nfs_get_inode(
    struct super_block* sb, const struct inode* dir, umode_t mode, int i_ino
);

void nfs_kill_sb(struct super_block* sb);

struct dentry* nfs_lookup(
    struct inode* parent_inode, struct dentry* child_dentry, unsigned int flag
);

struct file_system_type nfs_fs_type = {
    .name = MODULE_NAME,
    .mount = nfs_mount,
    .kill_sb = nfs_kill_sb,
};

struct inode_operations nfs_inode_ops = {
    .lookup = nfs_lookup,
};

struct dentry* nfs_mount(
    struct file_system_type* fs_type, int flags, const char* token, void* data
) {
  struct dentry* dir = mount_nodev(fs_type, flags, data, nfs_fill_super);
  if (dir == NULL) {
    log_error("Can't mount file system");
  } else {
    log_info("Mounted successfuly");
  }
  return dir;
}

int nfs_fill_super(struct super_block* sb, void* data, int silent) {
  struct inode* inode
      = nfs_get_inode(sb, /*dir=*/NULL, S_IFDIR, NETWORKFS_ROOT_INODE);
  sb->s_root = d_make_root(inode);
  if (sb->s_root == NULL) {
    return -ENOMEM;
  }
  log_info("fill_super returns 0");
  return 0;
}

struct inode* nfs_get_inode(
    struct super_block* sb, const struct inode* dir, umode_t mode, int i_ino
) {
  struct inode* inode = new_inode(sb);
  if (inode == NULL) {
    return NULL;
  }

  inode->i_ino = i_ino;
  inode->i_op = &nfs_inode_ops;
  inode_init_owner(&init_user_ns, inode, dir, mode | S_IRWXUGO);
  return inode;
}

void nfs_kill_sb(struct super_block* sb) {
  log_info("Super block is destroyed. Unmounted successfully.");
}

struct dentry* nfs_lookup(
    struct inode* parent_inode, struct dentry* child_dentry, unsigned int flag
) {
  log_info(
      "Lookup at inode %lu for a %s",
      parent_inode->i_ino,
      child_dentry->d_name.name
  );

  struct inode* inode = nfs_get_inode(
      parent_inode->i_sb,
      /*dir=*/NULL,
      /*mode=*/S_IFDIR,
      /*i_ino=*/NETWORKFS_ROOT_INODE
  );

  d_add(child_dentry, inode);

  return NULL;
}

static int __init nfs_init(void) {
  const int code = register_filesystem(&nfs_fs_type);
  if (code != 0) {
    log_error("Registration failed with code %d", code);
    return code;
  }
  log_info("Registered the filesystem successfully!");
  return 0;
}

static void __exit nfs_exit(void) {
  const int code = unregister_filesystem(&nfs_fs_type);
  if (code != 0) {
    log_error("Unregistration failed with code %d", code);
    return;
  }
  log_info("Unregistered the filesystem successfully");
}

module_init(nfs_init);
module_exit(nfs_exit);
