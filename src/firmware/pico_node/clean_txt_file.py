import re
import pandas as pd

def parse_jitter_log(input_file, output_file):
    data = []
    current_freq = None
    capture_mode = False
    
    # 正则表达式匹配头部：---DATA_START:10HZ---
    start_pattern = re.compile(r"---DATA_START:(\d+)HZ---")
    
    with open(input_file, 'r', encoding='utf-8') as f:
        lines = f.readlines()
        
    for line in lines:
        line = line.strip()
        
        # 检查是否开始新的数据块
        start_match = start_match = start_pattern.match(line)
        if start_match:
            current_freq = int(start_match.group(1))
            capture_mode = True
            continue
            
        # 检查是否结束数据块
        if "---SUMMARY" in line or "---DATA_END---" in line:
            capture_mode = False
            current_freq = None
            continue
            
        # 采集数据
        if capture_mode:
            # 跳过 CSV 标题行
            if "index,jitter_us" in line:
                continue
            
            # 解析 "index,jitter"
            try:
                parts = line.split(',')
                if len(parts) == 2:
                    idx = int(parts[0])
                    jitter = int(parts[1])
                    data.append({
                        'Frequency_Hz': current_freq,
                        'Sample_Index': idx,
                        'Jitter_us': jitter
                    })
            except ValueError:
                continue

    # 转换为 DataFrame 并保存
    df = pd.DataFrame(data)
    df.to_csv(output_file, index=False)
    print(f"处理完成！数据已保存至 {output_file}")
    print(f"共提取了 {len(df)} 个样本，涵盖频率: {df['Frequency_Hz'].unique()}")

# 使用方法
if __name__ == "__main__":
    # 假设你的日志文件名为 Jitter_Testing_Results.txt
    parse_jitter_log('Jitter_Testing_Results.txt', 'jitter_cleaned.csv')