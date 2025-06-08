# Perler Bead Generator (C++)

一个高效的C++拼豆图纸生成器，将图像转换为拼豆图纸并生成统计信息。

## 功能特性

- 图像像素化处理
- 颜色匹配到拼豆调色板
- 生成带有网格、坐标轴和统计信息的图纸
- 输出颜色统计文件
- 支持多种渲染模式（DPI模式、固定宽度模式）
- 高性能C++实现

## 依赖

- Cairo (图像渲染)
- FreeType2 (字体渲染)
- lodepng (PNG图像处理)

## 编译

```bash
mkdir build
cd build
cmake ..
make
```

## 使用方法

```bash
./PerlerBeadGenerator [选项] <输入图片>

选项:
  -o, --output <path>         输出文件路径 (默认: output.png)
  -s, --stats <path>          统计文件路径 (默认: color_counts.txt)
  -t, --title <string>        图纸标题
  -w, --width <pixels>        网格宽度 (像素数)
  -h, --height <pixels>       网格高度 (像素数)
  --dpi <value>               DPI设置 (默认: 150)
  --fixed-width <pixels>      固定宽度模式
  --show-grid                 显示网格线
  --grid-interval <n>         网格间隔 (默认: 10)
  --show-coords               显示坐标轴
  --include-stats             包含统计信息
  --grid-color <hex>          网格线颜色 (默认: #141414)
  --border-color <hex>        边框颜色
  --show-transparent          显示透明标签
  --help                      显示帮助信息
```

## 示例

```bash
# 基本用法
./PerlerBeadGenerator input.png

# 带标题和统计信息
./PerlerBeadGenerator -t "我的拼豆图案" --include-stats input.png

# 固定宽度模式
./PerlerBeadGenerator --fixed-width 800 --show-grid --show-coords input.png

# 自定义输出路径
./PerlerBeadGenerator -o my_pattern.png -s my_stats.txt input.png
```

## 输出文件

1. **图纸文件** (.png): 包含网格、色号、坐标轴和统计信息的完整图纸
2. **统计文件** (.txt): 包含每种颜色的用量统计

## 统计文件格式

```
# Perler Bead Color Statistics
# Generated on: 2025-06-08 15:30:00
# Title: 我的拼豆图案
# Grid Size: 32x24
# Total Beads: 768

A01 #FF0000 42
B12 #00FF00 35
C03 #0000FF 28
...

TOTAL: 768
```