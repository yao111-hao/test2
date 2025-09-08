# RecoNIC寄存器测试程序

## 概述

本程序提供了对RecoNIC智能网卡寄存器的直接访问功能，允许用户通过BAR空间地址读写任意网卡寄存器。该工具主要用于硬件调试、状态监控和配置验证。

## 功能特性

- ✅ **安全的寄存器访问**：提供地址验证和4字节对齐检查
- ✅ **多种操作模式**：支持单次读写和交互式连续操作
- ✅ **架构兼容**：支持x86_64和ARM64（如NVIDIA Jetson）
- ✅ **寄存器识别**：自动识别常见寄存器并显示名称描述
- ✅ **详细输出**：提供十六进制、十进制和二进制格式显示
- ✅ **错误处理**：完善的错误检查和安全保护机制

## 系统要求

- 操作系统：Linux（Ubuntu 20.04+ 推荐）
- 权限：需要root权限运行
- 硬件：安装了RecoNIC智能网卡的系统
- 依赖：已编译的libreconic库

## 编译安装

### 1. 编译libreconic库（如果尚未编译）

```bash
cd ../../lib
make
```

### 2. 编译寄存器测试程序

```bash
# 基本编译
make

# 调试版本编译
make debug

# 发布版本编译（优化）
make release

# ARM架构优化编译
make arm-optimize

# 查看编译帮助
make help
```

### 3. 安装到系统路径（可选）

```bash
sudo make install
```

## 使用方法

### 基本语法

```bash
sudo ./register_test [选项]
```

### 命令行选项

| 选项 | 长选项 | 参数 | 描述 |
|------|--------|------|------|
| -d | --device | 设备路径 | 字符设备名称（默认：/dev/reconic-mm） |
| -p | --pcie_resource | 资源路径 | PCIe资源路径（默认：/sys/bus/pci/devices/0000:d8:00.0/resource2） |
| -r | --read | 寄存器地址 | 读取指定偏移地址的寄存器 |
| -w | --write | 寄存器地址 | 写入指定偏移地址的寄存器（需配合-v） |
| -v | --value | 寄存器值 | 指定要写入的寄存器值 |
| -i | --interactive | 无 | 进入交互式模式 |
| -V | --verbose | 无 | 启用详细输出模式 |
| -h | --help | 无 | 显示帮助信息 |

### 使用示例

#### 1. 读取版本寄存器

```bash
sudo ./register_test -r 0x102000
```

输出示例：
```
=== 寄存器读取结果 ===
寄存器偏移：0x102000
寄存器名称：RN_SCR_VERSION（版本寄存器）
寄存器值：  0x12345678 (305419896)
二进制值：  0001 0010 0011 0100 0101 0110 0111 1000
```

#### 2. 写入计算控制寄存器

```bash
sudo ./register_test -w 0x103000 -v 0x12345678
```

输出示例：
```
=== 寄存器写入操作 ===
寄存器偏移：0x103000
寄存器名称：RN_CLR_CTL_CMD（计算控制命令寄存器）
写入值：    0x12345678 (305419896)
写入前值：  0x00000000 (0)
寄存器写入成功
写入后值：  0x12345678 (305419896)
✓ 验证成功：读回值与写入值一致
```

#### 3. 使用自定义PCIe路径

```bash
sudo ./register_test -p /sys/bus/pci/devices/0000:01:00.0/resource2 -r 0x102000
```

#### 4. 交互式模式

```bash
sudo ./register_test -i
```

交互式命令：
- `r <offset>` - 读取寄存器（如：r 0x102000）
- `w <offset> <value>` - 写入寄存器（如：w 0x103000 0x12345678）
- `h` - 显示帮助信息
- `q` - 退出交互模式

#### 5. 详细输出模式

```bash
sudo ./register_test -V -r 0x102000
```

## 常用寄存器地址

### 统计和配置寄存器（SCR）

| 地址 | 名称 | 描述 | 访问权限 |
|------|------|------|----------|
| 0x102000 | RN_SCR_VERSION | 版本寄存器 | 只读 |
| 0x102004 | RN_SCR_FATAL_ERR | 致命错误寄存器 | 只读 |
| 0x102008 | RN_SCR_TRMHR_REG | 传输高位寄存器 | 只读 |
| 0x10200C | RN_SCR_TRMLR_REG | 传输低位寄存器 | 只读 |

### 计算逻辑寄存器（CLR）

| 地址 | 名称 | 描述 | 访问权限 |
|------|------|------|----------|
| 0x103000 | RN_CLR_CTL_CMD | 计算控制命令寄存器 | 读写 |
| 0x103004 | RN_CLR_KER_STS | 内核状态寄存器 | 只读 |
| 0x103008 | RN_CLR_JOB_SUBMITTED | 作业提交寄存器 | 只读 |
| 0x10300C | RN_CLR_JOB_COMPLETED_NOT_READ | 作业完成未读寄存器 | 只读 |

### RDMA全局控制状态寄存器（GCSR）

| 地址 | 名称 | 描述 | 访问权限 |
|------|------|------|----------|
| 0x060000 | RN_RDMA_GCSR_XRNICCONF | RDMA全局配置寄存器 | 读写 |
| 0x060004 | RN_RDMA_GCSR_XRNICADCONF | RDMA高级配置寄存器 | 读写 |
| 0x060010 | RN_RDMA_GCSR_MACXADDLSB | MAC地址低32位寄存器 | 读写 |
| 0x060014 | RN_RDMA_GCSR_MACXADDMSB | MAC地址高32位寄存器 | 读写 |
| 0x060070 | RN_RDMA_GCSR_IPV4XADD | IPv4地址寄存器 | 读写 |

### QDMA AXI桥接寄存器

| 地址 | 名称 | 描述 | 访问权限 |
|------|------|------|----------|
| 0x016420 | AXIB_BDF_ADDR_TRANSLATE_ADDR_LSB | BDF地址转换低位 | 读写 |
| 0x016424 | AXIB_BDF_ADDR_TRANSLATE_ADDR_MSB | BDF地址转换高位 | 读写 |
| 0x016430 | AXIB_BDF_MAP_CONTROL_ADDR | BDF映射控制地址 | 读写 |

## ARM架构支持

本程序特别针对ARM架构（如NVIDIA Jetson平台）进行了优化：

### 架构检测

程序会自动检测当前架构并应用相应的优化设置：

- **x86_64**：使用 `-march=native -mtune=native`
- **aarch64**：使用 `-mcpu=native -mtune=native`
- **armv7l**：使用 `-mcpu=native -mtune=native`

### ARM特定编译

```bash
# 针对ARM架构优化编译
make arm-optimize

# 检查编译结果架构
file register_test
```

### NVIDIA Jetson平台注意事项

1. **PCIe路径可能不同**：
   ```bash
   # 查找实际的PCIe设备
   lspci | grep -i xilinx
   
   # 使用正确的路径
   sudo ./register_test -p /sys/bus/pci/devices/0000:01:00.0/resource2 -r 0x102000
   ```

2. **内存映射权限**：
   确保ARM平台上的内存映射权限正确设置。

## 安全注意事项

⚠️ **重要安全提醒**：

1. **需要root权限**：本程序需要root权限才能访问硬件寄存器
2. **只读寄存器保护**：某些寄存器为只读，写入操作可能无效
3. **系统稳定性**：不当的寄存器操作可能导致系统不稳定或硬件故障
4. **地址验证**：程序会检查地址对齐和范围，但仍需谨慎操作
5. **备份重要配置**：在修改关键寄存器前，建议备份当前配置

## 故障排除

### 常见错误及解决方案

1. **权限不足错误**
   ```
   错误：本程序需要root权限运行
   ```
   解决：使用 `sudo` 运行程序

2. **设备文件不存在**
   ```
   错误：无法打开PCIe资源文件
   ```
   解决：检查PCIe设备路径，确保驱动已正确加载

3. **地址对齐错误**
   ```
   错误：寄存器偏移地址无效或不安全
   ```
   解决：确保地址4字节对齐（地址末位为0、4、8、C）

4. **架构兼容性问题**
   ```
   编译错误或运行时崩溃
   ```
   解决：使用 `make arm-optimize` 进行ARM架构优化编译

### 调试方法

1. **启用详细输出**：
   ```bash
   sudo ./register_test -V -r 0x102000
   ```

2. **使用调试版本**：
   ```bash
   make debug
   sudo ./register_test -r 0x102000
   ```

3. **检查系统日志**：
   ```bash
   dmesg | tail -20
   ```

## 开发说明

### 代码结构

- `register_test.c` - 主程序入口和命令行处理
- `register_utils.c` - 寄存器访问工具函数
- `register_utils.h` - 头文件和函数声明
- `Makefile` - 编译配置，支持多架构
- `README.md` - 使用说明文档

### 扩展开发

要添加新的寄存器支持，请修改 `register_utils.c` 中的 `get_register_name()` 函数。

要添加新的功能，可以扩展 `register_utils.h` 中的接口定义。

## 许可证

Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: MIT
