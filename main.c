/*
 * random_image - 随机数据转 JPG 图片命令行工具
 *
 * 从指定数据源（内置生成器/文件/设备）读取随机数据，转换为像素数据，
 * 编码输出为 JPG 图片 <time>.jpg（同一秒重复运行时自动追加序号）。
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include "random_source.h"
#include "jpg_writer.h"

#define PROG_NAME      "random_image"
#define DEFAULT_WIDTH  1920
#define DEFAULT_HEIGHT 1080

static void usage(FILE *out)
{
    fprintf(out,
            "用法: %s [选项]\n"
            "从随机数据源读取数据，生成一张 JPG 噪声图片 <time>.jpg。\n"
            "\n"
            "选项:\n"
            "  -s <source>    随机数据来源（默认: 内置生成器）\n"
            "                 支持普通文件、字符设备(/dev/urandom 等)、\n"
            "                 块设备(/dev/sda 等，随机偏移读取，可能需要 root)\n"
            "  -r <WxH>       分辨率，如 1920x1080（默认: %dx%d）\n"
            "  -m <gray|rgb>  噪声样式: gray=灰度(默认), rgb=彩色\n"
            "  -h             显示本帮助\n",
            PROG_NAME, DEFAULT_WIDTH, DEFAULT_HEIGHT);
}

/* 解析 "宽x高" 格式分辨率，成功返回 0 */
static int parse_resolution(const char *arg, int *w, int *h)
{
    if (arg == NULL || sscanf(arg, "%dx%d", w, h) != 2 || *w <= 0 || *h <= 0)
        return -1;
    return 0;
}

/* 生成输出文件名：<YYYYMMDD_HHMMSS>.jpg，同名存在时追加 _1/_2... 序号 */
static int make_output_name(char *buf, size_t size)
{
    char      stem[32];
    time_t    now = time(NULL);
    struct tm tmv;
    size_t    n;

    if (localtime_r(&now, &tmv) == NULL)
        return -1;
    n = strftime(stem, sizeof(stem), "%Y%m%d_%H%M%S", &tmv);
    if (n == 0)
        return -1;

    snprintf(buf, size, "%s.jpg", stem);
    if (access(buf, F_OK) != 0)
        return 0;

    for (int i = 1; i < 1000; i++) {
        snprintf(buf, size, "%s_%d.jpg", stem, i);
        if (access(buf, F_OK) != 0)
            return 0;
    }
    return -1;
}

int main(int argc, char *argv[])
{
    const char  *source = NULL;
    int          width  = DEFAULT_WIDTH;
    int          height = DEFAULT_HEIGHT;
    jpg_mode_t   mode   = JW_GRAY;
    random_source_t rs;
    uint8_t     *pixels;
    size_t       need;
    char         outname[64];
    int          opt;

    while ((opt = getopt(argc, argv, "s:r:m:h")) != -1) {
        switch (opt) {
        case 's':
            source = optarg;
            break;
        case 'r':
            if (parse_resolution(optarg, &width, &height) != 0) {
                fprintf(stderr, "错误: 非法分辨率 '%s'（格式: 宽x高，如 1920x1080）\n",
                        optarg);
                return EXIT_FAILURE;
            }
            break;
        case 'm':
            if (strcmp(optarg, "gray") == 0)
                mode = JW_GRAY;
            else if (strcmp(optarg, "rgb") == 0)
                mode = JW_RGB;
            else {
                fprintf(stderr, "错误: 未知样式 '%s'（可选: gray, rgb）\n", optarg);
                return EXIT_FAILURE;
            }
            break;
        case 'h':
            usage(stdout);
            return EXIT_SUCCESS;
        default:
            usage(stderr);
            return EXIT_FAILURE;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "错误: 多余的参数 '%s'\n", argv[optind]);
        usage(stderr);
        return EXIT_FAILURE;
    }

    if (rs_open(&rs, source) != 0)
        return EXIT_FAILURE;

    need = jpg_needed_size(width, height, mode);
    pixels = malloc(need);
    if (pixels == NULL) {
        fprintf(stderr, "错误: 内存分配失败（需要 %zu 字节）\n", need);
        rs_close(&rs);
        return EXIT_FAILURE;
    }

    if (rs_read(&rs, pixels, need) != 0) {
        free(pixels);
        rs_close(&rs);
        return EXIT_FAILURE;
    }
    rs_close(&rs);

    if (make_output_name(outname, sizeof(outname)) != 0) {
        fprintf(stderr, "错误: 生成输出文件名失败\n");
        free(pixels);
        return EXIT_FAILURE;
    }

    if (jpg_write(outname, width, height, mode, pixels) != 0) {
        free(pixels);
        return EXIT_FAILURE;
    }
    free(pixels);

    printf("已生成 %s (%dx%d, %s)\n", outname, width, height,
           (mode == JW_RGB) ? "rgb" : "gray");
    return EXIT_SUCCESS;
}
