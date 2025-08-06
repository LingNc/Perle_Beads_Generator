project('perler-beads-generator', 'cpp',
  version: '0.2.0',
  license: 'MIT',
  default_options: [
    'cpp_std=c++23',
    'warning_level=2',
    'werror=false',
    'buildtype=debug'  # 调试模式，便于开发
  ],
  meson_version: '>= 0.62.0'  # 支持 C++ 模块需要较新版本
)

# 查找依赖
cairomm_dep = dependency('cairomm-1.16', required: false)

# 如果系统中没有 cairomm，使用子项目
if not cairomm_dep.found()
  message('系统中未找到 cairomm，使用子项目')
  cairomm_subproj = subproject('cairomm')
  cairomm_dep = cairomm_subproj.get_variable('cairomm_dep')
endif

# 查找 cxxopts 依赖
# 导入 cmake 模块
cmake = import('cmake')
# 配置 CMake 子项目
cxxopts_cmake = cmake.subproject('cxxopts')
# 获取 CMake target（cxxopts 提供的 target 名称）
cxxopts_dep = cxxopts_cmake.dependency('cxxopts')

# 添加头文件目录
inc_dir = include_directories('src/types')

# 使用 Python 脚本动态收集源文件
python = import('python').find_installation()

# 定义搜索目录和文件类型
src_directories = ['src/app', 'src/utils']
file_types = ['cppm', 'cpp']

# 调用收集脚本
collect_result = run_command(
  python, 'collect_sources.py',
  '--files', src_directories,
  '--types', file_types,
  check: true
)

# 解析脚本输出的文件列表
src_files = []
if collect_result.stdout().strip() != ''
  dynamic_files = collect_result.stdout().strip().split(';')
  # 将字符串列表转换为 files 对象
  src_files = files(dynamic_files)
endif

# 主文件
main_file = files('src/main.cpp')

# 所有源文件
all_sources = main_file + src_files

# 输出找到的文件数量（用于调试）
message('包含 @0@ 个源文件和模块文件'.format(src_files.length()))

# 添加子目录
subdir('tests')

# 创建一个主要的可执行文件，支持 C++ 模块
# 根据是否找到依赖来决定依赖列表
deps = [cairomm_dep]
if cxxopts_dep.found()
  deps += cxxopts_dep
endif

# 编译器参数支持模块
cpp_args = []

executable('perler-generator',
  all_sources,
  include_directories: inc_dir,
  dependencies: deps,
  cpp_args: cpp_args,
  install: false
)