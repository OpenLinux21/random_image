# random_image

一个 C 语言命令行工具：从指定数据源读取随机/杂乱数据，将其硬转为像素数据，
编码输出为一张合法的 JPG 噪声图片 `<time>.jpg`。

## 特性

- 多种随机数据来源：
  - 内置生成器（默认）：xorshift32 PRNG，时间 + 进程号 + CPU 时间混合做种子
  - 普通文件：顺序读取，数据不足时自动从头循环填充
  - 字符设备：`/dev/random`、`/dev/urandom`、`/dev/zero` 等，顺序读取
  - 块设备：`/dev/sda`、`/dev/nvme0n1` 等，随机偏移分块读取
    （通常需要 root 权限，无权限时给出提示）
- 可指定分辨率与噪声样式：gray 灰度（默认）/ rgb 彩色
- 纯 C11 实现，仅依赖 libc；JPG 编码用公共域单头文件库
  [stb_image_write](https://github.com/nothings/stb)（v1.16，已内置）

## 编译

```sh
make
```

## 用法

```sh
random_image [选项]
  -s <source>    随机数据来源（默认: 内置生成器）
                 支持普通文件、字符设备(/dev/urandom 等)、
                 块设备(/dev/sda 等，随机偏移读取，可能需要 root)
  -r <WxH>       分辨率，如 1920x1080（默认: 1920x1080）
  -m <gray|rgb>  噪声样式: gray=灰度(默认), rgb=彩色
  -h             显示本帮助
```

### 示例

```sh
./random_image                                  # 内置生成器，灰度，1920x1080
./random_image -m rgb -r 800x600                # 彩色噪声，800x600
./random_image -s /dev/urandom                  # 以内核熵源为数据
./random_image -s /dev/zero -m rgb              # 全零数据（纯黑噪声）
./random_image -s photo.dat -r 4096x2160        # 用任意文件当数据源
sudo ./random_image -s /dev/sda -r 1920x1080    # 读取整块磁盘（需 root）
```

### 输出

成功后在当前目录生成 `<YYYYMMDD_HHMMSS>.jpg`（如 `20261009_152000.jpg`）；
同一秒内重复运行会自动追加序号（`_1`、`_2`...）避免覆盖。

## 项目结构

```
main.c              入口与命令行参数解析
random_source.h/.c  随机数据源模块（生成器/文件/字符设备/块设备）
jpg_writer.h/.c     JPG 输出模块（gray/rgb 像素填充 + stb 编码）
stb_image_write.h   第三方单头文件库（公共域）
Makefile            增量构建
```

## 分支与版本

- `main`：稳定版本，以 tag 标记
- `develop`：日常开发分支
- 版本号 `x.x.x`：主版本 . 新功能 . 修复，均从 0 起始，如 `v0.1.0`

## 许可证

本项目见 [LICENSE](LICENSE)；`stb_image_write.h` 为公共域（public domain）。
