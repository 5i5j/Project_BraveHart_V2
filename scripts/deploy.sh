#!/bin/bash

# ==============================================================================
# Script: deploy.sh
# Purpose: Synchronize Project_BraveHart_V2 to remote edge nodes (Pi 4B / Pi 3B).
# Usage: ./scripts/deploy.sh [target_hostname_or_ip]
# Example: ./scripts/deploy.sh tim-pi-4b
# ==============================================================================

TARGET=$1
USER="tim"
PROJECT_ROOT="/home/tim/Tim/Project_BraveHart_V2"
TARGET_DIR="~/Project_BraveHart_V2"

# 1. Validation (保持不变)
if [ -z "$TARGET" ]; then
    echo "❌ Error: Target host/IP is missing."
    exit 1
fi

echo "🚀 Starting deployment to: $TARGET"

# --- 🆕 Step 1.5: P620 交叉编译 (仅在 P620 上运行) ---
if [[ "$(hostname)" == *"P620"* ]]; then
    echo "🛠️ Detected P620: Running Cross-Compilation for Pico..."
    cd $PROJECT_ROOT/build
    make -j$(nproc)
    
    if [ $? -ne 0 ]; then
        echo "❌ Build failed! Aborting sync."
        exit 1
    fi
    echo "✅ Build successful."
fi

# 2. Code Synchronization (在你的基础上增加了固件目录同步)
cd $PROJECT_ROOT
rsync -avz --delete \
    --exclude '.git/' \
    --exclude 'venv/' \
    --exclude 'data/' \
    --exclude 'build/' \
    --exclude '__pycache__/' \
    --exclude '.vscode/' \
    ./ $USER@$TARGET:$TARGET_DIR

# 3. Post-Deployment Check
if [ $? -eq 0 ]; then
    echo "✅ Sync successful! Location: $TARGET:$TARGET_DIR"
    
    # Optional: Display SSD storage status on Pi 4B
    if [[ "$TARGET" == *"pi-4b"* ]]; then
        echo "📊 SSD Storage Status on $TARGET:"
        ssh $USER@$TARGET "df -h /mnt/ssd_data/ | grep /dev/" || echo "⚠️ SSD not mounted!"
    fi
else
    echo "❌ Sync failed. Please check your Tailscale connection and SSH keys."
    exit 1
fi

echo "------------------------------------------------"
echo "🎉 Deployment to $TARGET finished."