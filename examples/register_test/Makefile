#==============================================================================
# Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT
#
#==============================================================================
#
# RecoNIC寄存器测试程序Makefile
# 支持x86_64和ARM64架构
#
#==============================================================================

# 编译器和基本标志
CC ?= gcc
ARCH := $(shell uname -m)

# 基本编译标志
CFLAGS = -g -Wall -Wextra -std=gnu99 -fPIC

# 架构特定优化
ifeq ($(ARCH),x86_64)
    CFLAGS += -march=native -mtune=native
    ARCH_DEFINES = -D__X86_64__
else ifeq ($(ARCH),aarch64)
    CFLAGS += -mcpu=native -mtune=native
    ARCH_DEFINES = -D__AARCH64__
else ifneq (,$(findstring arm,$(ARCH)))
    CFLAGS += -mcpu=native -mtune=native  
    ARCH_DEFINES = -D__ARM__
else
    $(warning 未知架构: $(ARCH), 使用默认设置)
    ARCH_DEFINES = -D__UNKNOWN_ARCH__
endif

# 添加架构定义
CFLAGS += $(ARCH_DEFINES)

# 包含路径
INCLUDES = -I. -I../../lib

# 库路径和链接库
LIBPATH = -L../../lib
LIBS = -lreconic

# 源文件和目标文件
EXECUTABLE = register_test
SOURCES = register_utils.c register_test.c
OBJECTS = $(SOURCES:.c=.o)

# 依赖检查
LIBRECONIC_PATH = ../../lib/libreconic.a

# 默认目标
all: check_deps $(EXECUTABLE)

# 检查依赖库
check_deps:
	@echo "检查架构: $(ARCH)"
	@echo "编译标志: $(CFLAGS)"
	@if [ ! -f "$(LIBRECONIC_PATH)" ]; then \
		echo "警告: libreconic.a 不存在，正在编译..."; \
		$(MAKE) -C ../../lib || (echo "错误: 无法编译libreconic库" && exit 1); \
	fi
	@echo "依赖检查完成"

# 生成可执行文件
$(EXECUTABLE): $(OBJECTS)
	@echo "链接 $(EXECUTABLE)..."
	$(CC) $(OBJECTS) -o $@ $(LIBPATH) $(LIBS)
	@echo "编译完成: $(EXECUTABLE)"
	@echo "架构: $(ARCH)"
	@file $(EXECUTABLE) 2>/dev/null || true

# 编译对象文件
%.o: %.c
	@echo "编译 $<..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 清理目标
clean:
	@echo "清理编译文件..."
	rm -f $(OBJECTS) $(EXECUTABLE)
	@echo "清理完成"

# 安装目标（可选）
install: $(EXECUTABLE)
	@echo "安装 $(EXECUTABLE) 到 /usr/local/bin/"
	sudo cp $(EXECUTABLE) /usr/local/bin/
	sudo chmod 755 /usr/local/bin/$(EXECUTABLE)
	@echo "安装完成"

# 卸载目标
uninstall:
	@echo "卸载 $(EXECUTABLE)..."
	sudo rm -f /usr/local/bin/$(EXECUTABLE)
	@echo "卸载完成"

# 测试目标
test: $(EXECUTABLE)
	@echo "运行基本测试..."
	@if [ "$(shell id -u)" != "0" ]; then \
		echo "测试需要root权限，请使用: sudo make test"; \
		exit 1; \
	fi
	@echo "显示帮助信息:"
	./$(EXECUTABLE) -h
	@echo ""
	@echo "测试完成。要进行实际硬件测试，请确保："
	@echo "1. RecoNIC硬件已正确安装"
	@echo "2. onic驱动已加载"
	@echo "3. PCIe设备可访问"

# ARM架构特殊处理
arm-optimize: CFLAGS += -O2 -DNDEBUG
arm-optimize: clean all
	@echo "ARM优化版本编译完成"

# 调试版本
debug: CFLAGS += -DDEBUG -O0 -ggdb3
debug: clean all
	@echo "调试版本编译完成"

# 发布版本
release: CFLAGS += -O2 -DNDEBUG -s
release: clean all
	@echo "发布版本编译完成"
	@strip $(EXECUTABLE) 2>/dev/null || true

# 显示帮助
help:
	@echo "RecoNIC寄存器测试程序编译帮助"
	@echo ""
	@echo "可用目标:"
	@echo "  all          - 编译程序（默认）"
	@echo "  clean        - 清理编译文件"
	@echo "  install      - 安装到系统路径"
	@echo "  uninstall    - 从系统路径卸载"
	@echo "  test         - 运行基本测试（需要root权限）"
	@echo "  debug        - 编译调试版本"
	@echo "  release      - 编译发布版本"
	@echo "  arm-optimize - ARM架构优化编译"
	@echo "  help         - 显示此帮助信息"
	@echo ""
	@echo "使用示例:"
	@echo "  make                    # 编译程序"
	@echo "  make clean              # 清理编译文件"
	@echo "  make debug              # 编译调试版本"
	@echo "  sudo make install       # 安装到系统"
	@echo "  sudo make test          # 运行测试"

# 声明伪目标
.PHONY: all clean install uninstall test debug release arm-optimize help check_deps
