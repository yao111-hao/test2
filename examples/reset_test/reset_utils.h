//==============================================================================
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//
//==============================================================================

#ifndef __RESET_UTILS_H__
#define __RESET_UTILS_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>

/*! \def REG_MAP_SIZE
 *  \brief PCIe BAR空间映射大小：64KB，足够访问所有复位寄存器
 */
#define REG_MAP_SIZE 0x00010000

/*! \def DEFAULT_DEVICE
 *  \brief 默认设备路径
 */
#define DEFAULT_DEVICE "/dev/reconic-mm"

/*! \def DEFAULT_PCIE_RESOURCE
 *  \brief 默认PCIe资源路径
 */
#define DEFAULT_PCIE_RESOURCE "/sys/bus/pci/devices/0005:01:00.0/resource2" // /sys/bus/pci/devices/0005\:01\:00.0/resource2

/*! \def RESET_TIMEOUT_MS
 *  \brief 复位超时时间（毫秒）
 */
#define RESET_TIMEOUT_MS 5000

/*! \def RESET_POLL_INTERVAL_MS
 *  \brief 复位状态轮询间隔（毫秒）
 */
#define RESET_POLL_INTERVAL_MS 10

/* ===== 系统配置寄存器偏移定义 ===== */
#define SYSCFG_OFFSET                           0x0
#define SYSCFG_OFFSET_BUILD_STATUS              (SYSCFG_OFFSET + 0x0)
#define SYSCFG_OFFSET_SYSTEM_RESET              (SYSCFG_OFFSET + 0x4)
#define SYSCFG_OFFSET_SYSTEM_STATUS             (SYSCFG_OFFSET + 0x8)
#define SYSCFG_OFFSET_SHELL_RESET               (SYSCFG_OFFSET + 0xC)
#define SYSCFG_OFFSET_SHELL_STATUS              (SYSCFG_OFFSET + 0x10)
#define SYSCFG_OFFSET_USER_RESET                (SYSCFG_OFFSET + 0x14)
#define SYSCFG_OFFSET_USER_STATUS               (SYSCFG_OFFSET + 0x18)

/* ===== CMAC子系统寄存器偏移定义 ===== */
#define CMAC_SUBSYSTEM_0_OFFSET                 0x8000
#define CMAC_SUBSYSTEM_1_OFFSET                 0xC000
#define CMAC_SUBSYSTEM_OFFSET(i)                (((i) == 0) ? CMAC_SUBSYSTEM_0_OFFSET : CMAC_SUBSYSTEM_1_OFFSET)

#define CMAC_OFFSET(i)                          (CMAC_SUBSYSTEM_OFFSET(i) + 0x0)
#define CMAC_OFFSET_GT_RESET(i)                 (CMAC_OFFSET(i) + 0x0000)
#define CMAC_OFFSET_RESET(i)                    (CMAC_OFFSET(i) + 0x0004)
#define CMAC_OFFSET_STAT_STATUS_1(i)            (CMAC_OFFSET(i) + 0x0208)
#define CMAC_OFFSET_STAT_RX_STATUS(i)           (CMAC_OFFSET(i) + 0x0204)

/* ===== 复位类型枚举 ===== */
typedef enum {
    RESET_TYPE_SYSTEM = 0,          /*!< 系统复位 */
    RESET_TYPE_SHELL,               /*!< Shell层复位 */
    RESET_TYPE_USER,                /*!< 用户复位 */
    RESET_TYPE_CMAC_PORT0,          /*!< CMAC端口0复位 */
    RESET_TYPE_CMAC_PORT1,          /*!< CMAC端口1复位 */
    RESET_TYPE_CMAC_GT_PORT0,       /*!< CMAC GT端口0复位 */
    RESET_TYPE_CMAC_GT_PORT1,       /*!< CMAC GT端口1复位 */
    RESET_TYPE_MAX                  /*!< 复位类型数量 */
} reset_type_t;

/* ===== Shell层复位位掩码 ===== */
#define SHELL_RESET_CMAC_PORT0                  0x10
#define SHELL_RESET_CMAC_PORT1                  0x100

/*! \struct reset_config_t
 *  \brief 复位配置结构体
 */
typedef struct {
    char* device_name;              /*!< 字符设备名称 */
    char* pcie_resource;            /*!< PCIe资源文件路径 */
    reset_type_t reset_type;        /*!< 复位类型 */
    int force_reset;                /*!< 强制复位标志，跳过确认 */
    int verbose_mode;               /*!< 详细输出模式 */
    int interactive_mode;           /*!< 交互模式标志 */
    int status_only;                /*!< 仅查看状态，不执行复位 */
} reset_config_t;

/**
 * @brief 解析命令行整数参数（支持十进制和十六进制）
 * @param optarg 命令行参数字符串
 * @return 解析后的64位整数值
 */
uint64_t getopt_integer(char *optarg);

/**
 * @brief 初始化PCIe BAR空间映射
 * @param pcie_resource PCIe资源文件路径
 * @param pcie_fd PCIe文件描述符指针（输出参数）
 * @return 映射的虚拟地址，失败返回NULL
 */
void* init_pcie_bar_mapping(const char* pcie_resource, int* pcie_fd);

/**
 * @brief 清理PCIe BAR空间映射
 * @param mapped_addr 映射的虚拟地址
 * @param pcie_fd PCIe文件描述符
 */
void cleanup_pcie_bar_mapping(void* mapped_addr, int pcie_fd);

/**
 * @brief 安全的32位寄存器写操作
 * @param pcie_base PCIe基地址
 * @param offset 寄存器偏移地址
 * @param value 要写入的值
 * @return 0-成功，负数-失败
 */
int safe_write32_data(uint32_t* pcie_base, off_t offset, uint32_t value);

/**
 * @brief 安全的32位寄存器读操作
 * @param pcie_base PCIe基地址
 * @param offset 寄存器偏移地址
 * @param value 读取值的存储指针（输出参数）
 * @return 0-成功，负数-失败
 */
int safe_read32_data(uint32_t* pcie_base, off_t offset, uint32_t* value);

/**
 * @brief 获取复位类型名称
 * @param reset_type 复位类型
 * @return 复位类型名称字符串
 */
const char* get_reset_type_name(reset_type_t reset_type);

/**
 * @brief 获取复位类型描述
 * @param reset_type 复位类型
 * @return 复位类型描述字符串
 */
const char* get_reset_type_description(reset_type_t reset_type);

/**
 * @brief 执行系统复位
 * @param pcie_base PCIe基地址
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_system_reset(uint32_t* pcie_base, int verbose);

/**
 * @brief 执行Shell层复位
 * @param pcie_base PCIe基地址
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_shell_reset(uint32_t* pcie_base, int verbose);

/**
 * @brief 执行用户复位
 * @param pcie_base PCIe基地址
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_user_reset(uint32_t* pcie_base, int verbose);

/**
 * @brief 执行CMAC端口复位
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID（0或1）
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_cmac_reset(uint32_t* pcie_base, int port_id, int verbose);

/**
 * @brief 执行CMAC GT复位
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID（0或1）
 * @param verbose 详细输出标志
 * @return 0-成功，负数-失败
 */
int perform_cmac_gt_reset(uint32_t* pcie_base, int port_id, int verbose);

/**
 * @brief 检查系统复位状态
 * @param pcie_base PCIe基地址
 * @param reset_completed 复位完成标志指针（输出参数）
 * @return 0-成功，负数-失败
 */
int check_system_reset_status(uint32_t* pcie_base, int* reset_completed);

/**
 * @brief 检查Shell层复位状态
 * @param pcie_base PCIe基地址
 * @param reset_completed 复位完成标志指针（输出参数）
 * @return 0-成功，负数-失败
 */
int check_shell_reset_status(uint32_t* pcie_base, int* reset_completed);

/**
 * @brief 检查用户复位状态
 * @param pcie_base PCIe基地址
 * @param reset_completed 复位完成标志指针（输出参数）
 * @return 0-成功，负数-失败
 */
int check_user_reset_status(uint32_t* pcie_base, int* reset_completed);

/**
 * @brief 检查CMAC复位状态
 * @param pcie_base PCIe基地址
 * @param port_id 端口ID（0或1）
 * @param reset_completed 复位完成标志指针（输出参数）
 * @return 0-成功，负数-失败
 */
int check_cmac_reset_status(uint32_t* pcie_base, int port_id, int* reset_completed);

/**
 * @brief 显示所有复位状态
 * @param pcie_base PCIe基地址
 * @return 0-成功，负数-失败
 */
int display_all_reset_status(uint32_t* pcie_base);

/**
 * @brief 等待复位完成
 * @param pcie_base PCIe基地址
 * @param reset_type 复位类型
 * @param timeout_ms 超时时间（毫秒）
 * @return 0-成功，负数-失败
 */
int wait_reset_completion(uint32_t* pcie_base, reset_type_t reset_type, int timeout_ms);

/**
 * @brief 确认复位操作
 * @param reset_type 复位类型
 * @return 1-确认，0-取消
 */
int confirm_reset_operation(reset_type_t reset_type);

/**
 * @brief 交互式复位模式
 * @param pcie_base PCIe基地址
 * @return 0-正常退出，负数-错误退出
 */
int interactive_reset_mode(uint32_t* pcie_base);

/**
 * @brief 打印复位帮助信息
 */
void print_reset_help(void);

#endif /* __RESET_UTILS_H__ */
