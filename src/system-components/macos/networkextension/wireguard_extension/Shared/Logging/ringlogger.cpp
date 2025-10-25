/* SPDX-License-Identifier: MIT
 *
 * Copyright © 2018-2023 WireGuard LLC. All Rights Reserved.
 */

#include <spdlog/spdlog.h>
#include "ringlogger.h"


// Modified the original ringlogger.c to work via spdlog.

struct log {
    uint32_t magic;
};

void write_msg_to_log(struct log *log, const char *tag, const char *msg)
{
}

int write_log_to_file(const char *file_name, const struct log *input_log)
{
    return 0;
}

uint32_t view_lines_from_cursor(const struct log *input_log, uint32_t cursor, void *ctx, void(*cb)(const char *, uint64_t, void *))
{
    return 0;
}

struct log *open_log(const char *file_name)
{
    return new struct log();
}

void close_log(struct log *log)
{
    if (log)
        delete log;
}
