//==============================================================================
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//
//==============================================================================

/**
 * @file test_final_reset.c
 * @brief 基于Verilog代码分析的最终修复版复位测试程序
 * 
 * 基于system_config_register.v的硬件实现分析，修复了所有软硬件不匹配问题：
 * 1. 复位寄存器的自清除机制
 * 2. CMAC子系统和适配器的同时复位
 * 3. 正确的复位完成检测
 * 4. 适当的硬件稳定等待时间
 */

#include "reset_utils.h"
#include <getopt.h>

/* 来自reset_utils_final.c的函数声明 */
extern int perform_cmac_reset_hardware_correct(uint32_t* pcie_base, int port_id, int verbose);
extern int perform_shell_reset_hardware_correct(uint32_t* pcie_base, int verbose);
extern int perform_system_reset_hardware_correct(uint32_t* pcie_base, int verbose);
extern int smart_reset_strategy(uint32_t* pcie_base, int port_id, int include_reinit, int verbose);
extern int diagnose_reset_issues(uint32_t* pcie_base);

/* 全局变量：详细输出模式标志 */
int verbose = 0;

/* 命令行选项配置表 */
static struct option const long_opts[] = {
    {"pcie_resource", required_argument, NULL, 'p'},
    {"system", no_argument, NULL, 's'},
    {"shell", no_argument, NULL, 'S'},
    {"cmac-port", required_argument, NULL, 'c'},
    {"cmac-all", no_argument, NULL, 'C'},
    {"diagnose", no_argument, NULL, 'd'},
    {"smart", no_argument, NULL, 'm'},
    {"status", no_argument, NULL, 't'},
    {"help", no_argument, NULL, 'h'},
    {"verbose", no_argument, NULL, 'V'},
    {0, 0, 0, 0}
};

/**
 * @brief 打印程序使用说明
 */
static void usage(const char *name)
{
    printf("\n=== RecoNIC最终修复版复位功能测试程序 ===\n");
    printf("基于Verilog硬件代码分析，修复了所有软硬件不匹配问题\n\n");
    printf("usage: %s [OPTIONS]\n\n", name);
    
    printf("选项说明：\n");
    printf("  -p (--pcie_resource) PCIe资源路径\n");
    printf("  -s (--system) 系统级硬件复位\n");
    printf("  -S (--shell) Shell层硬件复位\n");
    printf("  -c (--cmac-port) CMAC端口硬件复位（参数：0或1）\n");
    printf("  -C (--cmac-all) 所有CMAC端口硬件复位\n");
    printf("  -m (--smart) 智能复位策略（推荐）\n");
    printf("  -d (--diagnose) 硬件复位状态诊断\n");
    printf("  -t (--status) 显示复位状态\n");
    printf("  -V (--verbose) 启用详细输出\n");
    printf("  -h (--help) 显示帮助信息\n");
    
    printf("\n使用示例：\n");
    printf("  硬件诊断（推荐第一步）：\n");
    printf("    sudo %s --diagnose\n", name);
    printf("  智能复位策略（推荐）：\n");
    printf("    sudo %s --smart\n", name);
    printf("  单端口硬件复位：\n");
    printf("    sudo %s --cmac-port 0\n", name);
    printf("  完整系统硬件复位：\n");
    printf("    sudo %s --system\n", name);
    
    printf("\n硬件分析关键修复：\n");
    printf("  ✅ 复位寄存器自清除机制 - 等待硬件自动清零\n");
    printf("  ✅ CMAC适配器复位 - 同时复位子系统和适配器\n");
    printf("  ✅ 正确的复位完成检测 - 基于硬件实际实现\n");
    printf("  ✅ 适当的稳定等待时间 - 匹配硬件时序要求\n");
    
    printf("\n注意：这是基于Verilog代码分析的最终修复版本\n");
    printf("应该能够彻底解决复位后无法ping通的问题\n\n");
}

/**
 * @brief 主函数
 */
int main(int argc, char *argv[])
{
    int cmd_opt;
    char* pcie_resource = "/sys/bus/pci/devices/0000:d8:00.0/resource2";
    void* pcie_mapped_addr = NULL;
    int pcie_fd = -1;
    int ret = 0;
    int operation_specified = 0;
    int operation_type = 0;
    int cmac_port = -1;
    
    /* 解析命令行参数 */
    while ((cmd_opt = getopt_long(argc, argv, "p:sSc:CdmthV", long_opts, NULL)) != -1) {
        switch (cmd_opt) {
            case 'p':
                pcie_resource = strdup(optarg);
                break;
            case 's':
                operation_type = 1; // 系统复位
                operation_specified = 1;
                break;
            case 'S':
                operation_type = 2; // Shell复位
                operation_specified = 1;
                break;
            case 'c':
                cmac_port = (int)getopt_integer(optarg);
                operation_type = 3; // 单端口复位
                operation_specified = 1;
                break;
            case 'C':
                operation_type = 4; // 所有端口复位
                operation_specified = 1;
                break;
            case 'm':
                operation_type = 5; // 智能复位
                operation_specified = 1;
                break;
            case 'd':
                operation_type = 6; // 诊断
                operation_specified = 1;
                break;
            case 't':
                operation_type = 7; // 状态
                operation_specified = 1;
                break;
            case 'V':
                verbose = 1;
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            default:
                usage(argv[0]);
                return -EINVAL;
        }
    }
    
    if (!operation_specified) {
        fprintf(stderr, "错误：请指定操作类型\n");
        usage(argv[0]);
        return -EINVAL;
    }
    
    /* 检查权限 */
    if (geteuid() != 0) {
        fprintf(stderr, "错误：需要root权限运行\n");
        return -EPERM;
    }
    
    printf("=== RecoNIC最终修复版复位功能测试 ===\n");
    printf("基于Verilog硬件分析的完整修复版本\n");
    printf("PCIe资源：%s\n\n", pcie_resource);
    
    /* 初始化PCIe BAR空间映射 */
    pcie_mapped_addr = init_pcie_bar_mapping(pcie_resource, &pcie_fd);
    if (!pcie_mapped_addr) {
        fprintf(stderr, "错误：PCIe BAR空间映射失败\n");
        return -EIO;
    }
    
    /* 执行操作 */
    switch (operation_type) {
        case 1: /* 系统复位 */
            printf("⚠️ 警告：即将执行系统级硬件复位\n");
            printf("这会重置整个RecoNIC系统，影响所有功能模块\n");
            printf("确认继续？(y/N): ");
            fflush(stdout);
            {
                char confirm[10];
                if (fgets(confirm, sizeof(confirm), stdin) && 
                    (confirm[0] == 'y' || confirm[0] == 'Y')) {
                    ret = perform_system_reset_hardware_correct((uint32_t*)pcie_mapped_addr, verbose);
                } else {
                    printf("操作取消\n");
                }
            }
            break;
            
        case 2: /* Shell复位 */
            printf("⚠️ 警告：即将执行Shell层硬件复位\n");
            printf("这会重置所有Shell层子模块（QDMA、RDMA、CMAC等）\n");
            printf("确认继续？(y/N): ");
            fflush(stdout);
            {
                char confirm[10];
                if (fgets(confirm, sizeof(confirm), stdin) && 
                    (confirm[0] == 'y' || confirm[0] == 'Y')) {
                    ret = perform_shell_reset_hardware_correct((uint32_t*)pcie_mapped_addr, verbose);
                } else {
                    printf("操作取消\n");
                }
            }
            break;
            
        case 3: /* 单端口复位 */
            if (cmac_port != 0 && cmac_port != 1) {
                fprintf(stderr, "错误：无效的CMAC端口号\n");
                ret = -EINVAL;
                break;
            }
            printf("即将执行CMAC端口%d硬件级复位\n", cmac_port);
            printf("将同时复位CMAC子系统和适配器，确认继续？(y/N): ");
            fflush(stdout);
            {
                char confirm[10];
                if (fgets(confirm, sizeof(confirm), stdin) && 
                    (confirm[0] == 'y' || confirm[0] == 'Y')) {
                    ret = perform_cmac_reset_hardware_correct((uint32_t*)pcie_mapped_addr, cmac_port, verbose);
                } else {
                    printf("操作取消\n");
                }
            }
            break;
            
        case 4: /* 所有端口复位 */
            printf("即将执行所有CMAC端口硬件级复位，确认继续？(y/N): ");
            fflush(stdout);
            {
                char confirm[10];
                if (fgets(confirm, sizeof(confirm), stdin) && 
                    (confirm[0] == 'y' || confirm[0] == 'Y')) {
                    ret = smart_reset_strategy((uint32_t*)pcie_mapped_addr, -1, 1, verbose);
                } else {
                    printf("操作取消\n");
                }
            }
            break;
            
        case 5: /* 智能复位 */
            printf("🤖 智能复位策略：基于当前硬件状态选择最佳复位方案\n");
            printf("确认继续？(y/N): ");
            fflush(stdout);
            {
                char confirm[10];
                if (fgets(confirm, sizeof(confirm), stdin) && 
                    (confirm[0] == 'y' || confirm[0] == 'Y')) {
                    /* 先诊断，再根据结果选择策略 */
                    diagnose_reset_issues((uint32_t*)pcie_mapped_addr);
                    ret = smart_reset_strategy((uint32_t*)pcie_mapped_addr, 0, 1, verbose);
                } else {
                    printf("操作取消\n");
                }
            }
            break;
            
        case 6: /* 诊断 */
            ret = diagnose_reset_issues((uint32_t*)pcie_mapped_addr);
            break;
            
        case 7: /* 状态 */
            ret = display_all_reset_status((uint32_t*)pcie_mapped_addr);
            break;
            
        default:
            fprintf(stderr, "错误：未知操作类型\n");
            ret = -EINVAL;
            break;
    }
    
    /* 操作后状态检查 */
    if (ret == 0 && operation_type != 6 && operation_type != 7) {
        printf("\n=== 操作完成后的硬件状态验证 ===\n");
        diagnose_reset_issues((uint32_t*)pcie_mapped_addr);
        
        printf("=== 网络连接测试建议 ===\n");
        printf("1. 立即测试网络连接：ping <对端IP>\n");
        printf("2. 检查网络接口状态：ip link show\n");
        printf("3. 验证以太网链路：ethtool <interface_name>\n");
        printf("4. 如果ping成功 -> 问题完全解决！\n");
        printf("5. 如果仍然失败 -> 可能需要驱动重新加载\n");
    }

    cleanup_pcie_bar_mapping(pcie_mapped_addr, pcie_fd);
    
    if (verbose) {
        printf("\n程序执行完成，退出码：%d\n", ret);
    }
    
    return ret;
}