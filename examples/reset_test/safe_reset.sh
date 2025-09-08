#!/bin/bash
# 安全的逐步复位脚本
export LD_LIBRARY_PATH=/workspace/lib:$LD_LIBRARY_PATH

echo "=== RecoNIC安全复位向导 ==="
echo "此脚本将按影响程度递增顺序提供复位选项"
echo ""

if [ "$(id -u)" != "0" ]; then
    echo "错误：需要root权限"
    echo "请使用：sudo $0"
    exit 1
fi

echo "1. 查看当前复位状态"
./reset_test --status

echo ""
echo "选择复位类型："
echo "1) GT复位（影响最小）"
echo "2) CMAC端口复位（影响中等）"
echo "3) 用户逻辑复位（影响中等）"
echo "4) Shell层复位（影响较大）"
echo "5) 系统复位（影响最大）"
echo "0) 退出"
echo ""

read -p "请选择 (0-5): " choice

case $choice in
    1)
        read -p "选择GT端口 (0/1): " port
        ./reset_test --cmac-gt $port
        ;;
    2)
        read -p "选择CMAC端口 (0/1): " port
        ./reset_test --cmac-port $port
        ;;
    3)
        ./reset_test --user
        ;;
    4)
        ./reset_test --shell
        ;;
    5)
        ./reset_test --system
        ;;
    0)
        echo "退出"
        ;;
    *)
        echo "无效选择"
        ;;
esac

