#include "linufs.h"
#include "log.h"
#include <linux/slab.h>

#define LINUFS_INODE_NUMBER_ROOT 1000
#define LINUFS_INODE_NUMBER_INVALID 0
#define LINUFS_INODE_NUMBER_MIN 1111
#define LINUFS_INODE_NUMBER_MAX 2000

static INodeNumber linufs_inode_number_next = LINUFS_INODE_NUMBER_MIN;
static INode linufs_inodes[LINUFS_INODE_NUMBER_MAX + 1] = {0};

void string_initialize(String* this, size_t capacity) {
  this->length = 0;
  this->capacity = capacity;
  this->chars[this->length] = '\0';
}

void string_free(String* this) {
  this->length = 0;
  this->capacity = 0;
  this->chars[this->length] = '\0';
}

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
  string_initialize(&inode->content, STRING_MAX_LENGTH);
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
  string_free(&inode->content);

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

ssize_t
linufs_read(INodeNumber inode, char* user_buffer, size_t len, loff_t* offset) {
  if (user_buffer == NULL || offset == NULL) {
    return -1;
  }

  log_info("read %lu chars from inode %d at offset %lld", len, inode, *offset);

  const INode* linode = linufs_find(inode);
  if (linode == NULL) {
    return -1;
  }

  if (linode->content.length <= *offset) {
    return 0;
  }

  const loff_t remaining = min(len, linode->content.length - *offset);
  const loff_t uncopied = (loff_t
  )copy_to_user(user_buffer, linode->content.chars + *offset, remaining);
  if (uncopied != 0) {
    log_error(
        "copy_to_user returned %lld when remaining was %lld\n", //
        uncopied,
        remaining
    );
    return 0;
  }

  *offset += remaining - uncopied;
  return remaining - uncopied;
}

ssize_t linufs_write(
    INodeNumber inode, const char* user_buffer, size_t len, loff_t* offset
) {
  if (user_buffer == NULL || offset == NULL) {
    return -1;
  }

  log_info("write %lu chars to inode %d at offset %lld", len, inode, *offset);

  INode* linode = linufs_find(inode);
  if (linode == NULL) {
    return -1;
  }

  if (linode->content.capacity <= *offset) {
    return 0;
  }

  const loff_t remaining = min(len, linode->content.capacity - *offset);
  const loff_t uncopied = (loff_t
  )copy_from_user((linode->content.chars + *offset), user_buffer, remaining);
  if (uncopied != 0) {
    log_error(
        "copy_from_user returned %lld when remaining was %lld\n", //
        uncopied,
        remaining
    );
    return 0;
  }

  linode->content.length = *offset + remaining;
  linode->content.chars[linode->content.length] = '\0';

  log_info("Written");

  *offset += remaining;
  return remaining;
}
