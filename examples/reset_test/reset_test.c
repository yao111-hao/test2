//==============================================================================
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//
//==============================================================================

/**
 * @file reset_test.c
 * @brief RecoNIC复位功能测试程序
 * 
 * 该程序提供了对RecoNIC智能网卡的各种复位功能。
 * 支持系统复位、Shell层复位、用户复位以及CMAC子模块复位。
 * 
 * 主要功能：
 * - 系统级别的完整复位
 * - Shell层各子模块的独立复位
 * - CMAC网络接口的复位
 * - 复位状态监控和验证
 * - 交互式复位操作界面
 * 
 * 使用示例：
 *   sudo ./reset_test --system                          # 系统复位
 *   sudo ./reset_test --shell                           # Shell层复位
 *   sudo ./reset_test --cmac-port 0                     # CMAC端口0复位
 *   sudo ./reset_test --interactive                     # 交互式模式
 *   sudo ./reset_test --status                          # 查看复位状态
 * 
 * @author RecoNIC开发团队
 * @date 2023
 */

#include "reset_utils.h"
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
    {"system", no_argument, NULL, 's'},
    {"shell", no_argument, NULL, 'S'},
    {"user", no_argument, NULL, 'u'},
    {"cmac-port", required_argument, NULL, 'c'},
    {"cmac-gt", required_argument, NULL, 'g'},
    {"status", no_argument, NULL, 't'},
    {"interactive", no_argument, NULL, 'i'},
    {"force", no_argument, NULL, 'f'},
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
    
    fprintf(stdout, "\n=== RecoNIC复位功能测试程序 ===\n\n");
    fprintf(stdout, "usage: %s [OPTIONS]\n\n", name);
    
    fprintf(stdout, "选项说明：\n");
    fprintf(stdout, "  -%c (--%s) 字符设备名称（默认：%s）\n",
            long_opts[i].val, long_opts[i].name, DEVICE_NAME_DEFAULT);
    i++;
    fprintf(stdout, "  -%c (--%s) PCIe资源路径（默认：%s）\n",
            long_opts[i].val, long_opts[i].name, PCIE_RESOURCE_DEFAULT);
    i++;
    fprintf(stdout, "  -%c (--%s) 执行系统复位\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) 执行Shell层复位\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) 执行用户逻辑复位\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) 执行CMAC端口复位（参数：0或1）\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) 执行CMAC GT端口复位（参数：0或1）\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) 仅显示复位状态，不执行复位\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) 进入交互式模式\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) 强制复位，跳过确认提示\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) 显示帮助信息并退出\n",
            long_opts[i].val, long_opts[i].name);
    i++;
    fprintf(stdout, "  -%c (--%s) 启用详细输出模式\n",
            long_opts[i].val, long_opts[i].name);
    
    fprintf(stdout, "\n使用示例：\n");
    fprintf(stdout, "  系统复位：\n");
    fprintf(stdout, "    sudo %s --system\n", name);
    fprintf(stdout, "  Shell层复位：\n");
    fprintf(stdout, "    sudo %s --shell\n", name);
    fprintf(stdout, "  CMAC端口0复位：\n");
    fprintf(stdout, "    sudo %s --cmac-port 0\n", name);
    fprintf(stdout, "  强制执行用户复位：\n");
    fprintf(stdout, "    sudo %s --user --force\n", name);
    fprintf(stdout, "  查看复位状态：\n");
    fprintf(stdout, "    sudo %s --status\n", name);
    fprintf(stdout, "  交互式模式：\n");
    fprintf(stdout, "    sudo %s --interactive\n", name);
    
    fprintf(stdout, "\n复位类型说明：\n");
    fprintf(stdout, "  系统复位   - 完整的系统复位，影响整个RecoNIC系统\n");
    fprintf(stdout, "  Shell层复位 - 重置网卡Shell层各个子模块\n");
    fprintf(stdout, "  用户复位   - 重置用户可编程逻辑部分\n");
    fprintf(stdout, "  CMAC复位   - 重置指定的100G以太网端口\n");
    fprintf(stdout, "  GT复位     - 重置指定端口的GT收发器\n");
    
    fprintf(stdout, "\n安全注意事项：\n");
    fprintf(stdout, "  - 本程序需要root权限运行\n");
    fprintf(stdout, "  - 复位操作可能导致网络连接中断\n");
    fprintf(stdout, "  - 系统复位会影响整个RecoNIC系统\n");
    fprintf(stdout, "  - 请在维护窗口期间执行复位操作\n");
    fprintf(stdout, "  - 建议先使用--status查看当前状态\n");
    fprintf(stdout, "\n");
}

/**
 * @brief 执行指定类型的复位操作
 * @param config 复位配置
 * @param pcie_base PCIe基地址
 * @return 0-成功，负数-失败
 */
static int perform_reset_operation(reset_config_t* config, uint32_t* pcie_base)
{
    int ret = 0;
    
    /* 如果不是强制模式，需要用户确认 */
    if (!config->force_reset && !config->status_only) {
        if (!confirm_reset_operation(config->reset_type)) {
            printf("操作已取消\n");
            return 0;
        }
    }
    
    if (config->verbose_mode) {
        printf("执行复位操作：%s\n", get_reset_type_name(config->reset_type));
    }
    
    switch (config->reset_type) {
        case RESET_TYPE_SYSTEM:
            ret = perform_system_reset(pcie_base, config->verbose_mode);
            break;
            
        case RESET_TYPE_SHELL:
            ret = perform_shell_reset(pcie_base, config->verbose_mode);
            break;
            
        case RESET_TYPE_USER:
            ret = perform_user_reset(pcie_base, config->verbose_mode);
            break;
            
        case RESET_TYPE_CMAC_PORT0:
            ret = perform_cmac_reset(pcie_base, 0, config->verbose_mode);
            break;
            
        case RESET_TYPE_CMAC_PORT1:
            ret = perform_cmac_reset(pcie_base, 1, config->verbose_mode);
            break;
            
        case RESET_TYPE_CMAC_GT_PORT0:
            ret = perform_cmac_gt_reset(pcie_base, 0, config->verbose_mode);
            break;
            
        case RESET_TYPE_CMAC_GT_PORT1:
            ret = perform_cmac_gt_reset(pcie_base, 1, config->verbose_mode);
            break;
            
        default:
            fprintf(stderr, "错误：未知的复位类型 %d\n", config->reset_type);
            return -EINVAL;
    }
    
    if (ret != 0) {
        fprintf(stderr, "错误：%s执行失败\n", get_reset_type_name(config->reset_type));
    } else {
        printf("✓ %s执行成功\n", get_reset_type_name(config->reset_type));
    }
    
    return ret;
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
    reset_config_t config = {0};
    void* pcie_mapped_addr = NULL;
    int pcie_fd = -1;
    int ret = 0;
    int operation_specified = 0;
    int cmac_port = -1;
    
    /* 初始化配置默认值 */
    config.device_name = strdup(DEVICE_NAME_DEFAULT);
    config.pcie_resource = strdup(PCIE_RESOURCE_DEFAULT);
    config.reset_type = RESET_TYPE_SYSTEM;
    config.force_reset = 0;
    config.verbose_mode = 0;
    config.interactive_mode = 0;
    config.status_only = 0;
    
    /* 解析命令行参数 */
    while ((cmd_opt = getopt_long(argc, argv, "d:p:sSu:c:g:tifhV", long_opts, NULL)) != -1) {
        switch (cmd_opt) {
            case 'd':
                free(config.device_name);
                config.device_name = strdup(optarg);
                break;
                
            case 'p':
                free(config.pcie_resource);
                config.pcie_resource = strdup(optarg);
                break;
                
            case 's':
                config.reset_type = RESET_TYPE_SYSTEM;
                operation_specified = 1;
                break;
                
            case 'S':
                config.reset_type = RESET_TYPE_SHELL;
                operation_specified = 1;
                break;
                
            case 'u':
                config.reset_type = RESET_TYPE_USER;
                operation_specified = 1;
                break;
                
            case 'c':
                cmac_port = (int)getopt_integer(optarg);
                if (cmac_port == 0) {
                    config.reset_type = RESET_TYPE_CMAC_PORT0;
                } else if (cmac_port == 1) {
                    config.reset_type = RESET_TYPE_CMAC_PORT1;
                } else {
                    fprintf(stderr, "错误：无效的CMAC端口号 %d（应为0或1）\n", cmac_port);
                    ret = -EINVAL;
                    goto cleanup;
                }
                operation_specified = 1;
                break;
                
            case 'g':
                cmac_port = (int)getopt_integer(optarg);
                if (cmac_port == 0) {
                    config.reset_type = RESET_TYPE_CMAC_GT_PORT0;
                } else if (cmac_port == 1) {
                    config.reset_type = RESET_TYPE_CMAC_GT_PORT1;
                } else {
                    fprintf(stderr, "错误：无效的CMAC GT端口号 %d（应为0或1）\n", cmac_port);
                    ret = -EINVAL;
                    goto cleanup;
                }
                operation_specified = 1;
                break;
                
            case 't':
                config.status_only = 1;
                operation_specified = 1;
                break;
                
            case 'i':
                config.interactive_mode = 1;
                operation_specified = 1;
                break;
                
            case 'f':
                config.force_reset = 1;
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
        fprintf(stderr, "错误：请指定操作类型\n");
        usage(argv[0]);
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
        if (config.interactive_mode) {
            printf("运行模式：交互式模式\n");
        } else if (config.status_only) {
            printf("运行模式：状态查看模式\n");
        } else {
            printf("运行模式：单次操作模式\n");
            printf("复位类型：%s\n", get_reset_type_name(config.reset_type));
            printf("强制模式：%s\n", config.force_reset ? "是" : "否");
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
        ret = interactive_reset_mode((uint32_t*)pcie_mapped_addr);
    } else if (config.status_only) {
        /* 仅显示状态 */
        ret = display_all_reset_status((uint32_t*)pcie_mapped_addr);
    } else {
        /* 单次操作模式 */
        ret = perform_reset_operation(&config, (uint32_t*)pcie_mapped_addr);
        
        /* 执行复位后显示状态 */
        if (ret == 0 && !config.status_only) {
            printf("\n复位后状态：\n");
            display_all_reset_status((uint32_t*)pcie_mapped_addr);
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
