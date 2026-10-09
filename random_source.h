/*
 * random_source - 随机数据源模块
 *
 * 提供统一的数据源抽象：
 *   - 内置 PRNG 生成器（默认，xorshift32，时间+进程号做种子）
 *   - 普通文件（数据不足时从头循环填充）
 *   - 字符设备（如 /dev/random、/dev/urandom、/dev/zero，顺序读取）
 *   - 块设备（如 /dev/sda，随机偏移读取部分位置，可能需要 root 权限）
 */
#ifndef RANDOM_SOURCE_H
#define RANDOM_SOURCE_H

#include <stddef.h>
#include <stdint.h>

/* 数据源类型 */
typedef enum {
    RS_GENERATOR = 0,   /* 内置生成器 */
    RS_FILE,            /* 普通文件 */
    RS_CHARDEV,         /* 字符设备 */
    RS_BLOCKDEV         /* 块设备 */
} rs_type_t;

typedef struct {
    rs_type_t    type;
    int          fd;     /* 文件/设备描述符；内置生成器时为 -1 */
    uint64_t     size;   /* 文件/块设备字节大小；字符设备为 0 表示无限 */
    uint64_t     pos;    /* 普通文件当前读位置（用于循环填充） */
    uint32_t     seed;   /* PRNG 状态（内置生成器 / 块设备随机偏移共用） */
    const char  *name;   /* 数据源名称（用于错误提示） */
} random_source_t;

/* 打开数据源；source 为 NULL 或空串时使用内置生成器。
 * 成功返回 0，失败返回 -1（已打印错误信息）。 */
int rs_open(random_source_t *rs, const char *source);

/* 读取 len 字节填满 buf。
 * 成功返回 0，失败返回 -1（已打印错误信息）。 */
int rs_read(random_source_t *rs, void *buf, size_t len);

/* 关闭数据源。 */
void rs_close(random_source_t *rs);

#endif /* RANDOM_SOURCE_H */
