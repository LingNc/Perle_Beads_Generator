#!/usr/bin/env python3
"""
跨平台的源文件收集脚本
用于 Meson 构建系统
"""

import os
import argparse
from pathlib import Path

def collect_source_files(src_dirs, extensions) -> list:
    """收集指定目录下的源文件"""
    files: list = []

    for src_dir in src_dirs:
        if os.path.exists(src_dir):
            for ext in extensions:
                # 使用 pathlib 进行跨平台路径处理
                path = Path(src_dir)
                pattern = f"*.{ext}"
                files.extend(str(f) for f in path.rglob(pattern))

    return files

def parse_args(desc: str) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=desc)
    parser.add_argument('-f','--files',type=str,required=True,nargs='+',help='定义搜索的源文件路径，多个路径用空格分隔。')
    parser.add_argument('-t','--types',type=str,required=True,nargs='+',help='定义要搜索的文件扩展名，多个扩展名用空格分隔。')
    return parser.parse_args()

def main():
    # 获取搜索参数
    args = parse_args(desc='此脚本为 Meson 构建系统收集源文件。')

    # 定义要搜索的目录和文件扩展名
    src_directories = args.files
    # ['src/types', 'src/app', 'src/utils']
    file_extensions = args.types
    # ['cppm', 'cpp']

    # 收集文件
    source_files = collect_source_files(src_directories, file_extensions)

    # 过滤掉 main.cpp
    source_files = [f for f in source_files if not f.endswith('main.cpp')]

    # 输出结果，用分号分隔（Meson 可以解析）
    print(';'.join(source_files))

if __name__ == '__main__':
    main()
