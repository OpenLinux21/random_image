# random_image

一个 C 语言命令行工具：从指定数据源读取随机/杂乱数据，将其硬转为像素数据，
编码输出为一张合法的 JPG 图片 `<time>.jpg`。

## 特性（规划）

- 多种随机数据来源：内置生成器（默认）、普通文件、字符设备（`/dev/random`、
  `/dev/urandom`、`/dev/zero` 等）、块设备（`/dev/sda` 等，随机偏移读取）
- 可指定分辨率与噪声样式（灰度 / RGB 彩色）
- 纯 C11 实现，仅依赖 libc 与单头文件库 stb_image_write

## 编译

```sh
make
```

## 用法

```sh
./random_image [选项]
  -s <source>    随机数据来源（默认：内置生成器）
  -r <WxH>       分辨率，如 1920x1080（默认：1920x1080）
  -m <gray|rgb>  噪声样式：gray=灰度(默认)，rgb=彩色
  -h             显示帮助
```

## 开发分支

- `main`：稳定版本，以 tag 标记（版本号 `x.x.x`）
- `develop`：日常开发分支

## 许可证

见 [LICENSE](LICENSE)。
