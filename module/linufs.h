#ifndef NETWORKFS_LINUFS_H
#define NETWORKFS_LINUFS_H

#include <linux/fs.h>

typedef enum {
  DIRECTORY = DT_DIR,
  REGULAR_FILE = DT_REG,
} INodeType;

typedef int INodeNumber;

typedef struct {
  INodeNumber parent;
  INodeNumber number;
  const char* name;
  INodeType type;
} INode;

typedef struct {
  unsigned int count;
  INode** items;
} INodes;

/// Initializes LinuFS.
void linufs_initialize(void);

INodeNumber linufs_inode_number_root(void);

/// Creates an inode and returns its number.
INodeNumber linufs_create(INodeNumber parent, const char* name, INodeType type);

/// Tries to find an inode with th given name in the directory.
INode* linufs_lookup(INodeNumber directory, const char* name);

/// Returns an array of the directory entries.
INodes linufs_list(INodeNumber directory);

/// Disposes `DirectoryEntries` array.
void linufs_inodes_free(INodes entries);

#endif