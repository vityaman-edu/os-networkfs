#include "linufs.h"
#include "log.h"
#include <linux/slab.h>

#define LINUFS_INODE_NUMBER_ROOT 1000
#define LINUFS_INODE_NUMBER_INVALID 0
#define LINUFS_INODE_NUMBER_MIN 1111
#define LINUFS_INODE_NUMBER_MAX 2000

static INodeNumber linufs_inode_number_next = LINUFS_INODE_NUMBER_MIN;
static INode linufs_inodes[LINUFS_INODE_NUMBER_MAX + 1] = {0};

void linufs_initialize(void) {
  log_info("linufs: initializing...");
  linufs_create(linufs_inode_number_root(), "test.txt", REGULAR_FILE);
  linufs_create(linufs_inode_number_root(), "test-dir", DIRECTORY);
  linufs_create(linufs_inode_number_root(), "test.cpp", REGULAR_FILE);
}

INodeNumber linufs_inode_number_root(void) {
  return LINUFS_INODE_NUMBER_ROOT;
}

static bool linufs_inode_is_free(INode* inode) {
  return inode->number == LINUFS_INODE_NUMBER_INVALID;
}

static INode* linufs_find(INodeNumber number) {
  if (linufs_inode_number_next <= number) {
    return NULL;
  }
  if (number < LINUFS_INODE_NUMBER_MIN) {
    return NULL;
  }
  INode* inode = &linufs_inodes[number];
  if (linufs_inode_is_free(inode)) {
    return NULL;
  }
  return inode;
}

INodeNumber
linufs_create(INodeNumber parent, const char* name, INodeType type) {
  log_info("linufs: creating inode (%d, %s, %d)...", parent, name, type);

  // TODO: add assertion for the `linufs_next_inode_number`
  INode* inode = &linufs_inodes[linufs_inode_number_next];
  inode->parent = parent;
  inode->number = linufs_inode_number_next;
  inode->name = name;
  inode->type = type;
  return linufs_inode_number_next++;
}

Status linufs_remove(INodeNumber inode_number) {
  INode* inode = linufs_find(inode_number);
  if (inode == NULL) {
    return NOT_FOUND;
  }

  // TODO: memory leak, do recursive removal

  inode->name = NULL;
  inode->number = LINUFS_INODE_NUMBER_INVALID;
  inode->parent = LINUFS_INODE_NUMBER_INVALID;
  inode->type = 0;

  return OK;
}

INode* linufs_lookup(INodeNumber directory, const char* name) {
  log_info("linufs: looking up...");
  const INodeNumber min = LINUFS_INODE_NUMBER_MIN;
  const INodeNumber max = linufs_inode_number_next;
  for (INodeNumber i = min; i < max; ++i) {
    INode* inode = linufs_find(i);
    if (inode != NULL &&              //
        inode->parent == directory && //
        strcmp(inode->name, name) == 0) {
      return inode;
    }
  }
  return NULL;
}

static unsigned int linufs_list_count(INodeNumber directory) {
  log_info("linufs: list counting...");

  unsigned int count = 0;

  const INodeNumber min = LINUFS_INODE_NUMBER_MIN;
  const INodeNumber max = linufs_inode_number_next;
  for (INodeNumber i = min; i < max; ++i) {
    INode* inode = linufs_find(i);
    if (inode != NULL && inode->parent == directory) {
      count += 1;
    }
  }

  return count;
}

static void* linufs_memory_allocate(unsigned int size) {
  log_info("linufs: kmalloc called");
  return kmalloc(size, GFP_KERNEL);
}

static void linufs_memory_free(void* ptr) {
  log_info("linufs: kfree called");
  kfree(ptr);
}

INodes linufs_list(INodeNumber directory) {
  INodes inodes = {0};
  inodes.count = linufs_list_count(directory);
  inodes.items = linufs_memory_allocate(inodes.count * sizeof(INode*));

  log_info("linufs: looking listing...");

  const INodeNumber min = LINUFS_INODE_NUMBER_MIN;
  const INodeNumber max = linufs_inode_number_next;
  unsigned int next = 0;
  for (INodeNumber i = min; i < max; ++i) {
    INode* inode = linufs_find(i);
    if (inode != NULL && inode->parent == directory) {
      inodes.items[next++] = inode;
    }
  }

  return inodes;
}

void linufs_inodes_free(INodes entries) {
  linufs_memory_free(entries.items);
}
