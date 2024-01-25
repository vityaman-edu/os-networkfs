#include "linufs.h"
#include "log.h"
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("vityaman");
MODULE_DESCRIPTION("A simple Network File System");
MODULE_VERSION("0.0.1");

#define NETWORKFS_FILENAME_LENGTH 32

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

int nfs_iterate(struct file* filp, struct dir_context* ctx);

struct file_system_type nfs_fs_type = {
    .name = MODULE_NAME,
    .mount = nfs_mount,
    .kill_sb = nfs_kill_sb,
};

struct inode_operations nfs_inode_ops = {
    .lookup = nfs_lookup,
};

struct file_operations nfs_dir_ops = {
    .iterate = nfs_iterate,
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
  const INodeNumber root = linufs_inode_number_root();
  struct inode* inode = nfs_get_inode(sb, /*dir=*/NULL, S_IFDIR, root);

  sb->s_root = d_make_root(inode);
  if (sb->s_root == NULL) {
    log_error("fill_super returned NO MEMORY");
    return -ENOMEM;
  }

  log_info("fill_super returns OK");
  return 0;
}

struct inode* nfs_get_inode(
    struct super_block* sb, const struct inode* dir, umode_t mode, int i_ino
) {
  struct inode* inode = new_inode(sb);
  if (inode == NULL) {
    log_error("get inode returned null");
    return NULL;
  }

  inode->i_ino = i_ino;
  inode->i_op = &nfs_inode_ops;
  inode->i_fop = &nfs_dir_ops;

  inode_init_owner(&init_user_ns, inode, dir, mode | S_IRWXUGO);

  log_info("get inode with number %lu", inode->i_ino);
  return inode;
}

void nfs_kill_sb(struct super_block* sb) {
  log_info("Super block is destroyed. Unmounted successfully.");
}

struct dentry* nfs_lookup(
    struct inode* parent_inode, struct dentry* child_dentry, unsigned int flag
) {
  const unsigned long parent = parent_inode->i_ino;
  const char* name = child_dentry->d_name.name;

  log_info("Looking at inode %lu for a %s", parent, name);

  INode* linode = linufs_lookup((INodeNumber)parent, name);
  if (linode == NULL) {
    log_error("Not found inode with name %s at parent %lu", name, parent);
    return NULL;
  }

  const umode_t mode = (linode->type == REGULAR_FILE) ? S_IFREG : S_IFDIR;
  struct inode* inode = nfs_get_inode(
      parent_inode->i_sb,
      /*dir=*/NULL,
      mode,
      linode->number
  );

  d_add(child_dentry, inode);

  return NULL;
}

int nfs_iterate(struct file* filp, struct dir_context* ctx) {
  log_info("iteratation started");

  struct dentry* directory = filp->f_path.dentry;
  const INodeNumber directory_inode_number
      = (INodeNumber)directory->d_inode->i_ino;

  long long offset = filp->f_pos;

  if (offset == 0) {
    dir_emit(ctx, ".", strlen("."), directory->d_inode->i_ino, DT_DIR);
    log_info("emit entry[%lld] is '%s'", offset, ".");
    ctx->pos += 1;
    offset += 1;
  }

  if (offset == 1) {
    dir_emit(ctx, "..", strlen(".."), directory->d_inode->i_ino, DT_DIR);
    log_info("emit entry[%lld] is '%s'", offset, "..");
    ctx->pos += 1;
    offset += 1;
  }

  INodes inodes = linufs_list(directory_inode_number);

  for (long long i = offset - 2; i < inodes.count; ++i) {
    const INode* inode = inodes.items[i];
    log_info("emit entry[%lld] is '%s'", i + 2, inode->name);
    const bool is_ok = dir_emit(
        ctx,
        inode->name, //
        strlen(inode->name),
        inode->number,
        inode->type
    );
    if (!is_ok) {
      log_error("dir emit failed");
    }
    ctx->pos += 1;
  }

  linufs_inodes_free(inodes);

  log_error("iterate count is %lld", ctx->pos - filp->f_pos);
  return (int)(ctx->pos - filp->f_pos);
}

static int __init nfs_init(void) {
  const int code = register_filesystem(&nfs_fs_type);
  if (code != 0) {
    log_error("Registration failed with code %d", code);
    return code;
  }
  linufs_initialize();
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
