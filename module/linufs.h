#ifndef NETWORKFS_LINUFS_H
#define NETWORKFS_LINUFS_H

#include <linux/fs.h>

typedef enum {
  DIRECTORY = DT_DIR,
  REGULAR_FILE = DT_REG,
} INodeType;

typedef int INodeNumber;

#define STRING_MAX_LENGTH 100

// TODO(vityaman): fixed size for simplicity
typedef struct {
  char chars[STRING_MAX_LENGTH + 1];
  size_t length;
  size_t capacity;
} String;

void string_initialize(String* this, size_t capacity);

void string_free(String* this);

typedef struct {
  INodeNumber parent;
  INodeNumber number;
  const char* name;
  INodeType type;
  String content;
} INode;

typedef struct {
  unsigned int count;
  INode** items;
} INodes;

typedef enum {
  OK = 0,
  UNKNOWN = 1,
  NOT_FOUND = 2,
} Status;

/// Initializes LinuFS.
void linufs_initialize(const char* token);

INodeNumber linufs_inode_number_root(void);

/// Creates an inode and returns its number.
INodeNumber linufs_create(INodeNumber parent, const char* name, INodeType type);

/// Removes an inode.
Status linufs_remove(INodeNumber directory, const char* name);

/// Tries to find an inode with th given name in the directory.
INode* linufs_lookup(INodeNumber directory, const char* name);

/// Returns an array of the directory entries.
INodes linufs_list(INodeNumber directory);

/// Disposes `DirectoryEntries` array.
void linufs_inodes_free(INodes entries);

/// Reads `len` bytes from file with number `inode` at `offset`.
ssize_t
linufs_read(INodeNumber inode, char* user_buffer, size_t len, loff_t* offset);

/// Write `len` bytes to file with number `inode` at `offset`.
ssize_t linufs_write(
    INodeNumber inode, const char* user_buffer, size_t len, loff_t* offset
);

#endif