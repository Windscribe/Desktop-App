#ifndef RINGLOGGER_C_H
#define RINGLOGGER_C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct log;
void write_msg_to_log(struct log *log, const char *tag, const char *msg);
int write_log_to_file(const char *file_name, const struct log *input_log);
uint32_t view_lines_from_cursor(const struct log *input_log, uint32_t cursor, void *ctx, void(*)(const char *, uint64_t, void *));
struct log *open_log(const char *file_name);
void close_log(struct log *log);

#ifdef __cplusplus
}
#endif

#endif /* RINGLOGGER_C_H */
