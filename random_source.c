/*
 * random_source - 随机数据源模块实现
 */
#define _POSIX_C_SOURCE 200809L
#define _FILE_OFFSET_BITS 64

#include "random_source.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>       /* BLKGETSIZE64 */

/* 块设备单次随机读取的最大长度 */
#define RS_BLOCK_CHUNK (64u * 1024u)

/* xorshift32：轻量快速 PRNG */
static uint32_t xs32(uint32_t *state)
{
    uint32_t x = *state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

/* 由时间/进程号/CPU 时间混合生成非零种子 */
static uint32_t make_seed(void)
{
    uint32_t s = (uint32_t)time(NULL);

    s ^= (uint32_t)getpid() << 16;
    s ^= (uint32_t)clock();
    if (s == 0)
        s = 0x9E3779B9u;    /* 黄金比例常数，避免全零状态 */
    return s;
}

/* 打印读取错误，权限类错误附 root 提示 */
static void perror_read(const random_source_t *rs, uint64_t off)
{
    fprintf(stderr, "错误: 读取数据源 %s (偏移 %llu) 失败: %s%s\n",
            rs->name, (unsigned long long)off, strerror(errno),
            (errno == EACCES || errno == EPERM) ? "（可能需要 root 权限）" : "");
}

/* 循环 read 直到填满 n 字节（用于不会 EOF 的字符设备） */
static int read_full_fd(const random_source_t *rs, uint8_t *buf, size_t n)
{
    while (n > 0) {
        ssize_t r = read(rs->fd, buf, n);

        if (r < 0) {
            if (errno == EINTR)
                continue;
            perror_read(rs, 0);
            return -1;
        }
        if (r == 0) {       /* 字符设备不应出现 EOF，防御处理 */
            fprintf(stderr, "错误: 数据源 %s 意外结束\n", rs->name);
            return -1;
        }
        buf += r;
        n -= (size_t)r;
    }
    return 0;
}

/* 普通文件读取：到 EOF 后回到文件头循环填充 */
static int read_file(random_source_t *rs, uint8_t *buf, size_t n)
{
    while (n > 0) {
        ssize_t r = read(rs->fd, buf, n);

        if (r < 0) {
            if (errno == EINTR)
                continue;
            perror_read(rs, (uint64_t)rs->pos);
            return -1;
        }
        if (r == 0) {       /* EOF：循环回文件头 */
            if (lseek(rs->fd, 0, SEEK_SET) == (off_t)-1) {
                perror_read(rs, 0);
                return -1;
            }
            rs->pos = 0;
            continue;
        }
        buf += r;
        n -= (size_t)r;
        rs->pos += (uint64_t)r;
    }
    return 0;
}

/* 块设备读取：随机选择偏移，分块读取 */
static int read_blockdev(random_source_t *rs, uint8_t *buf, size_t n)
{
    while (n > 0) {
        uint64_t off = ((uint64_t)xs32(&rs->seed) << 32) | xs32(&rs->seed);
        size_t   chunk = n < RS_BLOCK_CHUNK ? n : RS_BLOCK_CHUNK;
        ssize_t  r;

        off %= rs->size;
        if ((uint64_t)chunk > rs->size - off)
            chunk = (size_t)(rs->size - off);

        r = pread(rs->fd, buf, chunk, (off_t)off);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            perror_read(rs, off);
            return -1;
        }
        if (r == 0) {
            fprintf(stderr, "错误: 读取块设备 %s (偏移 %llu) 意外结束\n",
                    rs->name, (unsigned long long)off);
            return -1;
        }
        buf += r;
        n -= (size_t)r;
    }
    return 0;
}

/* 内置生成器填充 */
static void fill_generator(random_source_t *rs, uint8_t *buf, size_t n)
{
    size_t i = 0;

    while (i < n) {
        uint32_t r    = xs32(&rs->seed);
        size_t   left = n - i;
        size_t   take = left < 4 ? left : 4;

        memcpy(buf + i, &r, take);
        i += take;
    }
}

int rs_open(random_source_t *rs, const char *source)
{
    struct stat st;

    memset(rs, 0, sizeof(*rs));
    rs->fd   = -1;
    rs->seed = make_seed();

    if (source == NULL || source[0] == '\0') {
        rs->type = RS_GENERATOR;
        rs->name = "(内置生成器)";
        return 0;
    }

    rs->name = source;
    rs->fd = open(source, O_RDONLY);
    if (rs->fd < 0) {
        fprintf(stderr, "错误: 无法打开数据源 %s: %s%s\n", source, strerror(errno),
                (errno == EACCES) ? "（可能需要 root 权限）" : "");
        return -1;
    }

    if (fstat(rs->fd, &st) != 0) {
        fprintf(stderr, "错误: 获取数据源 %s 信息失败: %s\n", source, strerror(errno));
        close(rs->fd);
        rs->fd = -1;
        return -1;
    }

    if (S_ISCHR(st.st_mode)) {
        rs->type = RS_CHARDEV;              /* /dev/urandom 等：顺序读，无限长 */
        rs->size = 0;
    } else if (S_ISBLK(st.st_mode)) {
        rs->type = RS_BLOCKDEV;             /* /dev/sda 等：随机偏移读 */
        if (ioctl(rs->fd, BLKGETSIZE64, &rs->size) != 0 || rs->size == 0) {
            off_t end = lseek(rs->fd, 0, SEEK_END);     /* 兜底：SEEK_END 取大小 */

            if (end == (off_t)-1 || end == 0) {
                fprintf(stderr, "错误: 无法确定块设备 %s 的大小\n", source);
                close(rs->fd);
                rs->fd = -1;
                return -1;
            }
            rs->size = (uint64_t)end;
        }
    } else if (S_ISREG(st.st_mode)) {
        if (st.st_size <= 0) {
            fprintf(stderr, "错误: %s 是空文件，无法作为数据源\n", source);
            close(rs->fd);
            rs->fd = -1;
            return -1;
        }
        rs->type = RS_FILE;
        rs->size = (uint64_t)st.st_size;
        rs->pos  = 0;
    } else {
        fprintf(stderr, "错误: %s 不是受支持的数据源类型（文件/字符设备/块设备）\n",
                source);
        close(rs->fd);
        rs->fd = -1;
        return -1;
    }

    return 0;
}

int rs_read(random_source_t *rs, void *buf, size_t len)
{
    if (len == 0)
        return 0;

    switch (rs->type) {
    case RS_GENERATOR:
        fill_generator(rs, (uint8_t *)buf, len);
        return 0;
    case RS_CHARDEV:
        return read_full_fd(rs, (uint8_t *)buf, len);
    case RS_FILE:
        return read_file(rs, (uint8_t *)buf, len);
    case RS_BLOCKDEV:
        return read_blockdev(rs, (uint8_t *)buf, len);
    }
    return -1;
}

void rs_close(random_source_t *rs)
{
    if (rs->fd >= 0) {
        close(rs->fd);
        rs->fd = -1;
    }
}
