#!/bin/bash
#==============================================================================
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
#
#==============================================================================
#
# RecoNIC复位功能测试程序 - 完整使用指南
# 提供详细的编译、测试和使用说明
#
#==============================================================================

# 颜色定义
GREEN='\033[0;32m'
BLUE='\033[0;34m' 
YELLOW='\033[1;33m'
RED='\033[0;31m'
PURPLE='\033[0;35m'
NC='\033[0m'

echo -e "${BLUE}=== RecoNIC复位功能测试程序 - 完整使用指南 ===${NC}\n"

# 步骤1：编译程序
echo -e "${YELLOW}步骤1：编译复位测试程序${NC}"
echo "cd /workspace/examples/reset_test && make"
if cd /workspace/examples/reset_test && make; then
    echo -e "${GREEN}✓ 复位测试程序编译成功${NC}\n"
else
    echo -e "${RED}✗ 复位测试程序编译失败${NC}\n"
    exit 1
fi

# 步骤2：设置库路径
echo -e "${YELLOW}步骤2：设置库路径${NC}"
export LD_LIBRARY_PATH=/workspace/lib:$LD_LIBRARY_PATH
echo "export LD_LIBRARY_PATH=/workspace/lib:\$LD_LIBRARY_PATH"
echo -e "${GREEN}✓ 库路径设置完成${NC}\n"

# 步骤3：测试程序基本功能
echo -e "${YELLOW}步骤3：测试程序基本功能${NC}"
echo "./reset_test -h"
if ./reset_test -h > /dev/null 2>&1; then
    echo -e "${GREEN}✓ 程序功能测试正常${NC}\n"
else
    echo -e "${RED}✗ 程序功能测试失败${NC}\n"
    exit 1
fi

# 步骤4：复位功能说明
echo -e "${YELLOW}步骤4：复位功能详细说明${NC}"
echo -e "${PURPLE}支持的复位类型：${NC}"
echo "1. 系统复位     - 完整的系统复位，重置整个RecoNIC系统"
echo "   寄存器地址：0x0004"
echo "   使用命令：sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./reset_test --system"
echo ""
echo "2. Shell层复位  - 重置网卡Shell层各个子模块"
echo "   寄存器地址：0x000C"
echo "   使用命令：sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./reset_test --shell"
echo ""
echo "3. 用户复位     - 重置用户可编程逻辑部分"
echo "   寄存器地址：0x0014"
echo "   使用命令：sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./reset_test --user"
echo ""
echo "4. CMAC端口复位 - 重置指定的100G以太网端口"
echo "   控制方式：通过Shell层复位寄存器"
echo "   使用命令：sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./reset_test --cmac-port [0|1]"
echo ""
echo "5. CMAC GT复位  - 重置GT收发器"
echo "   端口0地址：0x8000，端口1地址：0xC000"
echo "   使用命令：sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./reset_test --cmac-gt [0|1]"
echo ""

# 步骤5：基本使用示例
echo -e "${YELLOW}步骤5：基本使用示例${NC}"
echo -e "${BLUE}1. 查看复位状态（无需硬件）：${NC}"
echo "   sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./reset_test --status"
echo ""
echo -e "${BLUE}2. 交互式模式（推荐）：${NC}"
echo "   sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./reset_test --interactive"
echo ""
echo -e "${BLUE}3. 单次复位操作：${NC}"
echo "   sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./reset_test --user        # 用户复位"
echo "   sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./reset_test --cmac-port 0 # CMAC端口0复位"
echo "   sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./reset_test --system      # 系统复位"
echo ""
echo -e "${BLUE}4. 强制模式（跳过确认）：${NC}"
echo "   sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./reset_test --user --force"
echo ""
echo -e "${BLUE}5. 详细输出模式：${NC}"
echo "   sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./reset_test --status --verbose"
echo ""

# 步骤6：硬件测试前的检查
echo -e "${YELLOW}步骤6：硬件测试前的检查${NC}"
echo -e "${BLUE}执行实际硬件测试前，请确认：${NC}"
echo "1. RecoNIC硬件已正确安装："
echo "   lspci | grep -i xilinx"
echo ""
echo "2. onic驱动已加载："
echo "   lsmod | grep onic"
echo "   # 如果未加载，执行："
echo "   # sudo insmod ../../onic-driver/onic.ko"
echo ""
echo "3. 字符设备存在："
echo "   ls -la /dev/reconic-mm"
echo ""
echo "4. PCIe资源文件可访问："
echo "   ls -la /sys/bus/pci/devices/*/resource2"
echo ""

# 步骤7：ARM架构特殊说明
echo -e "${YELLOW}步骤7：ARM架构（如NVIDIA Jetson）特殊说明${NC}"
echo -e "${BLUE}ARM平台编译和使用：${NC}"
echo "1. ARM架构优化编译："
echo "   make clean && make arm-optimize"
echo ""
echo "2. 架构验证："
echo "   file reset_test  # 检查可执行文件架构"
echo "   uname -m         # 确认系统架构"
echo ""
echo "3. ARM平台可能的PCIe路径差异："
echo "   # 查找正确的PCIe设备路径"
echo "   lspci | grep -i xilinx"
echo "   ls -la /sys/bus/pci/devices/0000:*/resource2"
echo "   # 使用正确的路径"
echo "   sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./reset_test -p /path/to/correct/resource2 --status"
echo ""

# 步骤8：安全注意事项和最佳实践
echo -e "${YELLOW}步骤8：安全注意事项和最佳实践${NC}"
echo -e "${RED}⚠️  重要安全提醒：${NC}"
echo "• 复位操作会中断网络连接，请在维护窗口执行"
echo "• 系统复位影响范围最大，请谨慎使用"
echo "• 建议按影响程度递增顺序尝试复位："
echo "  1) GT复位 → 2) CMAC端口复位 → 3) 用户复位 → 4) Shell层复位 → 5) 系统复位"
echo "• 复位前请备份重要的网络配置"
echo "• 如果复位后系统无响应，请检查驱动状态或重启系统"
echo ""

# 步骤9：故障排除
echo -e "${YELLOW}步骤9：故障排除${NC}"
echo -e "${BLUE}常见问题及解决方案：${NC}"
echo "1. 权限错误："
echo "   错误：本程序需要root权限运行"
echo "   解决：使用 sudo 运行"
echo ""
echo "2. 库文件找不到："
echo "   错误：libreconic.so: cannot open shared object file"
echo "   解决：export LD_LIBRARY_PATH=/workspace/lib:\$LD_LIBRARY_PATH"
echo ""
echo "3. PCIe设备访问失败："
echo "   错误：无法打开PCIe资源文件"
echo "   解决：检查设备路径和驱动状态"
echo ""
echo "4. 复位超时："
echo "   错误：复位超时或失败"
echo "   解决：检查硬件状态和连接"
echo ""

# 步骤10：创建快捷脚本
echo -e "${YELLOW}步骤10：创建便利脚本${NC}"

# 创建状态检查脚本
cat > check_reset_status.sh << 'EOF'
#!/bin/bash
# 快速检查RecoNIC复位状态
export LD_LIBRARY_PATH=/workspace/lib:$LD_LIBRARY_PATH
if [ "$(id -u)" != "0" ]; then
    echo "需要root权限，尝试使用sudo..."
    sudo env LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./reset_test --status --verbose
else
    ./reset_test --status --verbose
fi
EOF

# 创建交互模式脚本
cat > interactive_reset.sh << 'EOF'
#!/bin/bash
# 快速进入RecoNIC复位交互模式
export LD_LIBRARY_PATH=/workspace/lib:$LD_LIBRARY_PATH
if [ "$(id -u)" != "0" ]; then
    echo "需要root权限，尝试使用sudo..."
    sudo env LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./reset_test --interactive
else
    ./reset_test --interactive
fi
EOF

# 创建安全复位脚本
cat > safe_reset.sh << 'EOF'
#!/bin/bash
# 安全的逐步复位脚本
export LD_LIBRARY_PATH=/workspace/lib:$LD_LIBRARY_PATH

echo "=== RecoNIC安全复位向导 ==="
echo "此脚本将按影响程度递增顺序提供复位选项"
echo ""

if [ "$(id -u)" != "0" ]; then
    echo "错误：需要root权限"
    echo "请使用：sudo $0"
    exit 1
fi

echo "1. 查看当前复位状态"
./reset_test --status

echo ""
echo "选择复位类型："
echo "1) GT复位（影响最小）"
echo "2) CMAC端口复位（影响中等）"
echo "3) 用户逻辑复位（影响中等）"
echo "4) Shell层复位（影响较大）"
echo "5) 系统复位（影响最大）"
echo "0) 退出"
echo ""

read -p "请选择 (0-5): " choice

case $choice in
    1)
        read -p "选择GT端口 (0/1): " port
        ./reset_test --cmac-gt $port
        ;;
    2)
        read -p "选择CMAC端口 (0/1): " port
        ./reset_test --cmac-port $port
        ;;
    3)
        ./reset_test --user
        ;;
    4)
        ./reset_test --shell
        ;;
    5)
        ./reset_test --system
        ;;
    0)
        echo "退出"
        ;;
    *)
        echo "无效选择"
        ;;
esac
EOF

chmod +x check_reset_status.sh interactive_reset.sh safe_reset.sh

echo -e "${GREEN}✓ 便利脚本已创建：${NC}"
echo "  ./check_reset_status.sh  - 快速状态检查"
echo "  ./interactive_reset.sh   - 交互式复位模式"  
echo "  ./safe_reset.sh          - 安全复位向导"
echo ""

echo -e "${GREEN}=== 复位功能测试程序配置完成！ ===${NC}"
echo ""
echo -e "${PURPLE}现在您可以开始使用RecoNIC复位功能了：${NC}"
echo "1. 快速状态检查：./check_reset_status.sh"
echo "2. 交互式操作：./interactive_reset.sh" 
echo "3. 安全复位向导：./safe_reset.sh"
echo ""
echo -e "${RED}⚠️  提醒：请在维护窗口期间执行复位操作！${NC}"
