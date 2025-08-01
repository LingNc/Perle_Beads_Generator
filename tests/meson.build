# 基础 cairomm 测试
test_cairomm = executable(
  'test_cairomm',
  'test_cairomm.cpp',
  dependencies: [cairomm_dep],
  install: false
)

# 添加测试
test('cairomm 基本测试', test_cairomm)

# 输出可执行文件路径
message('Test executables:')
message('  Basic cairomm: ' + test_cairomm.full_path())