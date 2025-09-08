//==============================================================================
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//
//==============================================================================

#include "reset_utils.h"

/**
 * @brief 解析命令行整数参数（支持十进制和十六进制）
 * @param optarg 命令行参数字符串
 * @return 解析后的64位整数值
 */
uint64_t getopt_integer(char *optarg)
{
    int rc;
    uint64_t value;

    rc = sscanf(optarg, "0x%lx", &value);
    if (rc <= 0)
        sscanf(optarg, "%lu", &value);

    return value;
}

/**
 * @brief 初始化PCIe BAR空间映射
 * @param pcie_resource PCIe资源文件路径
 * @param pcie_fd PCIe文件描述符指针（输出参数）
 * @return 映射的虚拟地址，失败返回NULL
 */
void* init_pcie_bar_mapping(const char* pcie_resource, int* pcie_fd)
{
    void* mapped_addr = NULL;
    
    if (!pcie_resource || !pcie_fd) {
        fprintf(stderr, "错误：参数不能为空\n");
        return NULL;
    }

    /* 打开PCIe资源文件 */
    *pcie_fd = open(pcie_resource, O_RDWR | O_SYNC);
    if (*pcie_fd < 0) {
        fprintf(stderr, "错误：无法打开PCIe资源文件 %s\n", pcie_resource);
        perror("open");
        return NULL;
    }

    /* 映射PCIe BAR空间到用户空间 */
    mapped_addr = mmap(NULL, REG_MAP_SIZE, PROT_READ | PROT_WRITE, 
                       MAP_SHARED, *pcie_fd, 0);
    if (mapped_addr == MAP_FAILED) {
        fprintf(stderr, "错误：无法映射PCIe BAR空间\n");
        perror("mmap");
        close(*pcie_fd);
        *pcie_fd = -1;
        return NULL;
    }

    printf("PCIe BAR空间映射成功：%s -> %p\n", pcie_resource, mapped_addr);
    return mapped_addr;
}

/**
 * @brief 清理PCIe BAR空间映射
 * @param mapped_addr 映射的虚拟地址
 * @param pcie_fd PCIe文件描述符
 */
void cleanup_pcie_bar_mapping(void* mapped_addr, int pcie_fd)
{
    if (mapped_addr && mapped_addr != MAP_FAILED) {
        munmap(mapped_addr, REG_MAP_SIZE);
        printf("PCIe BAR空间解除映射完成\n");
    }
    
    if (pcie_fd >= 0) {
        close(pcie_fd);
        printf("PCIe资源文件已关闭\n");
    }
}

/**
 * @brief 安全的32位寄存器写操作
 * @param pcie_base PCIe基地址
 * @param offset 寄存器偏移地址
 * @param value 要写入的值
 * @return 0-成功，负数-失败
 */
int safe_write32_data(uint32_t* pcie_base, off_t offset, uint32_t value)
{
    uint32_t* reg_addr;

    if (!pcie_base) {
        fprintf(stderr, "错误：PCIe基地址为空\n");
        return -EINVAL;
    }

    if (offset >= REG_MAP_SIZE || (offset & 0x3)) {
        fprintf(stderr, "错误：寄存器偏移地址0x%lx无效\n", offset);
        return -EINVAL;
    }

    reg_addr = (uint32_t*)((uintptr_t)pcie_base + offset);
    *(reg_addr) = value;
    
    /* 强制刷新写缓存 */
    __sync_synchronize();
    
    return 0;
}

/**
 * @brief 安全的32位寄存器读操作
 * @param pcie_base PCIe基地址
 * @param offset 寄存器偏移地址
 * @param value 读取值的存储指针（输出参数）
 * @return 0-成功，负数-失败
 */
int safe_read32_data(uint32_t* pcie_base, off_t offset, uint32_t* value)
{
    uint32_t* reg_addr;

    if (!pcie_base || !value) {
        fprintf(stderr, "错误：参数不能为空\n");
        return -EINVAL;
    }

    if (offset >= REG_MAP_SIZE || (offset & 0x3)) {
        fprintf(stderr, "错误：寄存器偏移地址0x%lx无效\n", offset);
        return -EINVAL;
    }

    reg_addr = (uint32_t*)((uintptr_t)pcie_base + offset);
    *value = *((uint32_t*)reg_addr);
    
    return 0;
}

/**
 * @brief 获取复位类型名称
 * @param reset_type 复位类型
 * @return 复位类型名称字符串
 */
const char* get_reset_type_name(reset_type_t reset_type)
{
    switch (reset_type) {
        case RESET_TYPE_SYSTEM:         return "系统复位";
        case RESET_TYPE_SHELL:          return "Shell层复位";
        case RESET_TYPE_USER:           return "用户复位";
        case RESET_TYPE_CMAC_PORT0:     return "CMAC端口0复位";
        case RESET_TYPE_CMAC_PORT1:     return "CMAC端口1复位";
        case RESET_TYPE_CMAC_GT_PORT0:  return "CMAC GT端口0复位";
        case RESET_TYPE_CMAC_GT_PORT1:  return "CMAC GT端口1复位";
        default:                        return "未知复位类型";
    }
}

/**
 * @brief 获取复位类型描述
 * @param reset_type 复位类型
 * @return 复位类型描述字符串
 */
const char* get_reset_type_description(reset_type_t reset_type)
{
    switch (reset_type) {
        case RESET_TYPE_SYSTEM:
            return "完整的系统复位，将重置整个RecoNIC系统";
        case RESET_TYPE_SHELL:
            return "Shell层复位，重置网卡Shell层各个子模块";
        case RESET_TYPE_USER:
            return "用户逻辑复位，重置用户可编程逻辑部分";
        case RESET_TYPE_CMAC_PORT0:
            return "CMAC端口0复位，重置第一个100G以太网端口";
        case RESET_TYPE_CMAC_PORT1:
            return "CMAC端口1复位，重置第二个100G以太网端口";
        case RESET_TYPE_CMAC_GT_PORT0:
            return "CMAC GT端口0复位，重置第一个端口的GT收发器";
        case RESET_TYPE_CMAC_GT_PORT1:
            return "CMAC GT端口1复位，重置第二个端口的GT收发器";
        default:
            return "未知的复位类型";
    }
}

/**
 * @brief 执行系统复位
 * @param pcie_base PCIe基地址
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_system_reset(uint32_t* pcie_base, int verbose)
{
    int ret;
    
    if (verbose) {
        printf("执行系统复位...\n");
    }
    
    /* 写入系统复位命令 */
    ret = safe_write32_data(pcie_base, SYSCFG_OFFSET_SYSTEM_RESET, 0x1);
    if (ret != 0) {
        fprintf(stderr, "错误：系统复位命令写入失败\n");
        return ret;
    }
    
    if (verbose) {
        printf("系统复位命令已发送，等待复位完成...\n");
    }
    
    /* 等待复位完成 */
    ret = wait_reset_completion(pcie_base, RESET_TYPE_SYSTEM, RESET_TIMEOUT_MS);
    if (ret != 0) {
        fprintf(stderr, "错误：系统复位超时或失败\n");
        return ret;
    }
    
    printf("✓ 系统复位完成\n");
    return 0;
}

/**
 * @brief 执行Shell层复位
 * @param pcie_base PCIe基地址
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_shell_reset(uint32_t* pcie_base, int verbose)
{
    int ret;
    
    if (verbose) {
        printf("执行Shell层复位...\n");
    }
    
    /* 写入Shell层复位命令（复位所有Shell子模块）*/
    ret = safe_write32_data(pcie_base, SYSCFG_OFFSET_SHELL_RESET, 0x110);
    if (ret != 0) {
        fprintf(stderr, "错误：Shell层复位命令写入失败\n");
        return ret;
    }
    
    if (verbose) {
        printf("Shell层复位命令已发送，等待复位完成...\n");
    }
    
    /* 等待复位完成 */
    ret = wait_reset_completion(pcie_base, RESET_TYPE_SHELL, RESET_TIMEOUT_MS);
    if (ret != 0) {
        fprintf(stderr, "错误：Shell层复位超时或失败\n");
        return ret;
    }
    
    printf("✓ Shell层复位完成\n");
    return 0;
}

/**
 * @brief 执行用户复位
 * @param pcie_base PCIe基地址
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_user_reset(uint32_t* pcie_base, int verbose)
{
    int ret;
    
    if (verbose) {
        printf("执行用户逻辑复位...\n");
    }
    
    /* 写入用户复位命令 */
    ret = safe_write32_data(pcie_base, SYSCFG_OFFSET_USER_RESET, 0x1);
    if (ret != 0) {
        fprintf(stderr, "错误：用户复位命令写入失败\n");
        return ret;
    }
    
    if (verbose) {
        printf("用户复位命令已发送，等待复位完成...\n");
    }
    
    /* 等待复位完成 */
    ret = wait_reset_completion(pcie_base, RESET_TYPE_USER, RESET_TIMEOUT_MS);
    if (ret != 0) {
        fprintf(stderr, "错误：用户复位超时或失败\n");
        return ret;
    }
    
    printf("✓ 用户逻辑复位完成\n");
    return 0;
}

/**
 * @brief 执行CMAC端口复位
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID（0或1）
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_cmac_reset(uint32_t* pcie_base, int port_id, int verbose)
{
    int ret;
    uint32_t reset_mask;
    
    if (port_id != 0 && port_id != 1) {
        fprintf(stderr, "错误：无效的CMAC端口ID %d（应为0或1）\n", port_id);
        return -EINVAL;
    }
    
    if (verbose) {
        printf("执行CMAC端口%d复位...\n", port_id);
    }
    
    /* 设置相应端口的复位掩码 */
    reset_mask = (port_id == 0) ? SHELL_RESET_CMAC_PORT0 : SHELL_RESET_CMAC_PORT1;
    
    /* 写入CMAC复位命令 */
    ret = safe_write32_data(pcie_base, SYSCFG_OFFSET_SHELL_RESET, reset_mask);
    if (ret != 0) {
        fprintf(stderr, "错误：CMAC端口%d复位命令写入失败\n", port_id);
        return ret;
    }
    
    if (verbose) {
        printf("CMAC端口%d复位命令已发送，等待复位完成...\n", port_id);
    }
    
    /* 等待复位完成 */
    ret = wait_reset_completion(pcie_base, 
                               (port_id == 0) ? RESET_TYPE_CMAC_PORT0 : RESET_TYPE_CMAC_PORT1, 
                               RESET_TIMEOUT_MS);
    if (ret != 0) {
        fprintf(stderr, "错误：CMAC端口%d复位超时或失败\n", port_id);
        return ret;
    }
    
    printf("✓ CMAC端口%d复位完成\n", port_id);
    return 0;
}

/**
 * @brief 执行CMAC GT复位
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID（0或1）
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_cmac_gt_reset(uint32_t* pcie_base, int port_id, int verbose)
{
    int ret;
    
    if (port_id != 0 && port_id != 1) {
        fprintf(stderr, "错误：无效的CMAC GT端口ID %d（应为0或1）\n", port_id);
        return -EINVAL;
    }
    
    if (verbose) {
        printf("执行CMAC GT端口%d复位...\n", port_id);
    }
    
    /* 写入CMAC GT复位命令 */
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_GT_RESET(port_id), 0x1);
    if (ret != 0) {
        fprintf(stderr, "错误：CMAC GT端口%d复位命令写入失败\n", port_id);
        return ret;
    }
    
    if (verbose) {
        printf("CMAC GT端口%d复位命令已发送，等待复位完成...\n", port_id);
    }
    
    /* GT复位通常很快，等待一段固定时间 */
    usleep(100000); /* 等待100ms */
    
    printf("✓ CMAC GT端口%d复位完成\n", port_id);
    return 0;
}

/**
 * @brief 检查系统复位状态
 * @param pcie_base PCIe基地址
 * @param reset_completed 复位完成标志指针（输出参数）
 * @return 0-成功，负数-失败
 */
int check_system_reset_status(uint32_t* pcie_base, int* reset_completed)
{
    uint32_t status_value;
    int ret;
    
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_SYSTEM_STATUS, &status_value);
    if (ret != 0) {
        return ret;
    }
    
    /* 系统复位完成状态检查：位0为1表示复位完成 */
    *reset_completed = (status_value & 0x1) ? 1 : 0;
    return 0;
}

/**
 * @brief 检查Shell层复位状态
 * @param pcie_base PCIe基地址
 * @param reset_completed 复位完成标志指针（输出参数）
 * @return 0-成功，负数-失败
 */
int check_shell_reset_status(uint32_t* pcie_base, int* reset_completed)
{
    uint32_t status_value;
    int ret;
    
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_SHELL_STATUS, &status_value);
    if (ret != 0) {
        return ret;
    }
    
    /* Shell复位完成状态检查：相应位为1表示复位完成 */
    *reset_completed = (status_value & 0x110) ? 1 : 0;
    return 0;
}

/**
 * @brief 检查用户复位状态
 * @param pcie_base PCIe基地址
 * @param reset_completed 复位完成标志指针（输出参数）
 * @return 0-成功，负数-失败
 */
int check_user_reset_status(uint32_t* pcie_base, int* reset_completed)
{
    uint32_t status_value;
    int ret;
    
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_USER_STATUS, &status_value);
    if (ret != 0) {
        return ret;
    }
    
    /* 用户复位完成状态检查：位0为1表示复位完成 */
    *reset_completed = (status_value & 0x1) ? 1 : 0;
    return 0;
}

/**
 * @brief 检查CMAC复位状态
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID（0或1）
 * @param reset_completed 复位完成标志指针（输出参数）
 * @return 0-成功，负数-失败
 */
int check_cmac_reset_status(uint32_t* pcie_base, int port_id, int* reset_completed)
{
    uint32_t status_value;
    uint32_t expected_mask;
    int ret;
    
    if (port_id != 0 && port_id != 1) {
        return -EINVAL;
    }
    
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_SHELL_STATUS, &status_value);
    if (ret != 0) {
        return ret;
    }
    
    /* CMAC复位完成状态检查 */
    expected_mask = (port_id == 0) ? SHELL_RESET_CMAC_PORT0 : SHELL_RESET_CMAC_PORT1;
    *reset_completed = (status_value & expected_mask) ? 1 : 0;
    return 0;
}

/**
 * @brief 显示所有复位状态
 * @param pcie_base PCIe基地址
 * @return 0-成功，负数-失败
 */
int display_all_reset_status(uint32_t* pcie_base)
{
    uint32_t system_status, shell_status, user_status;
    int ret;
    
    printf("\n=== RecoNIC系统复位状态 ===\n");
    
    /* 读取系统状态 */
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_SYSTEM_STATUS, &system_status);
    if (ret == 0) {
        printf("系统状态寄存器：     0x%08X\n", system_status);
        printf("  系统复位完成：     %s\n", (system_status & 0x1) ? "是" : "否");
    } else {
        printf("系统状态寄存器：     读取失败\n");
    }
    
    /* 读取Shell状态 */
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_SHELL_STATUS, &shell_status);
    if (ret == 0) {
        printf("Shell状态寄存器：    0x%08X\n", shell_status);
        printf("  CMAC端口0复位完成：%s\n", (shell_status & 0x10) ? "是" : "否");
        printf("  CMAC端口1复位完成：%s\n", (shell_status & 0x100) ? "是" : "否");
    } else {
        printf("Shell状态寄存器：    读取失败\n");
    }
    
    /* 读取用户状态 */
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_USER_STATUS, &user_status);
    if (ret == 0) {
        printf("用户状态寄存器：     0x%08X\n", user_status);
        printf("  用户复位完成：     %s\n", (user_status & 0x1) ? "是" : "否");
    } else {
        printf("用户状态寄存器：     读取失败\n");
    }
    
    printf("===========================\n\n");
    return 0;
}

/**
 * @brief 等待复位完成
 * @param pcie_base PCIe基地址
 * @param reset_type 复位类型
 * @param timeout_ms 超时时间（毫秒）
 * @return 0-成功，负数-失败
 */
int wait_reset_completion(uint32_t* pcie_base, reset_type_t reset_type, int timeout_ms)
{
    int elapsed_ms = 0;
    int reset_completed = 0;
    int ret;
    
    while (elapsed_ms < timeout_ms) {
        /* 根据复位类型检查相应的状态 */
        switch (reset_type) {
            case RESET_TYPE_SYSTEM:
                ret = check_system_reset_status(pcie_base, &reset_completed);
                break;
            case RESET_TYPE_SHELL:
                ret = check_shell_reset_status(pcie_base, &reset_completed);
                break;
            case RESET_TYPE_USER:
                ret = check_user_reset_status(pcie_base, &reset_completed);
                break;
            case RESET_TYPE_CMAC_PORT0:
                ret = check_cmac_reset_status(pcie_base, 0, &reset_completed);
                break;
            case RESET_TYPE_CMAC_PORT1:
                ret = check_cmac_reset_status(pcie_base, 1, &reset_completed);
                break;
            case RESET_TYPE_CMAC_GT_PORT0:
            case RESET_TYPE_CMAC_GT_PORT1:
                /* GT复位没有状态寄存器，直接返回成功 */
                return 0;
            default:
                return -EINVAL;
        }
        
        if (ret != 0) {
            fprintf(stderr, "错误：检查复位状态失败\n");
            return ret;
        }
        
        if (reset_completed) {
            return 0; /* 复位完成 */
        }
        
        /* 等待一段时间后再检查 */
        usleep(RESET_POLL_INTERVAL_MS * 1000);
        elapsed_ms += RESET_POLL_INTERVAL_MS;
    }
    
    /* 超时 */
    return -ETIMEDOUT;
}

/**
 * @brief 确认复位操作
 * @param reset_type 复位类型
 * @return 1-确认，0-取消
 */
int confirm_reset_operation(reset_type_t reset_type)
{
    char input[10];
    
    printf("\n⚠️  警告：您即将执行 %s\n", get_reset_type_name(reset_type));
    printf("描述：%s\n", get_reset_type_description(reset_type));
    printf("\n这个操作可能会影响系统稳定性和网络连接。\n");
    printf("确认要继续吗？(y/N): ");
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin)) {
        return (input[0] == 'y' || input[0] == 'Y') ? 1 : 0;
    }
    
    return 0; /* 默认取消 */
}

/**
 * @brief 交互式复位模式
 * @param pcie_base PCIe基地址
 * @return 0-正常退出，负数-错误退出
 */
int interactive_reset_mode(uint32_t* pcie_base)
{
    char input[256];
    int choice, ret;
    
    printf("\n=== 进入交互式复位模式 ===\n");
    
    while (1) {
        printf("\n可用的复位操作：\n");
        printf("1. 系统复位\n");
        printf("2. Shell层复位\n");
        printf("3. 用户复位\n");
        printf("4. CMAC端口0复位\n");
        printf("5. CMAC端口1复位\n");
        printf("6. CMAC GT端口0复位\n");
        printf("7. CMAC GT端口1复位\n");
        printf("8. 显示所有复位状态\n");
        printf("0. 退出交互模式\n\n");
        printf("请选择操作 (0-8): ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break;
        }
        
        choice = atoi(input);
        
        switch (choice) {
            case 1:
                if (confirm_reset_operation(RESET_TYPE_SYSTEM)) {
                    ret = perform_system_reset(pcie_base, 1);
                    if (ret != 0) {
                        printf("系统复位失败\n");
                    }
                } else {
                    printf("操作已取消\n");
                }
                break;
                
            case 2:
                if (confirm_reset_operation(RESET_TYPE_SHELL)) {
                    ret = perform_shell_reset(pcie_base, 1);
                    if (ret != 0) {
                        printf("Shell层复位失败\n");
                    }
                } else {
                    printf("操作已取消\n");
                }
                break;
                
            case 3:
                if (confirm_reset_operation(RESET_TYPE_USER)) {
                    ret = perform_user_reset(pcie_base, 1);
                    if (ret != 0) {
                        printf("用户复位失败\n");
                    }
                } else {
                    printf("操作已取消\n");
                }
                break;
                
            case 4:
                if (confirm_reset_operation(RESET_TYPE_CMAC_PORT0)) {
                    ret = perform_cmac_reset(pcie_base, 0, 1);
                    if (ret != 0) {
                        printf("CMAC端口0复位失败\n");
                    }
                } else {
                    printf("操作已取消\n");
                }
                break;
                
            case 5:
                if (confirm_reset_operation(RESET_TYPE_CMAC_PORT1)) {
                    ret = perform_cmac_reset(pcie_base, 1, 1);
                    if (ret != 0) {
                        printf("CMAC端口1复位失败\n");
                    }
                } else {
                    printf("操作已取消\n");
                }
                break;
                
            case 6:
                if (confirm_reset_operation(RESET_TYPE_CMAC_GT_PORT0)) {
                    ret = perform_cmac_gt_reset(pcie_base, 0, 1);
                    if (ret != 0) {
                        printf("CMAC GT端口0复位失败\n");
                    }
                } else {
                    printf("操作已取消\n");
                }
                break;
                
            case 7:
                if (confirm_reset_operation(RESET_TYPE_CMAC_GT_PORT1)) {
                    ret = perform_cmac_gt_reset(pcie_base, 1, 1);
                    if (ret != 0) {
                        printf("CMAC GT端口1复位失败\n");
                    }
                } else {
                    printf("操作已取消\n");
                }
                break;
                
            case 8:
                display_all_reset_status(pcie_base);
                break;
                
            case 0:
                printf("退出交互模式\n");
                return 0;
                
            default:
                printf("无效选择，请输入 0-8 之间的数字\n");
                break;
        }
    }
    
    return 0;
}

/**
 * @brief 打印复位帮助信息
 */
void print_reset_help(void)
{
    printf("\n=== RecoNIC复位功能帮助 ===\n\n");
    printf("复位类型说明：\n");
    printf("  系统复位     - 完整的系统复位，影响整个RecoNIC系统\n");
    printf("  Shell层复位  - 重置网卡Shell层各个子模块\n");
    printf("  用户复位     - 重置用户可编程逻辑部分\n");
    printf("  CMAC端口复位 - 重置指定的100G以太网端口\n");
    printf("  CMAC GT复位  - 重置指定端口的GT收发器\n");
    printf("\n复位寄存器地址：\n");
    printf("  系统复位寄存器：   0x%04X\n", SYSCFG_OFFSET_SYSTEM_RESET);
    printf("  Shell层复位寄存器：0x%04X\n", SYSCFG_OFFSET_SHELL_RESET);
    printf("  用户复位寄存器：   0x%04X\n", SYSCFG_OFFSET_USER_RESET);
    printf("\n状态寄存器地址：\n");
    printf("  系统状态寄存器：   0x%04X\n", SYSCFG_OFFSET_SYSTEM_STATUS);
    printf("  Shell层状态寄存器：0x%04X\n", SYSCFG_OFFSET_SHELL_STATUS);
    printf("  用户状态寄存器：   0x%04X\n", SYSCFG_OFFSET_USER_STATUS);
    printf("\n安全注意事项：\n");
    printf("  - 复位操作可能导致网络连接中断\n");
    printf("  - 系统复位会影响整个RecoNIC系统\n");
    printf("  - 请在维护窗口期间执行复位操作\n");
    printf("  - 建议先查看状态再执行复位\n");
    printf("=============================\n\n");
}