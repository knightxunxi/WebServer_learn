# GitHub 与 Linux 工作流

本项目在本地工作区开发，同时需要方便上传到 GitHub，并在 Linux 虚拟机中 clone、构建和运行。

## 首次推送到 GitHub

在 GitHub 创建空仓库后执行：

```bash
git remote add origin git@github.com:<user>/<repo>.git
git push -u origin main
```

如果使用 HTTPS：

```bash
git remote add origin https://github.com/<user>/<repo>.git
git push -u origin main
```

## 在 Linux 中克隆

```bash
git clone git@github.com:<user>/<repo>.git
cd <repo>
```

## 安装基础依赖

Ubuntu 示例：

```bash
sudo apt update
sudo apt install -y build-essential cmake git
```

可选工具：

```bash
sudo apt install -y gdb valgrind wrk apache2-utils linux-tools-common
```

## 构建和测试

```bash
bash scripts/build.sh
bash scripts/test.sh
```

## 建议分支流程

```text
main
  稳定学习里程碑

feature/<module-name>
  一个模块或一个里程碑

docs/<topic>
  只修改文档
```

建议提交格式：

```text
docs: 初始化第一阶段路线
build: 添加 cmake 基线
net: 添加 event loop
test: 添加 buffer 测试
bench: 记录 webserver wrk 压测结果
```

