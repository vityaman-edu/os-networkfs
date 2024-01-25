#ifndef NETWORKFS_LOG_H
#define NETWORKFS_LOG_H

#include "core.h"
#include <linux/printk.h>

#define log_info(fmt, ...) pr_info("[" MODULE_NAME "]: " fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) pr_err("[" MODULE_NAME "]: " fmt, ##__VA_ARGS__)

#endif