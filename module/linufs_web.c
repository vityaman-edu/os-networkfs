#include "http.h"
#include "linufs.h"
#include "log.h"
#include <linux/slab.h>

#define LINUFS_INODE_NUMBER_ROOT 100
#define LINUFS_INODE_NUMBER_INVALID 0
#define LINUFS_INODE_NUMBER_MIN 111
#define LINUFS_INODE_NUMBER_MAX 2000

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

// http
struct __attribute__((__packed__)) list_response {
  uint32_t count;
  struct __attribute__((__packed__)) entry {
    char name[256];
    uint32_t ino;
    uint8_t type;
  } entries[8];
};

struct __attribute__((__packed__)) lookup_response {
  uint32_t ino;
  uint8_t type;
};

struct __attribute__((__packed__)) create_response {
  uint32_t ino;
};

struct __attribute__((__packed__)) remove_response {};

struct __attribute__((__packed__)) read_response {
  uint32_t size;
  char content[1024];
};

struct __attribute__((__packed__)) write_response {};

static char token_[36];

void linufs_initialize(const char* token) {
  strcpy(token_, token);
  log_info("linufs web: initializing...");
  // Do nothing
  // linufs_create(linufs_inode_number_root(), "test.txt", REGULAR_FILE);
  // linufs_create(linufs_inode_number_root(), "test-dir", DIRECTORY);
  // linufs_create(linufs_inode_number_root(), "test.cpp", REGULAR_FILE);
}

INodeNumber linufs_inode_number_root(void) {
  return LINUFS_INODE_NUMBER_ROOT;
}

/// Creates an inode and returns its number.
INodeNumber
linufs_create(INodeNumber parent, const char* name, INodeType type) {
  char inode_str[11];
  (void)snprintf(inode_str, sizeof(inode_str), "%d", parent);

  char name_enc[255 * 3 + 1];
  encode(name, name_enc);

  char type_str[2];
  (void)snprintf(type_str, sizeof(type_str), "%d", (int)type);

  struct create_response response;
  int64_t code = networkfs_http_call(
      "admin",
      "create",
      (void*)&response,
      sizeof(response),
      3,
      "parent",
      inode_str,
      "name",
      name_enc,
      "type",
      type_str
  );
  if (code != 0) {
    log_error("networkfs_http_call create failed");
    return LINUFS_INODE_NUMBER_INVALID;
  }

  return (INodeNumber)response.ino;
}

/// Removes an inode.
Status linufs_remove(INodeNumber directory, const char* name) {
  char inode_str[11];
  (void)snprintf(inode_str, sizeof(inode_str), "%d", directory);

  char name_enc[255 * 3 + 1];
  encode(name, name_enc);

  struct remove_response response;
  int64_t code = networkfs_http_call(
      "admin",
      "remove",
      (void*)&response,
      sizeof(response),
      2,
      "parent",
      inode_str,
      "name",
      name_enc
  );
  if (code != 0) {
    log_error("networkfs_http_call error code %lld\n", code);
    return UNKNOWN;
  }

  return OK;
}

static void* linufs_memory_allocate(unsigned int size) {
  log_info("linufs: kmalloc called");
  return kmalloc(size, GFP_KERNEL);
}

static void linufs_memory_free(void* ptr) {
  log_info("linufs: kfree called");
  kfree(ptr);
}

/// Tries to find an inode with th given name in the directory.
INode* linufs_lookup(INodeNumber directory, const char* name) {
  char inode_str[11];
  (void)snprintf(inode_str, sizeof(inode_str), "%d", directory);

  char name_enc[255 * 3 + 1];
  encode(name, name_enc);

  struct lookup_response response;
  int64_t code = networkfs_http_call(
      "admin",
      "lookup",
      (void*)&response,
      sizeof(response),
      2,
      "parent",
      inode_str,
      "name",
      name_enc
  );
  if ((code) != 0) {
    log_error("networkfs_http_call error code %lld\n", code);
    return NULL;
  }

  INode* inode = linufs_memory_allocate(sizeof(INode));
  inode->parent = LINUFS_INODE_NUMBER_INVALID;
  inode->number = response.ino;
  inode->type = response.type;
  inode->name = "";
  string_initialize(&inode->content, 0);
  return inode;
}

/// Returns an array of the directory entries.
INodes linufs_list(INodeNumber directory) {
  char inode_str[11];
  (void)snprintf(inode_str, sizeof(inode_str), "%d", directory);

  struct list_response response;
  int64_t code = networkfs_http_call(
      "admin", //
      "list",
      (void*)&response,
      sizeof(response),
      1,
      "inode",
      inode_str
  );
  if (code != 0) {
    log_error("networkfs_http_call error code %lld\n", code);
    return (INodes){.items = NULL, .count = 0};
  }

  INodes inodes;
  inodes.count = response.count;
  inodes.items = linufs_memory_allocate(sizeof(INode*) * inodes.count);

  for (int32_t i = 0; i < response.count; i++) {
    inodes.items[i] = linufs_memory_allocate(sizeof(INode));
    inodes.items[i]->parent = LINUFS_INODE_NUMBER_INVALID;
    inodes.items[i]->name = response.entries[i].name;
    inodes.items[i]->number = response.entries[i].ino;
    inodes.items[i]->type = response.entries[i].type;
    string_initialize(&inodes.items[i]->content, 0);
  }

  return inodes;
}

/// Disposes `DirectoryEntries` array.
void linufs_inodes_free(INodes entries) {
  for (int32_t i = 0; i < entries.count; i++) {
    linufs_memory_free(entries.items[i]);
  }
  linufs_memory_free(entries.items);
}

/// Reads `len` bytes from file with number `inode` at `offset`.
ssize_t
linufs_read(INodeNumber inode, char* user_buffer, size_t len, loff_t* offset) {
  printk(KERN_INFO "Read %d offset %llu\n", inode, *offset);

  char inode_str[11];
  (void)snprintf(inode_str, sizeof(inode_str), "%d", inode);

  int64_t code;
  struct read_response response;
  if ((code = networkfs_http_call(
           "admin",
           "read",
           (void*)&response,
           sizeof(response),
           1,
           "inode",
           inode_str
       ))
      != 0) {
    printk(KERN_INFO "networkfs_http_call error code %lld\n", code);
    return -1;
  }

  if (*offset >= response.size) {
    return 0;
  }

  printk(KERN_INFO "Read content with size %d\n", response.size);

  size_t rest = min(len, (size_t)(response.size - *offset));

  int uncopied = copy_to_user(user_buffer, response.content, rest);
  if (uncopied != 0) {
    printk(
        KERN_INFO "copy_to_user return %d when size was %lu\n", uncopied, rest
    );
  }

  *offset += rest - uncopied;
  return (rest - uncopied);
}

/// Write `len` bytes to file with number `inode` at `offset`.
ssize_t linufs_write(
    INodeNumber inode, const char* user_buffer, size_t len, loff_t* offset
) {
  printk(KERN_INFO "Write %d offset %llu\n", inode, *offset);

  if (*offset >= 1024) {
    return 0;
  }
  len = min(len, (size_t)1023);

  char content[1024];
  int uncopied;
  uncopied = copy_from_user(content, user_buffer, len);
  if (uncopied != 0) {
    printk(
        KERN_INFO "copy_to_user return %d when size was %lu\n", uncopied, len
    );
    return 0;
  }
  content[len] = '\0';

  char inode_str[11];
  (void)snprintf(inode_str, sizeof(inode_str), "%d", inode);

  char content_enc[1023 * 3 + 1];
  encode(content, content_enc);

  int64_t code;
  struct write_response response;
  if ((code = networkfs_http_call(
           "admin",
           "write",
           (void*)&response,
           sizeof(response),
           2,
           "inode",
           inode_str,
           "content",
           content_enc
       ))
      != 0) {
    printk(KERN_INFO "networkfs_http_call error code %lld\n", code);
    return -1;
  }

  printk(KERN_INFO "Wrote\n");

  *offset += len;
  return len;
}