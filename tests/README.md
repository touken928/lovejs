# 测试目录（对齐 `src/`）

| `tests/` 路径 | 对应 `src/` | 说明 |
|---------------|---------------|------|
| `runtime/` | `src/runtime/` | 无 JS 运行时的单测（如 `embed`、宿主上下文） |
| `cli/` | `src/cli/` | `cli_test.cc`：`qianjs_cli_run()` 路由与退出码（需 **`QIANJS_BUILD_CLI=ON`** + 链接 **`qianjs_impl`**） |
| `native/` | `src/native/` | 可按模块加子目录与 `*_test.cc`；需 `JSEngine` 的用法可另做链接 `qjs::qjs` 或跑 `qianjs run` 脚本 |

在 **`tests/CMakeLists.txt`** 的 `QIANJS_TEST_SOURCES` 中登记新文件。根目录开启 **`QIANJS_BUILD_TESTS`** 时构建 **`qianjs_tests`** 并注册 **`ctest`**。

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j8
ctest --test-dir build --output-on-failure
```
