#!/bin/bash
#==============================================================================
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
#
#==============================================================================
#
# ARM架构兼容性检查和修复脚本
# 适用于NVIDIA Jetson等ARM64平台
#
#==============================================================================

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# 检测系统架构
detect_architecture() {
    ARCH=$(uname -m)
    log_info "检测到系统架构: $ARCH"
    
    case $ARCH in
        x86_64)
            log_info "x86_64架构，无需特殊处理"
            return 0
            ;;
        aarch64)
            log_info "ARM64架构，执行兼容性检查"
            return 1
            ;;
        armv7l)
            log_info "ARMv7架构，执行兼容性检查"
            return 1
            ;;
        *)
            log_warn "未知架构: $ARCH"
            return 2
            ;;
    esac
}

# 检查编译工具链
check_toolchain() {
    log_info "检查编译工具链..."
    
    # 检查GCC
    if ! command -v gcc &> /dev/null; then
        log_error "GCC编译器未安装"
        echo "请安装GCC: sudo apt update && sudo apt install build-essential"
        return 1
    fi
    
    GCC_VERSION=$(gcc --version | head -n1)
    log_info "GCC版本: $GCC_VERSION"
    
    # 检查make
    if ! command -v make &> /dev/null; then
        log_error "Make工具未安装"
        echo "请安装Make: sudo apt update && sudo apt install make"
        return 1
    fi
    
    return 0
}

# 检查系统依赖
check_system_dependencies() {
    log_info "检查系统依赖..."
    
    # 检查头文件
    MISSING_HEADERS=()
    
    if [ ! -f "/usr/include/stdint.h" ]; then
        MISSING_HEADERS+=("stdint.h")
    fi
    
    if [ ! -f "/usr/include/sys/mman.h" ]; then
        MISSING_HEADERS+=("sys/mman.h")
    fi
    
    if [ ! -f "/usr/include/unistd.h" ]; then
        MISSING_HEADERS+=("unistd.h")
    fi
    
    if [ ${#MISSING_HEADERS[@]} -gt 0 ]; then
        log_error "缺少头文件: ${MISSING_HEADERS[*]}"
        echo "请安装开发包: sudo apt update && sudo apt install libc6-dev"
        return 1
    fi
    
    log_success "系统依赖检查通过"
    return 0
}

# 检查PCIe设备
check_pcie_devices() {
    log_info "检查PCIe设备..."
    
    # 查找Xilinx设备
    XILINX_DEVICES=$(lspci | grep -i xilinx || true)
    if [ -z "$XILINX_DEVICES" ]; then
        log_warn "未发现Xilinx PCIe设备"
        log_warn "请确认RecoNIC硬件已正确安装"
    else
        log_success "发现Xilinx PCIe设备:"
        echo "$XILINX_DEVICES"
        
        # 检查resource文件
        for device in $(lspci | grep -i xilinx | awk '{print $1}'); do
            RESOURCE_PATH="/sys/bus/pci/devices/0000:${device}/resource2"
            if [ -f "$RESOURCE_PATH" ]; then
                log_success "找到PCIe资源文件: $RESOURCE_PATH"
            else
                log_warn "PCIe资源文件不存在: $RESOURCE_PATH"
            fi
        done
    fi
}

# 检查内核模块
check_kernel_modules() {
    log_info "检查内核模块..."
    
    # 检查onic模块
    if lsmod | grep -q "^onic"; then
        log_success "onic内核模块已加载"
    else
        log_warn "onic内核模块未加载"
        log_info "请加载内核模块: sudo insmod ../../onic-driver/onic.ko"
    fi
    
    # 检查字符设备
    if [ -c "/dev/reconic-mm" ]; then
        log_success "字符设备 /dev/reconic-mm 存在"
    else
        log_warn "字符设备 /dev/reconic-mm 不存在"
        log_info "请确认驱动已正确安装"
    fi
}

# ARM特定优化
apply_arm_optimizations() {
    log_info "应用ARM架构优化..."
    
    # 检查CPU信息
    if [ -f "/proc/cpuinfo" ]; then
        CPU_MODEL=$(grep "model name" /proc/cpuinfo | head -n1 | cut -d':' -f2 | xargs)
        log_info "CPU型号: $CPU_MODEL"
        
        # 针对NVIDIA Jetson的特殊处理
        if echo "$CPU_MODEL" | grep -qi "nvidia\|tegra\|jetson"; then
            log_info "检测到NVIDIA Jetson平台，应用特殊优化"
            export JETSON_PLATFORM=1
        fi
    fi
    
    # 设置ARM特定的编译标志
    export ARM_CFLAGS="-mcpu=native -mtune=native -O2"
    log_info "设置ARM编译标志: $ARM_CFLAGS"
    
    return 0
}

# 编译测试
test_compilation() {
    log_info "测试编译..."
    
    # 检查libreconic库
    LIBRECONIC_PATH="../../lib/libreconic.a"
    if [ ! -f "$LIBRECONIC_PATH" ]; then
        log_warn "libreconic库不存在，尝试编译..."
        if make -C ../../lib; then
            log_success "libreconic库编译成功"
        else
            log_error "libreconic库编译失败"
            return 1
        fi
    else
        log_success "libreconic库已存在"
    fi
    
    # 编译寄存器测试程序
    if make clean && make; then
        log_success "寄存器测试程序编译成功"
        
        # 检查可执行文件架构
        if command -v file &> /dev/null; then
            FILE_INFO=$(file register_test)
            log_info "可执行文件信息: $FILE_INFO"
        fi
        
        return 0
    else
        log_error "寄存器测试程序编译失败"
        return 1
    fi
}

# 生成测试脚本
generate_test_script() {
    log_info "生成测试脚本..."
    
    cat > test_register_access.sh << 'EOF'
#!/bin/bash
# RecoNIC寄存器访问测试脚本

echo "=== RecoNIC寄存器访问测试 ==="

# 检查权限
if [ "$(id -u)" != "0" ]; then
    echo "错误：需要root权限"
    echo "请使用: sudo $0"
    exit 1
fi

echo "1. 显示帮助信息..."
./register_test -h

echo "2. 尝试读取版本寄存器（如果硬件可用）..."
if ./register_test -r 0x102000 2>/dev/null; then
    echo "硬件访问成功"
else
    echo "硬件访问失败（可能硬件不可用）"
fi

echo "测试完成"
EOF

    chmod +x test_register_access.sh
    log_success "测试脚本已生成: test_register_access.sh"
}

# 主函数
main() {
    log_info "开始ARM架构兼容性检查..."
    
    detect_architecture
    ARCH_RESULT=$?
    
    if ! check_toolchain; then
        log_error "工具链检查失败"
        exit 1
    fi
    
    if ! check_system_dependencies; then
        log_error "系统依赖检查失败"
        exit 1
    fi
    
    check_pcie_devices
    check_kernel_modules
    
    if [ $ARCH_RESULT -eq 1 ]; then
        apply_arm_optimizations
    fi
    
    if test_compilation; then
        log_success "编译测试通过"
    else
        log_error "编译测试失败"
        exit 1
    fi
    
    generate_test_script
    
    log_success "ARM架构兼容性检查完成！"
    
    echo ""
    echo "=== 使用说明 ==="
    echo "1. 运行基本测试: sudo ./test_register_access.sh"
    echo "2. 读取寄存器:   sudo ./register_test -r 0x102000"
    echo "3. 交互模式:     sudo ./register_test -i"
    echo "4. 查看帮助:     ./register_test -h"
    echo ""
}

# 运行主函数
main "$@"
