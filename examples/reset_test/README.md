# RecoNIC复位功能测试程序

## 概述

本程序提供了对RecoNIC智能网卡的完整复位功能，支持系统级复位和各子模块复位操作。该工具基于onic驱动中的复位机制实现，为用户提供了安全、可靠的复位功能。

## 功能特性

- ✅ **系统级复位**：支持完整的系统复位功能
- ✅ **Shell层复位**：支持网卡Shell层各子模块的独立复位
- ✅ **用户逻辑复位**：支持用户可编程逻辑部分的复位
- ✅ **CMAC端口复位**：支持100G以太网端口的独立复位
- ✅ **GT收发器复位**：支持GT收发器的独立复位
- ✅ **状态监控**：提供复位状态的实时监控和验证
- ✅ **交互式操作**：支持交互式复位操作界面
- ✅ **安全保护**：提供复位前确认和强制模式选项
- ✅ **架构兼容**：支持x86_64和ARM64架构

## 系统要求

- 操作系统：Linux（Ubuntu 20.04+ 推荐）
- 权限：需要root权限运行
- 硬件：安装了RecoNIC智能网卡的系统
- 依赖：已编译的libreconic库和加载的onic驱动

## 编译安装

### 1. 编译程序

```bash
cd /workspace/examples/reset_test

# 基本编译
make

# 调试版本编译
make debug

# 发布版本编译（优化）
make release

# ARM架构优化编译
make arm-optimize
```

### 2. 安装到系统路径（可选）

```bash
sudo make install
```

## 使用方法

### 基本语法

```bash
sudo ./reset_test [选项]
```

### 命令行选项

| 短选项 | 长选项 | 参数 | 描述 |
|--------|--------|------|------|
| -d | --device | 设备路径 | 字符设备名称 |
| -p | --pcie_resource | 资源路径 | PCIe资源路径 |
| -s | --system | 无 | 执行系统复位 |
| -S | --shell | 无 | 执行Shell层复位 |
| -u | --user | 无 | 执行用户逻辑复位 |
| -c | --cmac-port | 端口号 | 执行CMAC端口复位（0或1） |
| -g | --cmac-gt | 端口号 | 执行CMAC GT端口复位（0或1） |
| -t | --status | 无 | 仅显示复位状态 |
| -i | --interactive | 无 | 进入交互式模式 |
| -f | --force | 无 | 强制复位，跳过确认 |
| -V | --verbose | 无 | 启用详细输出 |
| -h | --help | 无 | 显示帮助信息 |

## 使用示例

### 1. 查看复位状态

```bash
sudo ./reset_test --status
```

输出示例：
```
=== RecoNIC系统复位状态 ===
系统状态寄存器：     0x00000001
  系统复位完成：     是
Shell状态寄存器：    0x00000110
  CMAC端口0复位完成：是
  CMAC端口1复位完成：是
用户状态寄存器：     0x00000001
  用户复位完成：     是
===========================
```

### 2. 系统复位

```bash
sudo ./reset_test --system
```

输出示例：
```
⚠️  警告：您即将执行 系统复位
描述：完整的系统复位，将重置整个RecoNIC系统

这个操作可能会影响系统稳定性和网络连接。
确认要继续吗？(y/N): y
执行系统复位...
系统复位命令已发送，等待复位完成...
✓ 系统复位完成
```

### 3. Shell层复位

```bash
sudo ./reset_test --shell
```

### 4. CMAC端口复位

```bash
# 复位CMAC端口0
sudo ./reset_test --cmac-port 0

# 复位CMAC端口1  
sudo ./reset_test --cmac-port 1
```

### 5. GT收发器复位

```bash
# 复位CMAC GT端口0
sudo ./reset_test --cmac-gt 0

# 复位CMAC GT端口1
sudo ./reset_test --cmac-gt 1
```

### 6. 用户逻辑复位

```bash
sudo ./reset_test --user
```

### 7. 强制复位（跳过确认）

```bash
sudo ./reset_test --system --force
```

### 8. 交互式模式

```bash
sudo ./reset_test --interactive
```

交互式操作界面：
```
=== 进入交互式复位模式 ===

可用的复位操作：
1. 系统复位
2. Shell层复位
3. 用户复位
4. CMAC端口0复位
5. CMAC端口1复位
6. CMAC GT端口0复位
7. CMAC GT端口1复位
8. 显示所有复位状态
0. 退出交互模式

请选择操作 (0-8): 
```

### 9. 详细输出模式

```bash
sudo ./reset_test --system --verbose
```

## 复位类型详解

### 1. 系统复位（System Reset）
- **寄存器地址**：`0x0004`
- **功能**：完整的系统复位，重置整个RecoNIC系统
- **影响范围**：全部硬件模块
- **使用场景**：系统故障恢复、完整重新初始化

### 2. Shell层复位（Shell Reset）
- **寄存器地址**：`0x000C`
- **功能**：重置网卡Shell层各个子模块
- **影响范围**：CMAC、QDMA等Shell层模块
- **使用场景**：网络接口问题修复

### 3. 用户复位（User Reset）
- **寄存器地址**：`0x0014`
- **功能**：重置用户可编程逻辑部分
- **影响范围**：用户自定义硬件逻辑
- **使用场景**：用户逻辑故障恢复

### 4. CMAC端口复位
- **控制方式**：通过Shell层复位寄存器的特定位
- **端口0掩码**：`0x10`
- **端口1掩码**：`0x100`
- **功能**：重置指定的100G以太网端口
- **影响范围**：单个CMAC端口
- **使用场景**：网络接口故障恢复

### 5. CMAC GT复位
- **端口0寄存器**：`0x8000`
- **端口1寄存器**：`0xC000`
- **功能**：重置GT收发器
- **影响范围**：物理层收发器
- **使用场景**：物理层信号问题修复

## 寄存器映射

### 系统配置寄存器
| 寄存器名称 | 偏移地址 | 读写属性 | 功能描述 |
|------------|----------|----------|----------|
| BUILD_STATUS | 0x0000 | R | 构建状态 |
| SYSTEM_RESET | 0x0004 | W | 系统复位控制 |
| SYSTEM_STATUS | 0x0008 | R | 系统状态 |
| SHELL_RESET | 0x000C | W | Shell层复位控制 |
| SHELL_STATUS | 0x0010 | R | Shell层状态 |
| USER_RESET | 0x0014 | W | 用户复位控制 |
| USER_STATUS | 0x0018 | R | 用户状态 |

### CMAC子系统寄存器
| 寄存器名称 | 端口0地址 | 端口1地址 | 功能描述 |
|------------|-----------|-----------|----------|
| GT_RESET | 0x8000 | 0xC000 | GT收发器复位 |
| CMAC_RESET | 0x8004 | 0xC004 | CMAC核心复位 |

## 安全注意事项

⚠️ **重要安全提醒**：

### 操作前检查
1. **确认维护窗口**：复位操作会中断网络连接
2. **备份配置**：复位前备份重要的网络配置
3. **检查依赖服务**：确认没有关键服务正在使用网卡
4. **硬件状态**：确保硬件连接正常

### 复位顺序建议
1. 首先尝试最小影响的复位（GT复位、CMAC端口复位）
2. 然后考虑用户逻辑复位
3. 最后才考虑Shell层复位或系统复位

### 故障恢复
如果复位后系统无响应：
1. 检查驱动模块是否正常加载
2. 验证PCIe连接状态
3. 重新加载onic驱动
4. 必要时重启系统

## 故障排除

### 常见错误及解决方案

1. **权限不足错误**
   ```
   错误：本程序需要root权限运行
   ```
   **解决**：使用 `sudo` 运行程序

2. **PCIe设备访问失败**
   ```
   错误：无法打开PCIe资源文件
   ```
   **解决**：
   - 检查PCIe设备路径：`lspci | grep -i xilinx`
   - 确认驱动已加载：`lsmod | grep onic`

3. **复位超时错误**
   ```
   错误：系统复位超时或失败
   ```
   **解决**：
   - 检查硬件连接状态
   - 验证寄存器访问权限
   - 尝试重新加载驱动

4. **交互模式无响应**
   **解决**：按Ctrl+C退出，检查终端设置

### 调试方法

1. **启用详细输出**：
   ```bash
   sudo ./reset_test --status --verbose
   ```

2. **使用调试版本**：
   ```bash
   make debug
   sudo ./reset_test --status
   ```

3. **检查系统日志**：
   ```bash
   dmesg | tail -20
   journalctl -f
   ```

4. **验证驱动状态**：
   ```bash
   lsmod | grep onic
   cat /proc/interrupts | grep onic
   ```

## ARM架构支持

### NVIDIA Jetson平台优化

本程序特别针对ARM架构进行了优化：

1. **编译优化**：
   ```bash
   make arm-optimize
   ```

2. **架构检测**：程序自动检测并应用ARM特定优化

3. **兼容性验证**：
   ```bash
   file reset_test  # 检查可执行文件架构
   uname -m         # 确认系统架构
   ```

## 开发说明

### 代码结构
- `reset_test.c` - 主程序入口和命令行处理
- `reset_utils.c` - 复位功能实现和工具函数
- `reset_utils.h` - 头文件和函数声明
- `Makefile` - 多架构编译配置
- `README.md` - 使用说明文档

### 扩展开发
要添加新的复位功能：
1. 在 `reset_utils.h` 中定义新的复位类型
2. 在 `reset_utils.c` 中实现相应的复位函数
3. 在 `reset_test.c` 中添加命令行选项支持

## 许可证

Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.  
SPDX-License-Identifier: MIT
