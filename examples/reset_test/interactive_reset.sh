#!/bin/bash
# 快速进入RecoNIC复位交互模式
export LD_LIBRARY_PATH=/workspace/lib:$LD_LIBRARY_PATH
if [ "$(id -u)" != "0" ]; then
    echo "需要root权限，尝试使用sudo..."
    sudo env LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./reset_test --interactive
else
    ./reset_test --interactive
fi
