# 测试方案

第一阶段需要把测试流程补上，因为这个仓库的目标是按照工程流程学习，而不是只写出代码。

## 测试层级

单元测试：

- server_config：INI 解析、默认值、字段校验。
- HTTP parser：请求行、header、query、Content-Length、Content-Type、body 限制。
- Router：静态文件、MIME、目录穿越、内置 API。
- 图片静态资源：SVG 图片访问、`image/svg+xml` MIME 返回。
- response serializer：状态行、常见响应头和 body。
- 旧 epoll 模块测试通过 `CSL_BUILD_LEGACY_EPOLL=ON` 在 Linux 下启用。

集成测试：

- Echo Server 单连接
- Echo Server 多连接
- HTTP 请求与响应
- Keep-Alive
- 客户端异常断开
- 服务端主动关闭

手动测试：

- `curl`
- `nc`
- `telnet`
- 浏览器访问静态响应
- POST `/api/echo`

压测：

- `wrk`
- `ab`

调试和质量检查：

- ASan
- UBSan
- Valgrind
- gdb
- strace
- lsof
- perf

## 当前基线

当前默认 CTest 覆盖：

- `csl_smoke_test`
- `csl_asio_http_response_test`
- `csl_asio_server_config_test`
- `csl_asio_http_parser_test`
- `csl_asio_router_test`

```bash
cmake -S . -B build -DCSL_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## 手动验证命令

```bash
curl -v http://localhost:8080/
curl -v http://localhost:8080/gallery.html
curl -v http://localhost:8080/style.css
curl -I http://localhost:8080/assets/asio-flow.svg
curl -v http://localhost:8080/api/status
curl -v -X POST http://localhost:8080/api/echo \
  -H "Content-Type: application/json" \
  -d '{"msg":"hello"}'
```
