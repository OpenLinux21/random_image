/*
 * jpg_writer - JPG 输出模块实现
 */

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "jpg_writer.h"

#include <stdio.h>
#include <string.h>

/* JPG 编码质量（1-100） */
#define JPG_QUALITY 90

size_t jpg_needed_size(int width, int height, jpg_mode_t mode)
{
    size_t px = (size_t)width * (size_t)height;

    return (mode == JW_RGB) ? px * 3 : px;
}

int jpg_write(const char *filename, int width, int height,
              jpg_mode_t mode, const uint8_t *data)
{
    int comp;
    int ok;

    if (filename == NULL || filename[0] == '\0') {
        fprintf(stderr, "错误: 输出文件名为空\n");
        return -1;
    }
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "错误: 非法分辨率 %dx%d\n", width, height);
        return -1;
    }
    if (data == NULL) {
        fprintf(stderr, "错误: 像素数据为空\n");
        return -1;
    }

    comp = (mode == JW_RGB) ? 3 : 1;
    ok   = stbi_write_jpg(filename, width, height, comp, data, JPG_QUALITY);
    if (!ok) {
        fprintf(stderr, "错误: 写入 JPG 文件 %s 失败\n", filename);
        return -1;
    }
    return 0;
}
