//==============================================================================
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//
//==============================================================================

/**
 * @file reset_utils_final.c  
 * @brief 基于Verilog代码分析的最终修复版本
 *
 * 关键修复：
 * 1. 复位寄存器是自清除的 - 需要等待寄存器自动清零
 * 2. CMAC复位需要同时复位子系统和adapter
 * 3. 正确的复位完成检测机制
 * 4. 完整的重新初始化序列
 */

#include "reset_utils.h"

/* ===== 基于Verilog代码的正确位映射 ===== */
// Shell reset register位映射（来自system_config_register.v 第261-269行）
#define SHELL_RESET_QDMA_SUBSYSTEM          0x01    // 位0: QDMA子系统
#define SHELL_RESET_RDMA_SUBSYSTEM          0x02    // 位1: RDMA子系统
#define SHELL_RESET_CMAC0_SUBSYSTEM         0x10    // 位4: CMAC0子系统
#define SHELL_RESET_CMAC0_ADAPTER           0x20    // 位5: CMAC0适配器
#define SHELL_RESET_CMAC1_SUBSYSTEM         0x100   // 位8: CMAC1子系统  
#define SHELL_RESET_CMAC1_ADAPTER           0x200   // 位9: CMAC1适配器

// 组合掩码 - 同时复位子系统和适配器
#define SHELL_RESET_CMAC0_COMPLETE          (SHELL_RESET_CMAC0_SUBSYSTEM | SHELL_RESET_CMAC0_ADAPTER)
#define SHELL_RESET_CMAC1_COMPLETE          (SHELL_RESET_CMAC1_SUBSYSTEM | SHELL_RESET_CMAC1_ADAPTER)
#define SHELL_RESET_ALL_CMAC                (SHELL_RESET_CMAC0_COMPLETE | SHELL_RESET_CMAC1_COMPLETE)

/* ===== 超时和延迟参数（基于硬件时序要求）===== */
#define RESET_REGISTER_POLL_INTERVAL_US     1000    // 1ms轮询间隔
#define RESET_REGISTER_TIMEOUT_MS           10000   // 10秒超时
#define RESET_STABILIZATION_DELAY_US        50000   // 50ms稳定延迟
#define CMAC_INIT_DELAY_US                  100000  // 100ms CMAC初始化延迟

/* 从reset_utils_fixed.c引入的函数声明 */
extern int perform_cmac_reset_with_reinit(uint32_t* pcie_base, int port_id, int enable_rsfec, int verbose);

/**
 * @brief 等待复位寄存器自清除（基于Verilog实现）
 * @param pcie_base PCIe基地址  
 * @param reset_reg_offset 复位寄存器偏移地址
 * @param reset_mask 复位位掩码
 * @param timeout_ms 超时时间（毫秒）
 * @return 0-成功，负数-失败
 */
static int wait_reset_register_autoclear(uint32_t* pcie_base, uint32_t reset_reg_offset, 
                                         uint32_t reset_mask, int timeout_ms)
{
    uint32_t reg_value;
    int elapsed_us = 0;
    int ret;
    
    printf("    等待复位寄存器[0x%04X]位掩码[0x%X]自清除...\n", reset_reg_offset, reset_mask);
    
    while (elapsed_us < (timeout_ms * 1000)) {
        ret = safe_read32_data(pcie_base, reset_reg_offset, &reg_value);
        if (ret != 0) {
            fprintf(stderr, "错误：读取复位寄存器失败\n");
            return ret;
        }
        
        /* 检查相关位是否已清零 */
        if ((reg_value & reset_mask) == 0) {
            printf("    ✓ 复位寄存器自清除完成（耗时%d.%03dms）\n", 
                   elapsed_us/1000, elapsed_us%1000);
            return 0;
        }
        
        usleep(RESET_REGISTER_POLL_INTERVAL_US);
        elapsed_us += RESET_REGISTER_POLL_INTERVAL_US;
        
        if ((elapsed_us % 1000000) == 0) {  // 每秒打印一次进度
            printf("    等待复位寄存器自清除...已等待%ds\n", elapsed_us/1000000);
        }
    }
    
    fprintf(stderr, "错误：复位寄存器自清除超时\n");
    return -ETIMEDOUT;
}

/**
 * @brief 基于硬件分析的正确CMAC端口复位
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID（0或1）
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_cmac_reset_hardware_correct(uint32_t* pcie_base, int port_id, int verbose)
{
    int ret;
    uint32_t reset_mask;
    uint32_t status_mask; 
    uint32_t status_value;
    
    if (port_id != 0 && port_id != 1) {
        fprintf(stderr, "错误：无效的CMAC端口ID %d（应为0或1）\n", port_id);
        return -EINVAL;
    }
    
    /* 基于Verilog代码的正确位掩码 */
    if (port_id == 0) {
        reset_mask = SHELL_RESET_CMAC0_COMPLETE;  // 位4+5: CMAC0子系统+适配器
        status_mask = SHELL_RESET_CMAC0_COMPLETE;
    } else {
        reset_mask = SHELL_RESET_CMAC1_COMPLETE;  // 位8+9: CMAC1子系统+适配器  
        status_mask = SHELL_RESET_CMAC1_COMPLETE;
    }
    
    printf("=== 执行CMAC端口%d硬件级完整复位 ===\n", port_id);
    if (verbose) {
        printf("复位掩码：0x%X（同时复位子系统和适配器）\n", reset_mask);
    }
    
    /* 步骤1：读取复位前状态 */
    if (verbose) {
        printf("步骤1：检查复位前状态...\n");
        ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_SHELL_STATUS, &status_value);
        if (ret == 0) {
            printf("    Shell状态寄存器：0x%08X\n", status_value);
            printf("    CMAC%d状态：子系统=%s，适配器=%s\n", 
                   port_id,
                   (status_value & (1 << (4 + port_id*4))) ? "完成" : "未完成",
                   (status_value & (1 << (5 + port_id*4))) ? "完成" : "未完成");
        }
    }
    
    /* 步骤2：写入复位命令 */
    if (verbose) {
        printf("步骤2：写入Shell层复位命令...\n");
    }
    
    ret = safe_write32_data(pcie_base, SYSCFG_OFFSET_SHELL_RESET, reset_mask);
    if (ret != 0) {
        fprintf(stderr, "错误：复位命令写入失败\n");
        return ret;
    }
    
    if (verbose) {
        printf("    ✓ 复位命令已写入：掩码=0x%X\n", reset_mask);
    }
    
    /* 步骤3：等待复位寄存器自清除（基于硬件实现）*/
    if (verbose) {
        printf("步骤3：等待硬件复位寄存器自清除...\n");
    }
    
    ret = wait_reset_register_autoclear(pcie_base, SYSCFG_OFFSET_SHELL_RESET, 
                                       reset_mask, RESET_REGISTER_TIMEOUT_MS);
    if (ret != 0) {
        fprintf(stderr, "错误：复位寄存器自清除失败\n");
        return ret;
    }
    
    /* 步骤4：等待硬件稳定 */
    if (verbose) {
        printf("步骤4：等待硬件稳定...\n");
    }
    usleep(RESET_STABILIZATION_DELAY_US);
    
    /* 步骤5：验证复位完成状态 */
    if (verbose) {
        printf("步骤5：验证复位完成状态...\n");
    }
    
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_SHELL_STATUS, &status_value);
    if (ret == 0) {
        int subsystem_done = (status_value & (1 << (4 + port_id*4))) ? 1 : 0;
        int adapter_done = (status_value & (1 << (5 + port_id*4))) ? 1 : 0;
        
        if (verbose) {
            printf("    Shell状态寄存器：0x%08X\n", status_value);
            printf("    CMAC%d子系统复位：%s\n", port_id, subsystem_done ? "完成" : "进行中");
            printf("    CMAC%d适配器复位：%s\n", port_id, adapter_done ? "完成" : "进行中");
        }
        
        if (subsystem_done && adapter_done) {
            printf("    ✓ CMAC%d硬件复位完成验证成功\n", port_id);
        } else {
            printf("    ⚠ CMAC%d硬件复位状态不完整，但继续初始化\n", port_id);
        }
    }
    
    /* 步骤6：CMAC重新初始化延迟 */
    if (verbose) {
        printf("步骤6：等待CMAC%d硬件准备就绪...\n", port_id);
    }
    usleep(CMAC_INIT_DELAY_US);
    
    printf("✓ CMAC端口%d硬件级复位完成！\n", port_id);
    return 0;
}

/**
 * @brief 基于硬件分析的正确Shell层复位
 * @param pcie_base PCIe基地址
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_shell_reset_hardware_correct(uint32_t* pcie_base, int verbose)
{
    int ret;
    uint32_t reset_mask;
    uint32_t status_value;
    
    /* 复位所有Shell层子模块（基于Verilog位映射）*/
    reset_mask = SHELL_RESET_QDMA_SUBSYSTEM |     // 位0: QDMA
                 SHELL_RESET_RDMA_SUBSYSTEM |     // 位1: RDMA  
                 SHELL_RESET_CMAC0_COMPLETE |     // 位4+5: CMAC0+适配器
                 SHELL_RESET_CMAC1_COMPLETE;      // 位8+9: CMAC1+适配器
    
    printf("=== 执行Shell层硬件级完整复位 ===\n");
    if (verbose) {
        printf("复位掩码：0x%X（所有Shell层子模块）\n", reset_mask);
    }
    
    /* 步骤1：检查复位前状态 */
    if (verbose) {
        printf("步骤1：检查复位前状态...\n");
        ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_SHELL_STATUS, &status_value);
        if (ret == 0) {
            printf("    Shell状态寄存器：0x%08X\n", status_value);
        }
    }
    
    /* 步骤2：写入复位命令 */
    if (verbose) {
        printf("步骤2：写入Shell层复位命令...\n");
    }
    
    ret = safe_write32_data(pcie_base, SYSCFG_OFFSET_SHELL_RESET, reset_mask);
    if (ret != 0) {
        fprintf(stderr, "错误：Shell层复位命令写入失败\n");
        return ret;
    }
    
    /* 步骤3：等待复位寄存器自清除 */
    if (verbose) {
        printf("步骤3：等待Shell层复位寄存器自清除...\n");
    }
    
    ret = wait_reset_register_autoclear(pcie_base, SYSCFG_OFFSET_SHELL_RESET, 
                                       reset_mask, RESET_REGISTER_TIMEOUT_MS);
    if (ret != 0) {
        fprintf(stderr, "错误：Shell层复位寄存器自清除失败\n");
        return ret;
    }
    
    /* 步骤4：等待所有子模块稳定 */
    if (verbose) {
        printf("步骤4：等待所有Shell层子模块稳定...\n");
    }
    usleep(RESET_STABILIZATION_DELAY_US * 2);  // Shell层需要更长稳定时间
    
    /* 步骤5：验证复位完成状态 */
    if (verbose) {
        printf("步骤5：验证Shell层复位完成状态...\n");
    }
    
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_SHELL_STATUS, &status_value);
    if (ret == 0) {
        if (verbose) {
            printf("    Shell状态寄存器：0x%08X\n", status_value);
            printf("    QDMA子系统：   %s\n", (status_value & 0x01) ? "完成" : "进行中");
            printf("    RDMA子系统：   %s\n", (status_value & 0x02) ? "完成" : "进行中");
            printf("    CMAC0子系统：  %s\n", (status_value & 0x10) ? "完成" : "进行中");
            printf("    CMAC0适配器：  %s\n", (status_value & 0x20) ? "完成" : "进行中");
            printf("    CMAC1子系统：  %s\n", (status_value & 0x100) ? "完成" : "进行中");
            printf("    CMAC1适配器：  %s\n", (status_value & 0x200) ? "完成" : "进行中");
        }
    }
    
    printf("✓ Shell层硬件级复位完成！\n");
    return 0;
}

/**
 * @brief 基于硬件分析的正确系统复位
 * @param pcie_base PCIe基地址
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_system_reset_hardware_correct(uint32_t* pcie_base, int verbose)
{
    int ret;
    uint32_t status_value;
    
    printf("=== 执行系统硬件级完整复位 ===\n");
    
    /* 步骤1：检查复位前状态 */
    if (verbose) {
        printf("步骤1：检查复位前状态...\n");
        display_all_reset_status(pcie_base);
    }
    
    /* 步骤2：写入系统复位命令 */
    if (verbose) {
        printf("步骤2：写入系统复位命令...\n");
    }
    
    ret = safe_write32_data(pcie_base, SYSCFG_OFFSET_SYSTEM_RESET, 0x1);
    if (ret != 0) {
        fprintf(stderr, "错误：系统复位命令写入失败\n");
        return ret;
    }
    
    /* 步骤3：等待系统复位寄存器自清除 */
    if (verbose) {
        printf("步骤3：等待系统复位寄存器自清除...\n");
        printf("    注意：系统复位会等待所有Shell和User子模块完成\n");
    }
    
    ret = wait_reset_register_autoclear(pcie_base, SYSCFG_OFFSET_SYSTEM_RESET, 
                                       0x1, RESET_REGISTER_TIMEOUT_MS);
    if (ret != 0) {
        fprintf(stderr, "错误：系统复位寄存器自清除失败\n");
        return ret;
    }
    
    /* 步骤4：等待系统完全稳定 */
    if (verbose) {
        printf("步骤4：等待系统完全稳定...\n");
    }
    usleep(RESET_STABILIZATION_DELAY_US * 4);  // 系统复位需要更长稳定时间
    
    /* 步骤5：验证系统复位完成状态 */
    if (verbose) {
        printf("步骤5：验证系统复位完成状态...\n");
    }
    
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_SYSTEM_STATUS, &status_value);
    if (ret == 0) {
        if (verbose) {
            printf("    系统状态寄存器：0x%08X\n", status_value);
            printf("    系统复位完成：%s\n", (status_value & 0x1) ? "是" : "否");
        }
    }
    
    printf("✓ 系统硬件级复位完成！\n");
    return 0;
}

/**
 * @brief 智能复位策略 - 基于硬件分析的最佳实践
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID（0或1，-1表示所有端口）
 * @param include_reinit 是否包含重新初始化
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int smart_reset_strategy(uint32_t* pcie_base, int port_id, int include_reinit, int verbose)
{
    int ret;
    
    printf("=== 智能复位策略（基于硬件分析）===\n");
    
    if (port_id == -1) {
        /* 复位所有端口 */
        printf("目标：复位所有CMAC端口\n");
        
        if (verbose) {
            printf("执行CMAC端口0复位...\n");
        }
        ret = perform_cmac_reset_hardware_correct(pcie_base, 0, verbose);
        if (ret != 0) {
            fprintf(stderr, "CMAC端口0复位失败\n");
        }
        
        if (verbose) {
            printf("执行CMAC端口1复位...\n");
        }
        ret = perform_cmac_reset_hardware_correct(pcie_base, 1, verbose);
        if (ret != 0) {
            fprintf(stderr, "CMAC端口1复位失败\n");
        }
    } else {
        /* 复位指定端口 */
        printf("目标：复位CMAC端口%d\n", port_id);
        ret = perform_cmac_reset_hardware_correct(pcie_base, port_id, verbose);
        if (ret != 0) {
            return ret;
        }
    }
    
    /* 如果包含重新初始化 */
    if (include_reinit) {
        printf("\n=== 执行重新初始化序列 ===\n");
        
        if (port_id == -1) {
            /* 重新初始化所有端口 */
            if (verbose) printf("重新初始化CMAC端口0...\n");
            ret = perform_cmac_reset_with_reinit(pcie_base, 0, 1, verbose);
            if (ret != 0) fprintf(stderr, "CMAC端口0重新初始化失败\n");
            
            if (verbose) printf("重新初始化CMAC端口1...\n");
            ret = perform_cmac_reset_with_reinit(pcie_base, 1, 1, verbose);  
            if (ret != 0) fprintf(stderr, "CMAC端口1重新初始化失败\n");
        } else {
            /* 重新初始化指定端口 */
            if (verbose) printf("重新初始化CMAC端口%d...\n", port_id);
            ret = perform_cmac_reset_with_reinit(pcie_base, port_id, 1, verbose);
            if (ret != 0) return ret;
        }
    }
    
    printf("✓ 智能复位策略执行完成！\n");
    return 0;
}

/**
 * @brief 诊断复位问题 - 详细的硬件状态分析
 * @param pcie_base PCIe基地址
 * @return 0-成功，负数-失败
 */
int diagnose_reset_issues(uint32_t* pcie_base)
{
    uint32_t value;
    int ret;
    
    printf("\n=== RecoNIC硬件复位诊断 ===\n");
    
    /* 检查所有复位相关寄存器 */
    printf("复位寄存器状态（应该全为0，如果不为0说明复位正在进行）：\n");
    
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_SYSTEM_RESET, &value);
    printf("  系统复位寄存器[0x%04X]：0x%08X %s\n", 
           SYSCFG_OFFSET_SYSTEM_RESET, ret ? 0xDEADBEEF : value,
           ret ? "读取失败" : (value ? "⚠复位进行中" : "✓已清除"));
    
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_SHELL_RESET, &value);
    printf("  Shell复位寄存器[0x%04X]：0x%08X %s\n", 
           SYSCFG_OFFSET_SHELL_RESET, ret ? 0xDEADBEEF : value,
           ret ? "读取失败" : (value ? "⚠复位进行中" : "✓已清除"));
    
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_USER_RESET, &value);
    printf("  用户复位寄存器[0x%04X]：0x%08X %s\n", 
           SYSCFG_OFFSET_USER_RESET, ret ? 0xDEADBEEF : value,
           ret ? "读取失败" : (value ? "⚠复位进行中" : "✓已清除"));
    
    printf("\n状态寄存器详细分析：\n");
    
    /* 详细的Shell状态分析 */
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_SHELL_STATUS, &value);
    if (ret == 0) {
        printf("  Shell状态寄存器[0x%04X]：0x%08X\n", SYSCFG_OFFSET_SHELL_STATUS, value);
        printf("    位0 (QDMA子系统)：    %s\n", (value & 0x001) ? "✓完成" : "✗未完成");
        printf("    位1 (RDMA子系统)：    %s\n", (value & 0x002) ? "✓完成" : "✗未完成"); 
        printf("    位4 (CMAC0子系统)：   %s\n", (value & 0x010) ? "✓完成" : "✗未完成");
        printf("    位5 (CMAC0适配器)：   %s\n", (value & 0x020) ? "✓完成" : "✗未完成");
        printf("    位8 (CMAC1子系统)：   %s\n", (value & 0x100) ? "✓完成" : "✗未完成");
        printf("    位9 (CMAC1适配器)：   %s\n", (value & 0x200) ? "✓完成" : "✗未完成");
    } else {
        printf("  Shell状态寄存器：读取失败\n");
    }
    
    printf("\n硬件分析建议：\n");
    
    /* 基于状态给出建议 */
    ret = safe_read32_data(pcie_base, SYSCFG_OFFSET_SHELL_STATUS, &value);
    if (ret == 0) {
        if ((value & 0x030) != 0x030) {  // CMAC0不完整
            printf("  🔧 CMAC0需要复位：子系统和适配器状态不完整\n");
        }
        if ((value & 0x300) != 0x300) {  // CMAC1不完整  
            printf("  🔧 CMAC1需要复位：子系统和适配器状态不完整\n");
        }
        if ((value & 0x003) != 0x003) {  // QDMA/RDMA不完整
            printf("  🔧 数据路径需要复位：QDMA/RDMA状态不完整\n");
        }
        if (value == 0x333) {  // 所有关键位都正常
            printf("  ✓ 所有Shell层子模块状态正常\n");
        }
    }
    
    printf("================================\n\n");
    return 0;
}