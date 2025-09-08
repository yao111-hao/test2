//==============================================================================
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//
//==============================================================================

#include "register_utils.h"

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
 * @param pcie_axil_base PCIe AXIL基地址
 * @param offset 寄存器偏移地址
 * @param value 要写入的值
 * @return 0-成功，负数-失败
 */
int safe_write32_data(uint32_t* pcie_axil_base, off_t offset, uint32_t value)
{
    uint32_t* config_addr;

    if (!pcie_axil_base) {
        fprintf(stderr, "错误：PCIe基地址为空\n");
        return -EINVAL;
    }

    if (!is_valid_register_offset(offset)) {
        fprintf(stderr, "错误：寄存器偏移地址0x%lx无效或不安全\n", offset);
        return -EINVAL;
    }

    config_addr = (uint32_t*)((uintptr_t)pcie_axil_base + offset);
    *(config_addr) = value;
    
    /* 强制刷新写缓存 */
    __sync_synchronize();
    
    return 0;
}

/**
 * @brief 安全的32位寄存器读操作
 * @param pcie_axil_base PCIe AXIL基地址
 * @param offset 寄存器偏移地址
 * @param value 读取值的存储指针（输出参数）
 * @return 0-成功，负数-失败
 */
int safe_read32_data(uint32_t* pcie_axil_base, off_t offset, uint32_t* value)
{
    uint32_t* config_addr;

    if (!pcie_axil_base || !value) {
        fprintf(stderr, "错误：参数不能为空\n");
        return -EINVAL;
    }

    if (!is_valid_register_offset(offset)) {
        fprintf(stderr, "错误：寄存器偏移地址0x%lx无效或不安全\n", offset);
        return -EINVAL;
    }

    config_addr = (uint32_t*)((uintptr_t)pcie_axil_base + offset);
    *value = *((uint32_t*)config_addr);
    
    return 0;
}

/**
 * @brief 验证寄存器偏移地址的有效性
 * @param offset 寄存器偏移地址
 * @return 1-有效，0-无效
 */
int is_valid_register_offset(uint32_t offset)
{
    /* 检查偏移是否超出BAR空间范围 */
    if (offset >= REG_MAP_SIZE) {
        return 0;
    }

    /* 检查地址是否4字节对齐 */
    if (offset & 0x3) {
        return 0;
    }

    return 1;
}

/**
 * @brief 获取寄存器名称描述（如果已知）
 * @param offset 寄存器偏移地址
 * @return 寄存器名称字符串，未知返回"Unknown"
 */
const char* get_register_name(uint32_t offset)
{
    /* RecoNIC已知寄存器定义 */
    switch (offset) {
        case 0x102000: return "RN_SCR_VERSION（版本寄存器）";
        case 0x102004: return "RN_SCR_FATAL_ERR（致命错误寄存器）";
        case 0x102008: return "RN_SCR_TRMHR_REG（传输高位寄存器）";
        case 0x10200C: return "RN_SCR_TRMLR_REG（传输低位寄存器）";
        case 0x103000: return "RN_CLR_CTL_CMD（计算控制命令寄存器）";
        case 0x103004: return "RN_CLR_KER_STS（内核状态寄存器）";
        case 0x103008: return "RN_CLR_JOB_SUBMITTED（作业提交寄存器）";
        case 0x10300C: return "RN_CLR_JOB_COMPLETED_NOT_READ（作业完成未读寄存器）";
        case 0x060000: return "RN_RDMA_GCSR_XRNICCONF（RDMA全局配置寄存器）";
        case 0x060004: return "RN_RDMA_GCSR_XRNICADCONF（RDMA高级配置寄存器）";
        case 0x060010: return "RN_RDMA_GCSR_MACXADDLSB（MAC地址低32位寄存器）";
        case 0x060014: return "RN_RDMA_GCSR_MACXADDMSB（MAC地址高32位寄存器）";
        case 0x060070: return "RN_RDMA_GCSR_IPV4XADD（IPv4地址寄存器）";
        case 0x060100: return "RN_RDMA_GCSR_INSRRPKTCNT（接收包计数寄存器）";
        case 0x060104: return "RN_RDMA_GCSR_INAMPKTCNT（输入包计数寄存器）";
        case 0x060108: return "RN_RDMA_GCSR_OUTIOPKTCNT（输出IO包计数寄存器）";
        case 0x016420: return "AXIB_BDF_ADDR_TRANSLATE_ADDR_LSB（BDF地址转换低位）";
        case 0x016424: return "AXIB_BDF_ADDR_TRANSLATE_ADDR_MSB（BDF地址转换高位）";
        case 0x016428: return "AXIB_BDF_PASID_RESERVED_ADDR（BDF PASID保留地址）";
        case 0x01642C: return "AXIB_BDF_FUNCTION_NUM_ADDR（BDF功能号地址）";
        case 0x016430: return "AXIB_BDF_MAP_CONTROL_ADDR（BDF映射控制地址）";
        default: return "未知寄存器";
    }
}

/**
 * @brief 打印寄存器访问帮助信息
 */
void print_register_help(void)
{
    printf("\n=== RecoNIC寄存器访问帮助 ===\n\n");
    printf("常用寄存器偏移地址：\n");
    printf("  0x102000 - RN_SCR_VERSION（版本寄存器）\n");
    printf("  0x102004 - RN_SCR_FATAL_ERR（致命错误寄存器）\n");
    printf("  0x103000 - RN_CLR_CTL_CMD（计算控制命令寄存器）\n");
    printf("  0x103004 - RN_CLR_KER_STS（内核状态寄存器）\n");
    printf("  0x060000 - RN_RDMA_GCSR_XRNICCONF（RDMA全局配置寄存器）\n");
    printf("  0x060010 - RN_RDMA_GCSR_MACXADDLSB（MAC地址低32位寄存器）\n");
    printf("  0x060014 - RN_RDMA_GCSR_MACXADDMSB（MAC地址高32位寄存器）\n");
    printf("  0x060070 - RN_RDMA_GCSR_IPV4XADD（IPv4地址寄存器）\n");
    printf("  0x016420 - AXIB_BDF_ADDR_TRANSLATE_ADDR_LSB（BDF地址转换低位）\n");
    printf("  0x016430 - AXIB_BDF_MAP_CONTROL_ADDR（BDF映射控制地址）\n");
    printf("\n注意事项：\n");
    printf("  - 地址必须4字节对齐\n");
    printf("  - 地址范围：0x000000 - 0x3FFFFF\n");
    printf("  - 支持十进制和十六进制输入（如：102000 或 0x102000）\n");
    printf("  - 某些寄存器为只读，写入可能无效或危险\n\n");
}

/**
 * @brief 交互式寄存器访问模式
 * @param pcie_axil_base PCIe AXIL基地址
 * @return 0-正常退出，负数-错误退出
 */
int interactive_register_access(uint32_t* pcie_axil_base)
{
    char input[256];
    char operation;
    uint32_t offset, value;
    int ret;

    printf("\n=== 进入交互式寄存器访问模式 ===\n");
    printf("命令格式：\n");
    printf("  r <offset>        - 读取寄存器（十进制或0x开头的十六进制）\n");
    printf("  w <offset> <value> - 写入寄存器\n");
    printf("  h                 - 显示帮助信息\n");
    printf("  q                 - 退出交互模式\n\n");

    while (1) {
        printf("register> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break;
        }

        /* 移除换行符 */
        input[strcspn(input, "\n")] = 0;

        /* 跳过空行 */
        if (strlen(input) == 0) {
            continue;
        }

        operation = input[0];

        switch (operation) {
            case 'r':
            case 'R':
                if (sscanf(input, "%c 0x%x", &operation, &offset) == 2 ||
                    sscanf(input, "%c %u", &operation, &offset) == 2) {
                    ret = safe_read32_data(pcie_axil_base, offset, &value);
                    if (ret == 0) {
                        printf("寄存器[0x%06X] = 0x%08X (%u) - %s\n", 
                               offset, value, value, get_register_name(offset));
                    } else {
                        printf("读取失败\n");
                    }
                } else {
                    printf("格式错误：r <offset>\n");
                }
                break;

            case 'w':
            case 'W':
                if (sscanf(input, "%c 0x%x 0x%x", &operation, &offset, &value) == 3 ||
                    sscanf(input, "%c %u %u", &operation, &offset, &value) == 3 ||
                    sscanf(input, "%c 0x%x %u", &operation, &offset, &value) == 3 ||
                    sscanf(input, "%c %u 0x%x", &operation, &offset, &value) == 3) {
                    
                    printf("警告：即将写入寄存器[0x%06X] = 0x%08X - %s\n", 
                           offset, value, get_register_name(offset));
                    printf("确认写入吗？(y/N): ");
                    fflush(stdout);
                    
                    char confirm[10];
                    if (fgets(confirm, sizeof(confirm), stdin) && 
                        (confirm[0] == 'y' || confirm[0] == 'Y')) {
                        
                        ret = safe_write32_data(pcie_axil_base, offset, value);
                        if (ret == 0) {
                            printf("寄存器写入成功\n");
                            /* 读回验证 */
                            uint32_t read_back_value;
                            if (safe_read32_data(pcie_axil_base, offset, &read_back_value) == 0) {
                                printf("验证：寄存器[0x%06X] = 0x%08X\n", offset, read_back_value);
                            }
                        } else {
                            printf("写入失败\n");
                        }
                    } else {
                        printf("取消写入操作\n");
                    }
                } else {
                    printf("格式错误：w <offset> <value>\n");
                }
                break;

            case 'h':
            case 'H':
                print_register_help();
                break;

            case 'q':
            case 'Q':
                printf("退出交互模式\n");
                return 0;

            default:
                printf("未知命令：%c\n", operation);
                printf("使用 'h' 查看帮助信息\n");
                break;
        }
    }

    return 0;
}
