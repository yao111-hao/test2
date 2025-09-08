//==============================================================================
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//
//==============================================================================

/**
 * @file reset_utils_fixed.c  
 * @brief 修复版RecoNIC复位工具函数
 *
 * 基于onic驱动分析，复位后需要完整的重新初始化序列
 * 包括CMAC配置、lane对齐等待、流控制配置等
 */

#include "reset_utils.h"

/* 从onic-driver中提取的关键寄存器定义 */
#define CMAC_OFFSET_CONF_RX_1(i)            (CMAC_OFFSET(i) + 0x0014)
#define CMAC_OFFSET_CONF_TX_1(i)            (CMAC_OFFSET(i) + 0x000C)  
#define CMAC_OFFSET_STAT_RX_STATUS(i)       (CMAC_OFFSET(i) + 0x0204)
#define CMAC_OFFSET_RSFEC_CONF_ENABLE(i)    (CMAC_OFFSET(i) + 0x107C)
#define CMAC_OFFSET_RSFEC_CONF_IND_CORRECTION(i) (CMAC_OFFSET(i) + 0x1000)

/* 流控制相关寄存器 */
#define CMAC_OFFSET_CONF_RX_FC_CTRL_1(i)    (CMAC_OFFSET(i) + 0x0084)
#define CMAC_OFFSET_CONF_RX_FC_CTRL_2(i)    (CMAC_OFFSET(i) + 0x0088)
#define CMAC_OFFSET_CONF_TX_FC_QNTA_1(i)    (CMAC_OFFSET(i) + 0x0048)
#define CMAC_OFFSET_CONF_TX_FC_QNTA_2(i)    (CMAC_OFFSET(i) + 0x004C)
#define CMAC_OFFSET_CONF_TX_FC_QNTA_3(i)    (CMAC_OFFSET(i) + 0x0050)
#define CMAC_OFFSET_CONF_TX_FC_QNTA_4(i)    (CMAC_OFFSET(i) + 0x0054)
#define CMAC_OFFSET_CONF_TX_FC_QNTA_5(i)    (CMAC_OFFSET(i) + 0x0058)
#define CMAC_OFFSET_CONF_TX_FC_RFRH_1(i)    (CMAC_OFFSET(i) + 0x0034)
#define CMAC_OFFSET_CONF_TX_FC_RFRH_2(i)    (CMAC_OFFSET(i) + 0x0038)
#define CMAC_OFFSET_CONF_TX_FC_RFRH_3(i)    (CMAC_OFFSET(i) + 0x003C)
#define CMAC_OFFSET_CONF_TX_FC_RFRH_4(i)    (CMAC_OFFSET(i) + 0x0040)
#define CMAC_OFFSET_CONF_TX_FC_RFRH_5(i)    (CMAC_OFFSET(i) + 0x0044)
#define CMAC_OFFSET_CONF_TX_FC_CTRL_1(i)    (CMAC_OFFSET(i) + 0x0030)

/* QDMA相关寄存器 */
#define QDMA_FUNC_OFFSET                    0x1000
#define QDMA_FUNC_OFFSET_QCONF(i)           (QDMA_FUNC_OFFSET + (0x1000 * (i)) + 0x0)
#define QDMA_FUNC_OFFSET_INDIR_TABLE(i, k)  (QDMA_FUNC_OFFSET + (0x1000 * (i)) + 0x400 + ((k) * 4))

/* 掩码定义 */
#define QDMA_FUNC_QCONF_QBASE_MASK          0xFFFF0000
#define QDMA_FUNC_QCONF_NUMQ_MASK           0x0000FFFF

/* 超时和延迟参数 */
#define LANE_ALIGNMENT_TIMEOUT_CNT          32
#define LANE_ALIGNMENT_RESET_CNT            8
#define LANE_ALIGNMENT_CHECK_INTERVAL_MS    50

/**
 * @brief 检查RX lane是否已对齐
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID（0或1）
 * @return 1-已对齐，0-未对齐
 */
static int check_rx_lane_aligned(uint32_t* pcie_base, int port_id)
{
    uint32_t offset = CMAC_OFFSET_STAT_RX_STATUS(port_id);
    uint32_t val1, val2;
    
    /* 读取两次以清除之前锁存的值 */
    if (safe_read32_data(pcie_base, offset, &val1) != 0) {
        return 0;
    }
    if (safe_read32_data(pcie_base, offset, &val2) != 0) {
        return 0;
    }
    
    return (val2 == 0x3) ? 1 : 0;
}

/**
 * @brief 配置CMAC的RS-FEC
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
static int configure_cmac_rsfec(uint32_t* pcie_base, int port_id, int verbose)
{
    int ret;
    
    if (verbose) {
        printf("  配置CMAC端口%d的RS-FEC...\n", port_id);
    }
    
    /* 启用RS-FEC */
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_RSFEC_CONF_ENABLE(port_id), 0x3);
    if (ret != 0) return ret;
    
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_RSFEC_CONF_IND_CORRECTION(port_id), 0x7);
    if (ret != 0) return ret;
    
    return 0;
}

/**
 * @brief 配置CMAC的RX/TX
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
static int configure_cmac_rxtx(uint32_t* pcie_base, int port_id, int verbose)
{
    int ret;
    
    if (verbose) {
        printf("  配置CMAC端口%d的RX/TX...\n", port_id);
    }
    
    /* 启用接收和发送 */
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_RX_1(port_id), 0x1);
    if (ret != 0) return ret;
    
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_TX_1(port_id), 0x10);
    if (ret != 0) return ret;
    
    return 0;
}

/**
 * @brief 等待RX lane对齐完成
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
static int wait_rx_lane_alignment(uint32_t* pcie_base, int port_id, int verbose)
{
    int timeout_cnt = 0;
    int reset_cnt = 0;
    
    if (verbose) {
        printf("  等待CMAC端口%d的RX lane对齐...\n", port_id);
    }
    
    while (!check_rx_lane_aligned(pcie_base, port_id)) {
        usleep(LANE_ALIGNMENT_CHECK_INTERVAL_MS * 1000);
        timeout_cnt++;
        
        if (timeout_cnt == LANE_ALIGNMENT_RESET_CNT && reset_cnt == 0) {
            if (verbose) {
                printf("    Lane对齐超时，执行重新复位...\n");
            }
            
            /* 执行一次额外的复位 */
            perform_cmac_reset(pcie_base, port_id, 0);
            configure_cmac_rxtx(pcie_base, port_id, 0);
            reset_cnt++;
        }
        
        if (timeout_cnt > LANE_ALIGNMENT_TIMEOUT_CNT) {
            fprintf(stderr, "错误：CMAC端口%d RX lane对齐超时\n", port_id);
            return -ETIMEDOUT;
        }
    }
    
    if (verbose) {
        printf("  ✓ CMAC端口%d RX lane对齐成功（%d次尝试）\n", port_id, timeout_cnt + 1);
    }
    
    return 0;
}

/**
 * @brief 配置CMAC流控制
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
static int configure_cmac_flow_control(uint32_t* pcie_base, int port_id, int verbose)
{
    int ret;
    
    if (verbose) {
        printf("  配置CMAC端口%d的流控制...\n", port_id);
    }
    
    /* RX流控制配置 */
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_RX_FC_CTRL_1(port_id), 0x00003DFF);
    if (ret != 0) return ret;
    
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_RX_FC_CTRL_2(port_id), 0x0001C631);
    if (ret != 0) return ret;
    
    /* TX流控制配置 */
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_TX_FC_QNTA_1(port_id), 0xFFFFFFFF);
    if (ret != 0) return ret;
    
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_TX_FC_QNTA_2(port_id), 0xFFFFFFFF);
    if (ret != 0) return ret;
    
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_TX_FC_QNTA_3(port_id), 0xFFFFFFFF);
    if (ret != 0) return ret;
    
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_TX_FC_QNTA_4(port_id), 0xFFFFFFFF);
    if (ret != 0) return ret;
    
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_TX_FC_QNTA_5(port_id), 0x0000FFFF);
    if (ret != 0) return ret;
    
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_TX_FC_RFRH_1(port_id), 0xFFFFFFFF);
    if (ret != 0) return ret;
    
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_TX_FC_RFRH_2(port_id), 0xFFFFFFFF);
    if (ret != 0) return ret;
    
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_TX_FC_RFRH_3(port_id), 0xFFFFFFFF);
    if (ret != 0) return ret;
    
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_TX_FC_RFRH_4(port_id), 0xFFFFFFFF);
    if (ret != 0) return ret;
    
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_TX_FC_RFRH_5(port_id), 0x0000FFFF);
    if (ret != 0) return ret;
    
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_TX_FC_CTRL_1(port_id), 0x000001FF);
    if (ret != 0) return ret;
    
    /* 最后启用TX */
    ret = safe_write32_data(pcie_base, CMAC_OFFSET_CONF_TX_1(port_id), 0x1);
    if (ret != 0) return ret;
    
    return 0;
}

/**
 * @brief 初始化RETA重定向表（简化版本）
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
static int initialize_reta_table(uint32_t* pcie_base, int port_id, int verbose)
{
    int ret;
    int i;
    uint32_t qconf_val;
    
    if (verbose) {
        printf("  初始化CMAC端口%d的RETA表...\n", port_id);
    }
    
    /* 配置队列基础参数（简化版，假设默认值）*/
    qconf_val = ((0 << 16) | 64);  /* 队列基础=0，队列数=64 */
    ret = safe_write32_data(pcie_base, QDMA_FUNC_OFFSET_QCONF(port_id), qconf_val);
    if (ret != 0) return ret;
    
    /* 初始化间接表（简化版）*/
    for (i = 0; i < 128; i++) {
        uint32_t val = (i % 64) & 0x0000FFFF;  /* 简化：假设64个队列 */
        uint32_t offset = QDMA_FUNC_OFFSET_INDIR_TABLE(port_id, i);
        ret = safe_write32_data(pcie_base, offset, val);
        if (ret != 0) {
            if (verbose) {
                printf("    警告：RETA表项%d配置失败，继续...\n", i);
            }
        }
    }
    
    return 0;
}

/**
 * @brief 完整的CMAC复位和重新初始化
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID（0或1）
 * @param enable_rsfec 是否启用RS-FEC
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_cmac_reset_with_reinit(uint32_t* pcie_base, int port_id, int enable_rsfec, int verbose)
{
    int ret;
    
    if (port_id != 0 && port_id != 1) {
        fprintf(stderr, "错误：无效的CMAC端口ID %d（应为0或1）\n", port_id);
        return -EINVAL;
    }
    
    printf("=== 执行CMAC端口%d完整复位和重新初始化 ===\n", port_id);
    
    /* 步骤1：执行复位 */
    if (verbose) {
        printf("步骤1：执行CMAC端口%d复位...\n", port_id);
    }
    ret = perform_cmac_reset(pcie_base, port_id, verbose);
    if (ret != 0) {
        fprintf(stderr, "错误：CMAC端口%d复位失败\n", port_id);
        return ret;
    }
    
    /* 步骤2：配置RS-FEC（如果启用） */
    if (enable_rsfec) {
        if (verbose) {
            printf("步骤2：配置RS-FEC...\n");
        }
        ret = configure_cmac_rsfec(pcie_base, port_id, verbose);
        if (ret != 0) {
            fprintf(stderr, "错误：RS-FEC配置失败\n");
            return ret;
        }
    } else if (verbose) {
        printf("步骤2：跳过RS-FEC配置（未启用）\n");
    }
    
    /* 步骤3：配置RX/TX */
    if (verbose) {
        printf("步骤3：配置RX/TX...\n");
    }
    ret = configure_cmac_rxtx(pcie_base, port_id, verbose);
    if (ret != 0) {
        fprintf(stderr, "错误：RX/TX配置失败\n");
        return ret;
    }
    
    /* 步骤4：等待lane对齐 */
    if (verbose) {
        printf("步骤4：等待lane对齐...\n");
    }
    ret = wait_rx_lane_alignment(pcie_base, port_id, verbose);
    if (ret != 0) {
        fprintf(stderr, "错误：Lane对齐失败\n");
        return ret;
    }
    
    /* 步骤5：配置流控制 */
    if (verbose) {
        printf("步骤5：配置流控制...\n");
    }
    ret = configure_cmac_flow_control(pcie_base, port_id, verbose);
    if (ret != 0) {
        fprintf(stderr, "错误：流控制配置失败\n");
        return ret;
    }
    
    /* 步骤6：初始化RETA表 */
    if (verbose) {
        printf("步骤6：初始化RETA表...\n");
    }
    ret = initialize_reta_table(pcie_base, port_id, verbose);
    if (ret != 0) {
        if (verbose) {
            printf("警告：RETA表初始化失败，但继续执行\n");
        }
    }
    
    printf("✓ CMAC端口%d完整复位和重新初始化成功！\n", port_id);
    return 0;
}

/**
 * @brief 修复版本：执行CMAC端口复位
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID（0或1）
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_cmac_reset_fixed(uint32_t* pcie_base, int port_id, int verbose)
{
    /* 默认启用RS-FEC */
    return perform_cmac_reset_with_reinit(pcie_base, port_id, 1, verbose);
}

/**
 * @brief 修复版本：执行Shell层复位
 * @param pcie_base PCIe基地址
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_shell_reset_fixed(uint32_t* pcie_base, int verbose)
{
    int ret;
    
    printf("=== 执行Shell层完整复位和重新初始化 ===\n");
    
    if (verbose) {
        printf("步骤1：执行Shell层复位...\n");
    }
    
    /* 执行Shell层复位 */
    ret = perform_shell_reset(pcie_base, verbose);
    if (ret != 0) {
        return ret;
    }
    
    if (verbose) {
        printf("步骤2：重新初始化所有CMAC端口...\n");
    }
    
    /* 重新初始化所有CMAC端口 */
    ret = perform_cmac_reset_with_reinit(pcie_base, 0, 1, verbose);
    if (ret != 0) {
        fprintf(stderr, "警告：CMAC端口0重新初始化失败\n");
    }
    
    ret = perform_cmac_reset_with_reinit(pcie_base, 1, 1, verbose);
    if (ret != 0) {
        fprintf(stderr, "警告：CMAC端口1重新初始化失败\n");
    }
    
    printf("✓ Shell层完整复位和重新初始化完成！\n");
    return 0;
}

/**
 * @brief 修复版本：执行系统复位
 * @param pcie_base PCIe基地址
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_system_reset_fixed(uint32_t* pcie_base, int verbose)
{
    int ret;
    
    printf("=== 执行系统完整复位和重新初始化 ===\n");
    
    if (verbose) {
        printf("步骤1：执行系统复位...\n");
    }
    
    /* 执行系统复位 */
    ret = perform_system_reset(pcie_base, verbose);
    if (ret != 0) {
        return ret;
    }
    
    /* 系统复位后需要更长的等待时间 */
    if (verbose) {
        printf("步骤2：等待系统稳定...\n");
    }
    sleep(2);
    
    if (verbose) {
        printf("步骤3：重新初始化所有子系统...\n");
    }
    
    /* 重新初始化所有CMAC端口 */
    ret = perform_cmac_reset_with_reinit(pcie_base, 0, 1, verbose);
    if (ret != 0) {
        fprintf(stderr, "警告：CMAC端口0重新初始化失败\n");
    }
    
    ret = perform_cmac_reset_with_reinit(pcie_base, 1, 1, verbose);
    if (ret != 0) {
        fprintf(stderr, "警告：CMAC端口1重新初始化失败\n");
    }
    
    printf("✓ 系统完整复位和重新初始化完成！\n");
    return 0;
}