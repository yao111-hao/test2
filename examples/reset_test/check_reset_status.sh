#!/bin/bash
# 快速检查RecoNIC复位状态
export LD_LIBRARY_PATH=/workspace/lib:$LD_LIBRARY_PATH
if [ "$(id -u)" != "0" ]; then
    echo "需要root权限，尝试使用sudo..."
    sudo env LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./reset_test --status --verbose
else
    ./reset_test --status --verbose
fi

