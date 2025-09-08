#!/bin/bash
#==============================================================================
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
#==============================================================================
#
# RecoNIC复位问题最终解决方案 - 使用指南
# 基于Verilog代码深度分析的完整修复版本
#
#==============================================================================

GREEN='\033[0;32m'
BLUE='\033[0;34m' 
YELLOW='\033[1;33m'
RED='\033[0;31m'
PURPLE='\033[0;35m'
NC='\033[0m'

echo -e "${BLUE}=== RecoNIC复位问题最终解决方案 ===${NC}"
echo -e "${PURPLE}基于Verilog代码深度分析的完整修复版本${NC}\n"

# 显示关键发现
echo -e "${YELLOW}🔍 通过Verilog代码分析发现的根本问题：${NC}"
echo -e "${RED}1. 复位寄存器是自清除的${NC} - 我之前写入后立即返回，没等待硬件清零"
echo -e "${RED}2. CMAC适配器复位缺失${NC} - 我只复位了子系统，忘记了适配器模块" 
echo -e "${RED}3. 状态检测时序错误${NC} - 我在复位期间检查状态，应该等复位完成"
echo -e "${RED}4. 缺少硬件稳定时间${NC} - 复位后需要等待硬件完全稳定\n"

echo -e "${YELLOW}✅ 基于硬件分析的完整修复：${NC}"
echo -e "${GREEN}1. 等待复位寄存器自清除机制${NC} - 真正等待硬件完成"
echo -e "${GREEN}2. 同时复位CMAC子系统和适配器${NC} - 位4+5（端口0）或位8+9（端口1）"
echo -e "${GREEN}3. 正确的复位完成检测${NC} - 基于硬件实际状态"
echo -e "${GREEN}4. 适当的硬件稳定等待${NC} - 匹配硬件时序要求\n"

# 检查编译状态
if [ ! -f "./test_final_reset" ]; then
    echo -e "${YELLOW}步骤1：编译最终修复版本${NC}"
    echo "make -f Makefile_final final"
    if make -f Makefile_final final; then
        echo -e "${GREEN}✓ 最终修复版编译成功${NC}\n"
    else
        echo -e "${RED}✗ 编译失败${NC}"
        exit 1
    fi
else
    echo -e "${GREEN}✓ 最终修复版已编译就绪${NC}\n"
fi

# 设置环境
echo -e "${YELLOW}步骤2：设置环境${NC}"
export LD_LIBRARY_PATH=/workspace/lib:$LD_LIBRARY_PATH
echo "export LD_LIBRARY_PATH=/workspace/lib:\$LD_LIBRARY_PATH"
echo -e "${GREEN}✓ 环境设置完成${NC}\n"

# 权限检查
echo -e "${YELLOW}步骤3：权限检查${NC}"
if [ "$(id -u)" != "0" ]; then
    echo -e "${RED}需要root权限进行硬件操作${NC}"
    echo "请使用: sudo $0"
    echo ""
    echo -e "${BLUE}或者手动执行以下命令：${NC}"
    echo "cd /workspace/examples/reset_test"
    echo "export LD_LIBRARY_PATH=/workspace/lib:\$LD_LIBRARY_PATH"
    echo "sudo ./test_final_reset --diagnose"
    echo "sudo ./test_final_reset --smart"
    exit 1
else
    echo -e "${GREEN}✓ Root权限确认${NC}\n"
fi

# 显示使用选项
echo -e "${YELLOW}步骤4：选择测试方案${NC}"
echo "请选择要执行的测试："
echo "1) 硬件状态诊断（推荐第一步）"
echo "2) 智能复位策略（自动选择最佳方案）"
echo "3) CMAC端口0复位（单独复位）"
echo "4) CMAC端口1复位（单独复位）"
echo "5) Shell层完整复位（影响所有子模块）"
echo "6) 系统级完整复位（影响整个系统）"
echo "7) 显示所有选项的详细说明"
echo "0) 退出"
echo ""

read -p "请选择 (0-7): " choice

case $choice in
    1)
        echo -e "${BLUE}=== 执行硬件状态诊断 ===${NC}"
        ./test_final_reset --diagnose
        ;;
    2)
        echo -e "${BLUE}=== 执行智能复位策略 ===${NC}"
        echo "智能复位会先诊断状态，然后自动选择最佳复位方案"
        ./test_final_reset --smart --verbose
        ;;
    3)
        echo -e "${BLUE}=== 执行CMAC端口0复位 ===${NC}"
        echo "这会同时复位CMAC0子系统和适配器（位4+5）"
        ./test_final_reset --cmac-port 0 --verbose
        ;;
    4)
        echo -e "${BLUE}=== 执行CMAC端口1复位 ===${NC}"
        echo "这会同时复位CMAC1子系统和适配器（位8+9）"
        ./test_final_reset --cmac-port 1 --verbose
        ;;
    5)
        echo -e "${BLUE}=== 执行Shell层完整复位 ===${NC}"
        echo "⚠️ 警告：这会复位所有Shell层子模块"
        ./test_final_reset --shell --verbose
        ;;
    6)
        echo -e "${BLUE}=== 执行系统级完整复位 ===${NC}"
        echo "⚠️ 警告：这会复位整个RecoNIC系统"
        ./test_final_reset --system --verbose
        ;;
    7)
        echo -e "${BLUE}=== 详细使用说明 ===${NC}"
        ./test_final_reset -h
        ;;
    0)
        echo "退出"
        exit 0
        ;;
    *)
        echo -e "${RED}无效选择${NC}"
        exit 1
        ;;
esac

# 测试后的网络验证提醒
echo ""
echo -e "${YELLOW}=== 复位操作完成 ===${NC}"
echo -e "${PURPLE}现在请立即测试网络连接：${NC}"
echo ""
echo -e "${BLUE}1. 测试ping连接：${NC}"
echo "   ping <对端设备IP>"
echo ""
echo -e "${BLUE}2. 检查网络接口：${NC}"
echo "   ip link show"
echo "   ethtool <interface_name>"
echo ""
echo -e "${BLUE}3. 预期结果：${NC}"
echo -e "${GREEN}   ✅ ping应该立即成功${NC}"
echo -e "${GREEN}   ✅ 网络接口应该显示UP状态${NC}"
echo -e "${GREEN}   ✅ 无需重启主机${NC}"
echo ""
echo -e "${PURPLE}如果ping成功 -> 🎉 问题彻底解决！${NC}"
echo -e "${PURPLE}如果仍然失败 -> 请联系开发人员进行进一步分析${NC}"