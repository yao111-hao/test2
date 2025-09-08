//==============================================================================
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//
//==============================================================================

/**
 * @file test_fixed_reset.c
 * @brief 测试修复版本的复位功能
 * 
 * 这个临时测试程序用于验证修复后的复位功能
 * 包含完整的复位后重新初始化序列
 */

#include "reset_utils.h"
#include <getopt.h>

/* 函数声明 - 来自reset_utils_fixed.c */
extern int perform_cmac_reset_fixed(uint32_t* pcie_base, int port_id, int verbose);
extern int perform_shell_reset_fixed(uint32_t* pcie_base, int verbose);
extern int perform_system_reset_fixed(uint32_t* pcie_base, int verbose);
extern int perform_cmac_reset_with_reinit(uint32_t* pcie_base, int port_id, int enable_rsfec, int verbose);

/*! 全局变量：详细输出模式标志 */
int verbose = 0;

/*! 命令行选项配置表 */
static struct option const long_opts[] = {
    {"pcie_resource", required_argument, NULL, 'p'},
    {"system", no_argument, NULL, 's'},
    {"shell", no_argument, NULL, 'S'},
    {"cmac-port", required_argument, NULL, 'c'},
    {"status", no_argument, NULL, 't'},
    {"help", no_argument, NULL, 'h'},
    {"verbose", no_argument, NULL, 'V'},
    {0, 0, 0, 0}
};

/**
 * @brief 打印程序使用说明
 * @param name 程序名称
 */
static void usage(const char *name)
{
    printf("\n=== RecoNIC修复版复位功能测试 ===\n\n");
    printf("usage: %s [OPTIONS]\n\n", name);
    printf("选项说明：\n");
    printf("  -p (--pcie_resource) PCIe资源路径\n");
    printf("  -s (--system) 执行系统完整复位（包含重新初始化）\n");
    printf("  -S (--shell) 执行Shell层完整复位（包含重新初始化）\n");
    printf("  -c (--cmac-port) 执行CMAC端口完整复位（参数：0或1）\n");
    printf("  -t (--status) 显示复位状态\n");
    printf("  -V (--verbose) 启用详细输出\n");
    printf("  -h (--help) 显示帮助信息\n");
    printf("\n使用示例：\n");
    printf("  查看状态：\n");
    printf("    sudo %s --status\n", name);
    printf("  修复版CMAC端口0复位：\n");
    printf("    sudo %s --cmac-port 0\n", name);
    printf("  修复版Shell层复位：\n");
    printf("    sudo %s --shell\n", name);
    printf("\n注意：这是临时测试版本，用于验证修复效果\n");
    printf("请先测试CMAC端口复位，确认网络连接恢复后再测试其他类型\n\n");
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
    int operation_type = 0; /* 1-system, 2-shell, 3-cmac */
    int cmac_port = -1;
    
    /* 解析命令行参数 */
    while ((cmd_opt = getopt_long(argc, argv, "p:sSc:thV", long_opts, NULL)) != -1) {
        switch (cmd_opt) {
            case 'p':
                pcie_resource = strdup(optarg);
                break;
            case 's':
                operation_type = 1;
                operation_specified = 1;
                break;
            case 'S':
                operation_type = 2;
                operation_specified = 1;
                break;
            case 'c':
                cmac_port = (int)getopt_integer(optarg);
                operation_type = 3;
                operation_specified = 1;
                break;
            case 't':
                operation_type = 4;
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
        fprintf(stderr, "请使用：sudo %s [参数]\n", argv[0]);
        return -EPERM;
    }
    
    printf("=== RecoNIC修复版复位功能测试 ===\n");
    printf("PCIe资源：%s\n", pcie_resource);
    
    /* 初始化PCIe BAR空间映射 */
    pcie_mapped_addr = init_pcie_bar_mapping(pcie_resource, &pcie_fd);
    if (!pcie_mapped_addr) {
        fprintf(stderr, "错误：PCIe BAR空间映射失败\n");
        ret = -EIO;
        goto cleanup;
    }
    
    printf("PCIe BAR空间映射成功\n\n");
    
    /* 执行操作 */
    switch (operation_type) {
        case 1: /* 系统复位 */
            printf("⚠️  警告：即将执行系统完整复位（包含重新初始化）\n");
            printf("这会重置整个RecoNIC系统并重新配置所有网络接口\n");
            printf("确认继续？(y/N): ");
            fflush(stdout);
            {
                char confirm[10];
                if (fgets(confirm, sizeof(confirm), stdin) && 
                    (confirm[0] == 'y' || confirm[0] == 'Y')) {
                    ret = perform_system_reset_fixed((uint32_t*)pcie_mapped_addr, verbose);
                } else {
                    printf("操作取消\n");
                }
            }
            break;
            
        case 2: /* Shell层复位 */
            printf("⚠️  警告：即将执行Shell层完整复位（包含重新初始化）\n");
            printf("这会重置Shell层并重新配置所有CMAC端口\n");
            printf("确认继续？(y/N): ");
            fflush(stdout);
            {
                char confirm[10];
                if (fgets(confirm, sizeof(confirm), stdin) && 
                    (confirm[0] == 'y' || confirm[0] == 'Y')) {
                    ret = perform_shell_reset_fixed((uint32_t*)pcie_mapped_addr, verbose);
                } else {
                    printf("操作取消\n");
                }
            }
            break;
            
        case 3: /* CMAC端口复位 */
            if (cmac_port != 0 && cmac_port != 1) {
                fprintf(stderr, "错误：无效的CMAC端口号 %d（应为0或1）\n", cmac_port);
                ret = -EINVAL;
                break;
            }
            printf("即将执行CMAC端口%d完整复位（包含重新初始化）\n", cmac_port);
            printf("这会复位指定端口并重新配置网络接口\n");
            printf("确认继续？(y/N): ");
            fflush(stdout);
            {
                char confirm[10];
                if (fgets(confirm, sizeof(confirm), stdin) && 
                    (confirm[0] == 'y' || confirm[0] == 'Y')) {
                    ret = perform_cmac_reset_fixed((uint32_t*)pcie_mapped_addr, cmac_port, verbose);
                } else {
                    printf("操作取消\n");
                }
            }
            break;
            
        case 4: /* 显示状态 */
            ret = display_all_reset_status((uint32_t*)pcie_mapped_addr);
            break;
            
        default:
            fprintf(stderr, "错误：未知操作类型\n");
            ret = -EINVAL;
            break;
    }
    
    if (ret == 0 && operation_type != 4) {
        printf("\n=== 操作完成后的状态检查 ===\n");
        display_all_reset_status((uint32_t*)pcie_mapped_addr);
        
        printf("\n=== 测试建议 ===\n");
        printf("1. 请测试网络连接：ping 对端设备\n");
        printf("2. 如果ping成功，说明修复版复位功能工作正常\n");
        printf("3. 如果仍然无法ping通，请提供Verilog代码进行深入分析\n");
    }

cleanup:
    cleanup_pcie_bar_mapping(pcie_mapped_addr, pcie_fd);
    
    if (verbose) {
        printf("\n程序执行完成，退出码：%d\n", ret);
    }
    
    return ret;
}