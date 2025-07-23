project('perler-beads-generator', 'cpp',
  version: '1.0.0',
  license: 'MIT',
  default_options: [
    'cpp_std=c++17',
    'warning_level=2',
    'werror=false'
  ],
  meson_version: '>= 0.55.0'
)

# 查找依赖
cairomm_dep = dependency('cairomm-1.16', required: false)

# 如果系统中没有 cairomm，使用子项目
if not cairomm_dep.found()
  message('System cairomm not found, using subproject')
  cairomm_subproj = subproject('cairomm')
  cairomm_dep = cairomm_subproj.get_variable('cairomm_dep')
endif

# 添加子目录
subdir('tests')

# 创建一个主要的可执行文件（可选）
# executable('perler-generator',
#   'src/main.cpp',
#   dependencies: [cairomm_dep],
#   install: true
# )