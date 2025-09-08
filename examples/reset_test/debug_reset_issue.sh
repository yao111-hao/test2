#!/bin/bash
#==============================================================================
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
#
#==============================================================================
#
# RecoNIC复位问题调试脚本
# 收集详细的调试信息用于分析硬件复位问题
#
#==============================================================================

# 颜色定义
GREEN='\033[0;32m'
BLUE='\033[0;34m' 
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

LOG_DIR="./debug_logs"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

echo -e "${BLUE}=== RecoNIC复位问题调试数据收集 ===${NC}"
echo "时间戳: $TIMESTAMP"
echo ""

# 创建日志目录
mkdir -p $LOG_DIR
cd $LOG_DIR

echo -e "${YELLOW}步骤1：检查权限${NC}"
if [ "$(id -u)" != "0" ]; then
    echo -e "${RED}错误：需要root权限${NC}"
    echo "请使用: sudo $0"
    exit 1
fi
echo -e "${GREEN}✓ Root权限确认${NC}"

echo -e "${YELLOW}步骤2：检查环境设置${NC}"
export LD_LIBRARY_PATH=/workspace/lib:$LD_LIBRARY_PATH
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo -e "${GREEN}✓ 环境变量设置完成${NC}"

echo -e "${YELLOW}步骤3：收集系统基础信息${NC}"
echo "=== 系统信息 ===" > system_info_${TIMESTAMP}.txt
uname -a >> system_info_${TIMESTAMP}.txt
cat /proc/cpuinfo | head -20 >> system_info_${TIMESTAMP}.txt
free -h >> system_info_${TIMESTAMP}.txt
echo -e "${GREEN}✓ 系统信息已收集${NC}"

echo -e "${YELLOW}步骤4：检查PCIe设备状态${NC}"
echo "=== PCIe设备信息 ===" > pcie_info_${TIMESTAMP}.txt
lspci | grep -i xilinx >> pcie_info_${TIMESTAMP}.txt
lspci -v | grep -A 30 Xilinx >> pcie_info_${TIMESTAMP}.txt
ls -la /sys/bus/pci/devices/*/resource2 >> pcie_info_${TIMESTAMP}.txt
echo -e "${GREEN}✓ PCIe信息已收集${NC}"

echo -e "${YELLOW}步骤5：检查驱动状态${NC}"
echo "=== 驱动信息 ===" > driver_info_${TIMESTAMP}.txt
lsmod | grep onic >> driver_info_${TIMESTAMP}.txt
ls -la /dev/reconic-mm >> driver_info_${TIMESTAMP}.txt
cat /proc/interrupts | grep onic >> driver_info_${TIMESTAMP}.txt
dmesg | grep -i onic | tail -20 >> driver_info_${TIMESTAMP}.txt
echo -e "${GREEN}✓ 驱动信息已收集${NC}"

echo -e "${YELLOW}步骤6：收集复位前状态${NC}"
echo "=== 复位前寄存器状态 ===" > before_reset_${TIMESTAMP}.txt
../test_fixed_reset --status >> before_reset_${TIMESTAMP}.txt 2>&1
echo -e "${GREEN}✓ 复位前状态已收集${NC}"

echo -e "${YELLOW}步骤7：测试原始复位功能${NC}"
echo "=== 原始复位测试 ===" > original_reset_test_${TIMESTAMP}.txt
echo "测试CMAC端口0复位（原始版本）..." >> original_reset_test_${TIMESTAMP}.txt
../reset_test --cmac-port 0 --verbose >> original_reset_test_${TIMESTAMP}.txt 2>&1
echo -e "${GREEN}✓ 原始复位测试完成${NC}"

echo -e "${YELLOW}步骤8：收集原始复位后状态${NC}"
echo "=== 原始复位后状态 ===" > after_original_reset_${TIMESTAMP}.txt
../test_fixed_reset --status >> after_original_reset_${TIMESTAMP}.txt 2>&1
echo -e "${GREEN}✓ 原始复位后状态已收集${NC}"

echo -e "${YELLOW}步骤9：测试修复版复位功能${NC}"
echo "=== 修复版复位测试 ===" > fixed_reset_test_${TIMESTAMP}.txt
echo "测试CMAC端口0复位（修复版本）..." >> fixed_reset_test_${TIMESTAMP}.txt
echo "y" | ../test_fixed_reset --cmac-port 0 --verbose >> fixed_reset_test_${TIMESTAMP}.txt 2>&1
echo -e "${GREEN}✓ 修复版复位测试完成${NC}"

echo -e "${YELLOW}步骤10：收集修复版复位后状态${NC}"
echo "=== 修复版复位后状态 ===" > after_fixed_reset_${TIMESTAMP}.txt
../test_fixed_reset --status >> after_fixed_reset_${TIMESTAMP}.txt 2>&1
echo -e "${GREEN}✓ 修复版复位后状态已收集${NC}"

echo -e "${YELLOW}步骤11：网络连接测试${NC}"
echo "=== 网络接口状态 ===" > network_status_${TIMESTAMP}.txt
ip link show >> network_status_${TIMESTAMP}.txt
ip addr show >> network_status_${TIMESTAMP}.txt
echo "" >> network_status_${TIMESTAMP}.txt

# 尝试找到onic接口
ONIC_INTERFACE=$(ip link show | grep -E "onic|ens[0-9]" | head -1 | cut -d':' -f2 | tr -d ' ')
if [ -n "$ONIC_INTERFACE" ]; then
    echo "找到网络接口: $ONIC_INTERFACE" >> network_status_${TIMESTAMP}.txt
    ethtool $ONIC_INTERFACE >> network_status_${TIMESTAMP}.txt 2>&1
else
    echo "未找到onic网络接口" >> network_status_${TIMESTAMP}.txt
fi

echo -e "${GREEN}✓ 网络状态已收集${NC}"

echo -e "${YELLOW}步骤12：生成调试报告${NC}"
cat > debug_report_${TIMESTAMP}.md << EOF
# RecoNIC复位问题调试报告

## 测试时间
$TIMESTAMP

## 系统环境
- 架构: $(uname -m)
- 内核: $(uname -r)
- 发行版: $(cat /etc/os-release | grep PRETTY_NAME | cut -d'=' -f2)

## PCIe设备
$(lspci | grep -i xilinx | head -1)

## 驱动状态
- onic模块: $(lsmod | grep onic | wc -l > 0 && echo "已加载" || echo "未加载")
- 字符设备: $(ls /dev/reconic-mm &>/dev/null && echo "存在" || echo "不存在")

## 测试结果概述

### 原始复位测试
请查看: original_reset_test_${TIMESTAMP}.txt

### 修复版复位测试  
请查看: fixed_reset_test_${TIMESTAMP}.txt

### 状态对比
- 复位前: before_reset_${TIMESTAMP}.txt
- 原始复位后: after_original_reset_${TIMESTAMP}.txt
- 修复版复位后: after_fixed_reset_${TIMESTAMP}.txt

## 问题分析

基于收集的日志，请重点关注：

1. **Lane对齐状态** - CMAC_OFFSET_STAT_RX_STATUS的值是否为0x3
2. **复位完成标志** - Shell状态寄存器的相应位是否置位
3. **错误信息** - 复位过程中是否有超时或失败

## 下一步分析

需要结合Verilog代码分析：
1. 复位信号的硬件实现
2. 状态寄存器的更新时序
3. Lane对齐的硬件机制
4. 可能的硬件Bug或特殊要求

EOF

echo -e "${GREEN}✓ 调试报告已生成${NC}"

echo ""
echo -e "${BLUE}=== 调试数据收集完成 ===${NC}"
echo "调试文件保存在: $LOG_DIR/"
echo ""
echo "收集的文件:"
ls -la ${LOG_DIR}/*${TIMESTAMP}*
echo ""
echo -e "${YELLOW}请将这些调试日志和Verilog代码一起提供给开发人员进行分析${NC}"