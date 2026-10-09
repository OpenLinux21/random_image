/*
 * jpg_writer - JPG 输出模块
 *
 * 将随机数据填充为像素缓冲区，用 stb_image_write 编码为 JPG 文件。
 * 支持 gray（灰度，每像素 1 字节）与 rgb（彩色，每像素 3 字节）两种样式。
 */
#ifndef JPG_WRITER_H
#define JPG_WRITER_H

#include <stddef.h>
#include <stdint.h>

/* 噪声样式 */
typedef enum {
    JW_GRAY = 0,   /* 灰度噪声 */
    JW_RGB         /* RGB 彩色噪声 */
} jpg_mode_t;

/* 计算 width x height 图像所需的随机数据字节数 */
size_t jpg_needed_size(int width, int height, jpg_mode_t mode);

/* 将随机数据 data（长度须不小于 jpg_needed_size）编码为 JPG 写入 filename。
 * 成功返回 0，失败返回 -1（已打印错误信息）。 */
int jpg_write(const char *filename, int width, int height,
              jpg_mode_t mode, const uint8_t *data);

#endif /* JPG_WRITER_H */
