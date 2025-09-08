//==============================================================================
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//
//==============================================================================

#ifndef __REGISTER_UTILS_H__
#define __REGISTER_UTILS_H__

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
 *  \brief PCIe BAR2映射大小：4MB
 */
#define REG_MAP_SIZE 0x00400000

/*! \def DEFAULT_DEVICE
 *  \brief 默认设备路径
 */
#define DEFAULT_DEVICE "/dev/reconic-mm"

/*! \def DEFAULT_PCIE_RESOURCE
 *  \brief 默认PCIe资源路径
 */
#define DEFAULT_PCIE_RESOURCE "/sys/bus/pci/devices/0000:d8:00.0/resource2"

/*! \struct register_access_t
 *  \brief 寄存器访问配置结构体
 */
typedef struct {
    char* device_name;          /*!< 字符设备名称 */
    char* pcie_resource;        /*!< PCIe资源文件路径 */
    uint32_t register_offset;   /*!< 寄存器偏移地址 */
    uint32_t register_value;    /*!< 寄存器值 */
    int is_write_operation;     /*!< 写操作标志：1-写，0-读 */
    int verbose_mode;           /*!< 详细输出模式 */
    int interactive_mode;       /*!< 交互模式标志 */
} register_access_t;

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
 * @param pcie_axil_base PCIe AXIL基地址
 * @param offset 寄存器偏移地址
 * @param value 要写入的值
 * @return 0-成功，负数-失败
 */
int safe_write32_data(uint32_t* pcie_axil_base, off_t offset, uint32_t value);

/**
 * @brief 安全的32位寄存器读操作
 * @param pcie_axil_base PCIe AXIL基地址
 * @param offset 寄存器偏移地址
 * @param value 读取值的存储指针（输出参数）
 * @return 0-成功，负数-失败
 */
int safe_read32_data(uint32_t* pcie_axil_base, off_t offset, uint32_t* value);

/**
 * @brief 验证寄存器偏移地址的有效性
 * @param offset 寄存器偏移地址
 * @return 1-有效，0-无效
 */
int is_valid_register_offset(uint32_t offset);

/**
 * @brief 获取寄存器名称描述（如果已知）
 * @param offset 寄存器偏移地址
 * @return 寄存器名称字符串，未知返回"Unknown"
 */
const char* get_register_name(uint32_t offset);

/**
 * @brief 打印寄存器访问帮助信息
 */
void print_register_help(void);

/**
 * @brief 交互式寄存器访问模式
 * @param pcie_axil_base PCIe AXIL基地址
 * @return 0-正常退出，负数-错误退出
 */
int interactive_register_access(uint32_t* pcie_axil_base);

#endif /* __REGISTER_UTILS_H__ */