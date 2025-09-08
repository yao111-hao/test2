#!/bin/bash
#==============================================================================
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
#
#==============================================================================
#
# RecoNIC寄存器测试程序 - 测试命令脚本
# 提供完整的编译和测试流程
#
#==============================================================================

# 颜色定义
GREEN='\033[0;32m'
BLUE='\033[0;34m' 
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}=== RecoNIC寄存器测试程序 - 编译与测试 ===${NC}\n"

# 步骤1：编译libreconic库
echo -e "${YELLOW}步骤1：编译RecoNIC用户空间库${NC}"
echo "cd ../../lib && make"
cd ../../lib && make
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ libreconic库编译成功${NC}\n"
else
    echo -e "${RED}✗ libreconic库编译失败${NC}\n"
    exit 1
fi

# 步骤2：编译寄存器测试程序
echo -e "${YELLOW}步骤2：编译寄存器测试程序${NC}"
echo "cd ../examples/register_test && make"
cd ../examples/register_test && make
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ 寄存器测试程序编译成功${NC}\n"
else
    echo -e "${RED}✗ 寄存器测试程序编译失败${NC}\n"
    exit 1
fi

# 步骤3：设置库路径
echo -e "${YELLOW}步骤3：设置库路径${NC}"
export LD_LIBRARY_PATH=/workspace/lib:$LD_LIBRARY_PATH
echo "export LD_LIBRARY_PATH=/workspace/lib:\$LD_LIBRARY_PATH"
echo -e "${GREEN}✓ 库路径设置完成${NC}\n"

# 步骤4：测试程序功能
echo -e "${YELLOW}步骤4：测试程序基本功能${NC}"
echo "./register_test -h"
./register_test -h
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ 程序功能测试正常${NC}\n"
else
    echo -e "${RED}✗ 程序功能测试失败${NC}\n"
    exit 1
fi

# 步骤5：显示使用示例
echo -e "${YELLOW}步骤5：使用示例${NC}"
echo -e "${BLUE}基本用法：${NC}"
echo "1. 读取版本寄存器："
echo "   sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./register_test -r 0x102000"
echo ""
echo "2. 写入计算控制寄存器："
echo "   sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./register_test -w 0x103000 -v 0x12345678"
echo ""
echo "3. 进入交互式模式："
echo "   sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./register_test -i"
echo ""
echo "4. 详细输出模式："
echo "   sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./register_test -V -r 0x102000"
echo ""

# 步骤6：ARM架构特殊说明
echo -e "${YELLOW}步骤6：ARM架构（如NVIDIA Jetson）特殊说明${NC}"
echo -e "${BLUE}ARM平台编译：${NC}"
echo "   make clean && make arm-optimize"
echo ""
echo -e "${BLUE}ARM平台兼容性检查：${NC}"
echo "   ./arm_compatibility_check.sh"
echo ""

# 步骤7：实际测试指导
echo -e "${YELLOW}步骤7：实际硬件测试指导${NC}"
echo -e "${BLUE}前置条件检查：${NC}"
echo "1. 确认RecoNIC硬件已安装："
echo "   lspci | grep -i xilinx"
echo ""
echo "2. 确认onic驱动已加载："
echo "   lsmod | grep onic"
echo "   # 如果未加载，执行："
echo "   sudo insmod ../../onic-driver/onic.ko"
echo ""
echo "3. 确认字符设备存在："
echo "   ls -la /dev/reconic-mm"
echo ""
echo "4. 查找正确的PCIe资源路径："
echo "   ls -la /sys/bus/pci/devices/*/resource2"
echo ""

# 步骤8：安全提醒
echo -e "${YELLOW}步骤8：安全注意事项${NC}"
echo -e "${RED}⚠️  重要提醒：${NC}"
echo "• 本程序需要root权限运行"
echo "• 寄存器操作可能影响硬件状态"
echo "• 某些寄存器为只读，写入无效"
echo "• 不当操作可能导致系统不稳定"
echo "• 建议先在测试环境中使用"
echo ""

# 步骤9：故障排除
echo -e "${YELLOW}步骤9：常见问题解决${NC}"
echo -e "${BLUE}问题1：libreconic.so找不到${NC}"
echo "解决：export LD_LIBRARY_PATH=/workspace/lib:\$LD_LIBRARY_PATH"
echo ""
echo -e "${BLUE}问题2：权限不足${NC}"
echo "解决：使用 sudo env LD_LIBRARY_PATH=\$LD_LIBRARY_PATH ./register_test ..."
echo ""
echo -e "${BLUE}问题3：PCIe设备路径错误${NC}"
echo "解决：使用 -p 参数指定正确的resource2路径"
echo ""

echo -e "${GREEN}=== 编译和配置完成！ ===${NC}"
echo -e "${GREEN}现在可以使用sudo权限运行寄存器测试程序了${NC}\n"