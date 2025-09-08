//==============================================================================
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//
//==============================================================================

/**
 * @file register_test.c
 * @brief RecoNIC寄存器测试程序
 * 
 * 该程序提供了对RecoNIC智能网卡寄存器的读写测试功能。
 * 用户可以通过BAR空间地址直接访问网卡中的寄存器。
 * 
 * 主要功能：
 * - 命令行方式进行单次寄存器读写操作
 * - 交互式模式进行连续的寄存器访问
 * - 安全检查和错误处理
 * - 支持常见寄存器的名称显示
 * 
 * 使用示例：
 *   sudo ./register_test -r 0x102000                    # 读取版本寄存器
 *   sudo ./register_test -w 0x103000 0x12345678         # 写入计算控制寄存器
 *   sudo ./register_test -i                             # 进入交互模式
 * 
 * @author RecoNIC开发团队
 * @date 2023
 */

#include "register_utils.h"
#include "../../lib/reconic.h"
#include "../../lib/control_api.h"

/*! \def DEVICE_NAME_DEFAULT
 *  \brief 默认字符设备名称
 */
#define DEVICE_NAME_DEFAULT DEFAULT_DEVICE

/*! \def PCIE_RESOURCE_DEFAULT
 *  \brief 默认PCIe资源路径
 */
#define PCIE_RESOURCE_DEFAULT DEFAULT_PCIE_RESOURCE

/*! 全局变量：详细输出模式标志 */
int verbose = 0;

/*! 命令行选项配置表 */
static struct option const long_opts[] = {
    {"device", required_argument, NULL, 'd'},
    {"pcie_resource", required_argument, NULL, 'p'},
    {"read", required_argument, NULL, 'r'},
    {"write", required_argument, NULL, 'w'},
    {"value", required_argument, NULL, 'v'},
    {"interactive", no_argument, NULL, 'i'},
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
    int i = 0;
    
    fprintf(stdout, "\n=== RecoNIC寄存器测试程序 ===\n\n");
    fprintf(stdout, "usage: %s [OPTIONS]\n\n", name);
    
    fprintf(stdout, "选项说明：\n");
    fprintf(stdout, "  -%c (--%s) 字符设备名称（默认：%s）\n",
            long_opts[i].val, long_opts[i].name, DEVICE_NAME_DEFAULT);
    i++;
    fprintf(stdout, "  -%c (--%s) PCIe资源路径（默认：%s）\n",
            long_opts[i].val, long_opts[i].name, PCIE_RESOURCE_DEFAULT);
    i++;
    fprintf(stdout, "  -%c (--%s) 读取寄存器，指定偏移地址（十进制或0x十六进制）\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) 写入寄存器，需配合-v参数指定值\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) 指定要写入的寄存器值（十进制或0x十六进制）\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) 进入交互式模式\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) 显示帮助信息并退出\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) 启用详细输出模式\n",
            long_opts[i].val, long_opts[i].name);
    
    fprintf(stdout, "\n使用示例：\n");
    fprintf(stdout, "  读取版本寄存器：\n");
    fprintf(stdout, "    sudo %s -r 0x102000\n", name);
    fprintf(stdout, "  写入计算控制寄存器：\n");
    fprintf(stdout, "    sudo %s -w 0x103000 -v 0x12345678\n", name);
    fprintf(stdout, "  使用自定义PCIe路径读取寄存器：\n");
    fprintf(stdout, "    sudo %s -p /sys/bus/pci/devices/0000:01:00.0/resource2 -r 0x102000\n", name);
    fprintf(stdout, "  进入交互模式：\n");
    fprintf(stdout, "    sudo %s -i\n", name);
    
    fprintf(stdout, "\n注意事项：\n");
    fprintf(stdout, "  - 本程序需要root权限运行\n");
    fprintf(stdout, "  - 寄存器地址必须4字节对齐\n");
    fprintf(stdout, "  - 某些寄存器为只读，写入可能无效\n");
    fprintf(stdout, "  - 不当的寄存器操作可能导致系统不稳定\n");
    fprintf(stdout, "\n");
}

/**
 * @brief 执行单次寄存器读操作
 * @param config 寄存器访问配置
 * @param pcie_axil_base PCIe AXIL基地址
 * @return 0-成功，负数-失败
 */
static int perform_register_read(register_access_t* config, uint32_t* pcie_axil_base)
{
    uint32_t value;
    int ret;
    
    if (config->verbose_mode) {
        printf("执行寄存器读操作：偏移=0x%06X\n", config->register_offset);
    }
    
    ret = safe_read32_data(pcie_axil_base, config->register_offset, &value);
    if (ret != 0) {
        fprintf(stderr, "错误：寄存器读取失败\n");
        return ret;
    }
    
    printf("=== 寄存器读取结果 ===\n");
    printf("寄存器偏移：0x%06X\n", config->register_offset);
    printf("寄存器名称：%s\n", get_register_name(config->register_offset));
    printf("寄存器值：  0x%08X (%u)\n", value, value);
    printf("二进制值：  ");
    for (int i = 31; i >= 0; i--) {
        printf("%d", (value >> i) & 1);
        if (i % 4 == 0 && i > 0) printf(" ");
    }
    printf("\n");
    
    return 0;
}

/**
 * @brief 执行单次寄存器写操作
 * @param config 寄存器访问配置
 * @param pcie_axil_base PCIe AXIL基地址
 * @return 0-成功，负数-失败
 */
static int perform_register_write(register_access_t* config, uint32_t* pcie_axil_base)
{
    uint32_t read_back_value;
    int ret;
    
    if (config->verbose_mode) {
        printf("执行寄存器写操作：偏移=0x%06X，值=0x%08X\n", 
               config->register_offset, config->register_value);
    }
    
    printf("=== 寄存器写入操作 ===\n");
    printf("寄存器偏移：0x%06X\n", config->register_offset);
    printf("寄存器名称：%s\n", get_register_name(config->register_offset));
    printf("写入值：    0x%08X (%u)\n", config->register_value, config->register_value);
    
    /* 先读取当前值 */
    ret = safe_read32_data(pcie_axil_base, config->register_offset, &read_back_value);
    if (ret == 0) {
        printf("写入前值：  0x%08X (%u)\n", read_back_value, read_back_value);
    }
    
    /* 执行写操作 */
    ret = safe_write32_data(pcie_axil_base, config->register_offset, config->register_value);
    if (ret != 0) {
        fprintf(stderr, "错误：寄存器写入失败\n");
        return ret;
    }
    
    printf("寄存器写入成功\n");
    
    /* 读回验证 */
    ret = safe_read32_data(pcie_axil_base, config->register_offset, &read_back_value);
    if (ret == 0) {
        printf("写入后值：  0x%08X (%u)\n", read_back_value, read_back_value);
        
        if (read_back_value == config->register_value) {
            printf("✓ 验证成功：读回值与写入值一致\n");
        } else {
            printf("⚠ 警告：读回值与写入值不一致（可能是只读寄存器）\n");
        }
    } else {
        printf("⚠ 警告：无法读回验证写入结果\n");
    }
    
    return 0;
}

/**
 * @brief 主函数
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 0-成功，非0-失败
 */
int main(int argc, char *argv[])
{
    int cmd_opt;
    register_access_t config = {0};
    void* pcie_mapped_addr = NULL;
    int pcie_fd = -1;
    int ret = 0;
    int operation_specified = 0;
    
    /* 初始化配置默认值 */
    config.device_name = strdup(DEVICE_NAME_DEFAULT);
    config.pcie_resource = strdup(PCIE_RESOURCE_DEFAULT);
    config.register_offset = 0;
    config.register_value = 0;
    config.is_write_operation = 0;
    config.verbose_mode = 0;
    config.interactive_mode = 0;
    
    /* 解析命令行参数 */
    while ((cmd_opt = getopt_long(argc, argv, "d:p:r:w:v:ihV", long_opts, NULL)) != -1) {
        switch (cmd_opt) {
            case 'd':
                free(config.device_name);
                config.device_name = strdup(optarg);
                break;
                
            case 'p':
                free(config.pcie_resource);
                config.pcie_resource = strdup(optarg);
                break;
                
            case 'r':
                config.register_offset = (uint32_t)getopt_integer(optarg);
                config.is_write_operation = 0;
                operation_specified = 1;
                break;
                
            case 'w':
                config.register_offset = (uint32_t)getopt_integer(optarg);
                config.is_write_operation = 1;
                operation_specified = 1;
                break;
                
            case 'v':
                config.register_value = (uint32_t)getopt_integer(optarg);
                break;
                
            case 'i':
                config.interactive_mode = 1;
                operation_specified = 1;
                break;
                
            case 'V':
                config.verbose_mode = 1;
                verbose = 1;
                break;
                
            case 'h':
                usage(argv[0]);
                ret = 0;
                goto cleanup;
                
            default:
                usage(argv[0]);
                ret = -EINVAL;
                goto cleanup;
        }
    }
    
    /* 检查参数有效性 */
    if (!operation_specified) {
        fprintf(stderr, "错误：请指定操作类型（-r、-w或-i）\n");
        usage(argv[0]);
        ret = -EINVAL;
        goto cleanup;
    }
    
    if (config.is_write_operation && config.register_value == 0 && optind == argc) {
        fprintf(stderr, "错误：写操作需要指定寄存器值（-v参数）\n");
        ret = -EINVAL;
        goto cleanup;
    }
    
    /* 检查权限 */
    if (geteuid() != 0) {
        fprintf(stderr, "错误：本程序需要root权限运行\n");
        fprintf(stderr, "请使用：sudo %s [参数]\n", argv[0]);
        ret = -EPERM;
        goto cleanup;
    }
    
    if (config.verbose_mode) {
        printf("=== 配置信息 ===\n");
        printf("字符设备：%s\n", config.device_name);
        printf("PCIe资源：%s\n", config.pcie_resource);
        if (!config.interactive_mode) {
            printf("寄存器偏移：0x%06X\n", config.register_offset);
            printf("操作类型：%s\n", config.is_write_operation ? "写入" : "读取");
            if (config.is_write_operation) {
                printf("写入值：0x%08X\n", config.register_value);
            }
        }
        printf("==================\n\n");
    }
    
    /* 初始化PCIe BAR空间映射 */
    pcie_mapped_addr = init_pcie_bar_mapping(config.pcie_resource, &pcie_fd);
    if (!pcie_mapped_addr) {
        fprintf(stderr, "错误：PCIe BAR空间映射失败\n");
        ret = -EIO;
        goto cleanup;
    }
    
    if (config.verbose_mode) {
        printf("PCIe BAR空间初始化完成\n\n");
    }
    
    /* 执行操作 */
    if (config.interactive_mode) {
        /* 交互式模式 */
        ret = interactive_register_access((uint32_t*)pcie_mapped_addr);
    } else {
        /* 单次操作模式 */
        if (config.is_write_operation) {
            ret = perform_register_write(&config, (uint32_t*)pcie_mapped_addr);
        } else {
            ret = perform_register_read(&config, (uint32_t*)pcie_mapped_addr);
        }
    }

cleanup:
    /* 清理资源 */
    cleanup_pcie_bar_mapping(pcie_mapped_addr, pcie_fd);
    
    if (config.device_name) {
        free(config.device_name);
    }
    if (config.pcie_resource) {
        free(config.pcie_resource);
    }
    
    if (config.verbose_mode) {
        printf("\n程序执行完成，退出码：%d\n", ret);
    }
    
    return ret;
}