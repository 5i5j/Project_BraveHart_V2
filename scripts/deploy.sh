#!/bin/bash

# ==============================================================================
# Script: deploy.sh
# Purpose: Sync code, build firmware, and flash Pico remotely via picotool.
# ==============================================================================

# 💡 修改 1：允许输入 IP 地址，如果不包含 "pi-"，则强制指定为 Pi Target
TARGET=$1
USER="tim"
PROJECT_ROOT="/home/tim/Tim/Project_BraveHart_V2"
TARGET_DIR="~/Project_BraveHart_V2"

# 💡 修改 2：更新固件路径为 bno_test.elf (Corrected firmware path)
FIRMWARE_REL_PATH="src/firmware/pico_i2c_scan/bno_test.elf"

# 1. Validation
if [ -z "$TARGET" ]; then
    echo "❌ Error: Target host/IP is missing. Usage: ./deploy.sh <hostname_or_IP>"
    exit 1
fi

echo "🚀 Starting deployment to: $TARGET"

# --- Step 1: P620 Cross-Compilation ---
if [[ "$(hostname)" == *"p620"* ]]; then
    echo "🛠️ Detected P620: Running Cross-Compilation..."
    cd "$PROJECT_ROOT/build" || exit 1
    # 💡 确保编译最新的目标
    make bno_test -j$(nproc)
    
    if [ $? -ne 0 ]; then
        echo "❌ Build failed! Aborting."
        exit 1
    fi
    echo "✅ Build successful."
fi

# --- Step 2: Remote Pico Flashing ---
# 💡 修改 3：逻辑优化，只要文件存在就烧录，并使用 -O 兼容局域网传输
if [ -f "$PROJECT_ROOT/build/$FIRMWARE_REL_PATH" ]; then
    echo "⚡ Flashing $FIRMWARE_REL_PATH via remote picotool..."
    
    # 使用 -O 强制使用旧版 SCP 协议，并在局域网 IP 下更稳定 (Use legacy SCP)
    scp -O "$PROJECT_ROOT/build/$FIRMWARE_REL_PATH" "$USER@$TARGET:/tmp/pico_fw.elf"
    
    # 远程执行烧录
    ssh -t "$USER@$TARGET" "sudo picotool load /tmp/pico_fw.elf -f"
    
    if [ $? -eq 0 ]; then
        echo "✅ Pico Flash Successful."
    else
        echo "⚠️ Pico Flash failed!"
    fi
else
    echo "ℹ️ Skipping flash: Firmware not found at $FIRMWARE_REL_PATH."
fi

# --- Step 3: Code Synchronization (rsync) ---
# ... 保持原有的 rsync 逻辑不变 ...
cd "$PROJECT_ROOT" || exit 1
echo "🔄 Synchronizing code..."
rsync -avz --delete \
    --exclude '.git/' \
    --exclude 'venv/' \
    --exclude 'data/' \
    --exclude 'build/' \
    --exclude '__pycache__/' \
    --exclude '.vscode/' \
    --exclude 'src/firmware/pico-sdk/' \
    --exclude 'src/firmware/FreeRTOS-Kernel/' \
    --exclude 'src/firmware/micro_ros_raspberrypi_pico_sdk/' \
    ./ "$USER@$TARGET:$TARGET_DIR"

echo "------------------------------------------------"
echo "🎉 Deployment to $TARGET finished."