# RecoNIC寄存器测试程序 - 编译与测试指南

## 快速开始

### 1. 编译程序

```bash
# 进入测试程序目录
cd /workspace/examples/register_test

# 清理之前的编译文件
make clean

# 编译程序（会自动编译依赖的libreconic库）
make

# 或者针对ARM架构优化编译
make arm-optimize
```

### 2. 验证编译结果

```bash
# 检查可执行文件
ls -la register_test
file register_test

# 显示帮助信息（无需root权限）
./register_test -h
```

### 3. 运行ARM兼容性检查

```bash
# 运行兼容性检查脚本
./arm_compatibility_check.sh
```

## 详细编译说明

### 系统要求

- **操作系统**：Linux（Ubuntu 20.04+ 推荐）
- **架构支持**：x86_64、aarch64（ARM64）、armv7l
- **编译工具**：gcc、make
- **权限**：编译不需要root权限，运行需要root权限

### 依赖库编译

```bash
# 首先编译RecoNIC用户空间库
cd /workspace/lib
make clean
make

# 验证库文件生成
ls -la libreconic.so libreconic.a
```

### 寄存器测试程序编译

```bash
cd /workspace/examples/register_test

# 基本编译
make

# 调试版本编译（包含更多调试信息）
make debug

# 发布版本编译（优化性能）
make release

# ARM架构特殊优化编译
make arm-optimize

# 清理编译文件
make clean
```

### 编译选项说明

| 编译目标 | 优化级别 | 调试信息 | 适用场景 |
|----------|----------|----------|----------|
| `make` | -g | 是 | 日常开发 |
| `make debug` | -O0 -ggdb3 | 详细 | 问题调试 |
| `make release` | -O2 | 无 | 生产环境 |
| `make arm-optimize` | -O2 | 是 | ARM平台 |

## 测试指南

### 基本功能测试（无需硬件）

```bash
# 1. 显示帮助信息
./register_test -h

# 2. 显示程序版本和编译信息
./register_test --help

# 3. 测试参数解析
./register_test -r 0x102000  # 会因为权限不足而失败，但能验证参数解析
```

### 硬件访问测试（需要root权限和硬件）

```bash
# 确保以root权限运行
sudo -i

# 1. 读取版本寄存器
./register_test -r 0x102000

# 2. 读取状态寄存器
./register_test -r 0x102004

# 3. 交互式模式测试
./register_test -i

# 4. 详细输出模式
./register_test -V -r 0x102000

# 5. 使用自定义PCIe路径（根据实际硬件调整）
./register_test -p /sys/bus/pci/devices/0000:01:00.0/resource2 -r 0x102000
```

### ARM平台特殊测试

```bash
# 1. 运行ARM兼容性检查
./arm_compatibility_check.sh

# 2. 使用生成的测试脚本
sudo ./test_register_access.sh

# 3. 检查CPU架构特定优化
grep -i "cpu\|arch" /proc/cpuinfo
lscpu | grep -i arch
```

## 常见问题解决

### 编译错误

**错误1：`libreconic.a: No such file`**
```bash
# 解决：先编译库文件
cd ../../lib
make
cd -
make
```

**错误2：`fatal error: 'reconic.h' file not found`**
```bash
# 解决：检查包含路径
make clean
make CFLAGS="-I../../lib"
```

**错误3：ARM架构编译警告**
```bash
# 解决：使用ARM优化编译
make clean
make arm-optimize
```

### 运行时错误

**错误1：`Permission denied`**
```bash
# 解决：使用root权限
sudo ./register_test -r 0x102000
```

**错误2：`No such file or directory: /dev/reconic-mm`**
```bash
# 解决：检查驱动是否加载
lsmod | grep onic
# 如果没有加载，加载驱动
sudo insmod ../../onic-driver/onic.ko
```

**错误3：`Cannot open PCIe resource`**
```bash
# 解决：查找正确的PCIe设备路径
lspci | grep -i xilinx
# 使用正确的路径
ls -la /sys/bus/pci/devices/0000:*/resource2
```

### ARM平台特殊问题

**问题1：NVIDIA Jetson平台PCIe路径不同**
```bash
# 查找实际的PCIe设备
sudo lspci -v | grep -A 10 -i xilinx

# 使用正确的路径运行
sudo ./register_test -p /sys/bus/pci/devices/0000:XX:YY.Z/resource2 -r 0x102000
```

**问题2：内存映射权限问题**
```bash
# 检查内存映射相关设置
cat /proc/sys/vm/mmap_min_addr
# 如果值过高，可能需要调整（谨慎操作）
```

## 性能测试

### 寄存器访问延迟测试

```bash
# 使用详细输出模式测量访问时间
time sudo ./register_test -V -r 0x102000

# 批量读取测试（交互模式）
sudo ./register_test -i
# 然后在交互模式中输入：
# r 0x102000
# r 0x102004
# r 0x103000
# q
```

### 多架构性能对比

```bash
# 编译不同优化版本并对比
make clean && make && time sudo ./register_test -r 0x102000
make clean && make release && time sudo ./register_test -r 0x102000
make clean && make arm-optimize && time sudo ./register_test -r 0x102000
```

## 安全测试

### 地址边界测试

```bash
# 测试无效地址（应该失败）
sudo ./register_test -r 0x400000  # 超出范围
sudo ./register_test -r 0x102001  # 未对齐

# 测试只读寄存器写入（应该警告）
sudo ./register_test -w 0x102000 -v 0x12345678  # 版本寄存器通常是只读
```

### 权限测试

```bash
# 测试非root用户访问（应该失败）
./register_test -r 0x102000

# 测试文件权限
ls -la /dev/reconic-mm
ls -la /sys/bus/pci/devices/*/resource2
```

## 自动化测试脚本

创建自动化测试脚本：

```bash
cat > full_test.sh << 'EOF'
#!/bin/bash
set -e

echo "=== RecoNIC寄存器测试程序完整测试 ==="

# 编译测试
echo "1. 编译测试..."
make clean
make

echo "2. 权限检查..."
if [ "$(id -u)" != "0" ]; then
    echo "需要root权限进行硬件测试"
    echo "运行: sudo $0"
    exit 1
fi

echo "3. 硬件检查..."
if lspci | grep -qi xilinx; then
    echo "检测到Xilinx设备"
else
    echo "未检测到Xilinx设备"
fi

echo "4. 驱动检查..."
if lsmod | grep -q onic; then
    echo "onic驱动已加载"
else
    echo "onic驱动未加载"
fi

echo "5. 设备文件检查..."
if [ -c "/dev/reconic-mm" ]; then
    echo "字符设备存在"
else
    echo "字符设备不存在"
fi

echo "6. 基本功能测试..."
./register_test -h > /dev/null && echo "帮助功能正常"

echo "7. 硬件访问测试..."
if ./register_test -r 0x102000 2>/dev/null; then
    echo "硬件访问成功"
else
    echo "硬件访问失败（可能硬件不可用）"
fi

echo "=== 测试完成 ==="
EOF

chmod +x full_test.sh
```

运行完整测试：
```bash
sudo ./full_test.sh
```