import pandas as pd
import numpy as np
import glob
import os

def analyze_jitter(data_dir='data'):
    # 1. 扫描所有分段文件
    file_pattern = os.path.join(data_dir, 'ego_*.parquet')
    files = glob.glob(file_pattern)
    
    if not files:
        # 兼容旧逻辑：如果没有 data 目录，尝试读取单文件
        if os.path.exists('ego_dynamics.parquet'):
            files = ['ego_dynamics.parquet']
        else:
            print(f"❌ No data files found in '{data_dir}' or root.")
            return

    print(f"📂 Loading {len(files)} data segments...")
    
    # 2. 读取并合并数据
    try:
        df_list = [pd.read_parquet(f, engine='pyarrow') for f in files]
        df = pd.concat(df_list, ignore_index=True)
        
        # 必须按本地时间戳排序，确保 diff() 计算的是相邻包
        df = df.sort_values(by='pi_ns').reset_index(drop=True)
    except Exception as e:
        print(f"❌ Error loading parquet: {e}")
        return

    # 3. 计算时间差 (Microseconds)
    df['pico_diff'] = df['pico_us'].diff()
    
    # 4. 过滤回环 (Rollover) 和 异常值
    # 我们期望是 20000us，过滤掉小于0（回环）和大于 100ms 的极端跳变
    mask = (df['pico_diff'] > 0) & (df['pico_diff'] < 100000)
    valid_diffs = df.loc[mask, 'pico_diff']

    print(f"📊 Jitter Analysis Report ({len(df)} total samples)")
    print("-" * 40)

    if len(valid_diffs) == 0:
        print("No valid intervals found. Check data integrity.")
        return

    # 5. 统计分析
    avg_us = valid_diffs.mean()
    std_us = valid_diffs.std()
    max_us = valid_diffs.max()
    min_us = valid_diffs.min()
    actual_hz = 1_000_000 / avg_us

    print(f"Target Interval:   20000.00 us (50.00 Hz)")
    print(f"Average Interval:  {avg_us:.2f} us ({actual_hz:.2f} Hz)")
    print(f"Standard Dev:      {std_us:.2f} us")
    print(f"Jitter Range:      [{min_us:.0f}, {max_us:.0f}] us")
    
    # 6. Pi 接收端分析 (Jitter from Pi OS side)
    df['pi_diff_us'] = df['pi_ns'].diff() / 1000
    # 同样只分析有效样本
    valid_pi_diffs = df.loc[mask, 'pi_diff_us']
    
    print("-" * 40)
    print(f"Pi Reception StdDev: {valid_pi_diffs.std():.2f} us")

    # 7. 最终结论
    if std_us < 800:
        print("\n✅ RESULT: EXCELLENT. High precision timing.")
    elif std_us < 2000:
        print("\n⚠️ RESULT: ACCEPTABLE. Minor jitter detected.")
    else:
        print("\n❌ RESULT: UNSTABLE. System load too high.")

if __name__ == "__main__":
    analyze_jitter()