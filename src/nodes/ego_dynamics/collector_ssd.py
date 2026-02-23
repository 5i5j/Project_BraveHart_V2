import time
import pandas as pd
import os
import queue
import threading
import uuid
from ...drivers.serial_bridge_pico import SerialBridgePico

# 针对 Pi 4B 调大缓冲区
data_queue = queue.Queue(maxsize=200)

# 修改为你的 SSD 挂载路径
BASE_DATA_PATH = "/mnt/ssd_data/Project_BraveHart_V2/data"

def writer_worker(columns):
    """后台写入线程：利用 SSD 的高 I/O 能力"""
    while True:
        chunk = data_queue.get()
        if chunk is None: break
        
        # 使用更清晰的时间戳命名，SSD 处理这些文件非常快
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        unique_id = uuid.uuid4().hex[:6]
        filename = os.path.join(BASE_DATA_PATH, f"ego_log_{timestamp}_{unique_id}.parquet")
        
        try:
            df = pd.DataFrame(chunk, columns=columns)
            # SSD 环境下，使用 pyarrow 的写入几乎是瞬间完成的
            df.to_parquet(filename, engine='pyarrow', index=False)
        except Exception as e:
            print(f"\n[Disk Error] Failed to write to SSD: {e}")
        finally:
            data_queue.task_done()

def run_collector(port='/dev/ttyACM0', baud=115200):
    # 自动创建 SSD 上的数据目录
    if not os.path.exists(BASE_DATA_PATH):
        try:
            os.makedirs(BASE_DATA_PATH, exist_ok=True)
            print(f"📁 Created SSD data directory: {BASE_DATA_PATH}")
        except Exception as e:
            print(f"❌ Could not create directory on SSD: {e}")
            return

    bridge = SerialBridgePico(port, baud)
    data_buffer = []
    columns = ['seq', 'pico_us', 'ax', 'ay', 'az', 'qi', 'qj', 'qk', 'qr', 'pi_ns']
    
    writer_thread = threading.Thread(target=writer_worker, args=(columns,), daemon=True)
    writer_thread.start()

    print(f"📡 Pi 4B SSD-Accelerated Collector started.")
    print(f"💾 Storage Path: {BASE_DATA_PATH}")

    try:
        while True:
            packet = bridge.read_packet()
            if packet:
                arrival_time = time.time_ns()
                data_buffer.append(list(packet) + [arrival_time])
                # Pi 4B 下可以稍微减少 print 频率，或保持现状
                # print(".", end='', flush=True)

            # Pi 4B + SSD 性能强劲，可以每 2000 条（40秒）存一次，减少文件数
            if len(data_buffer) >= 2000:
                data_queue.put(list(data_buffer))
                data_buffer = []
                print(f"\n🚀 {time.strftime('%H:%M:%S')} - 2000 samples pushed to SSD.")

    except KeyboardInterrupt:
        if data_buffer:
            data_queue.put(list(data_buffer))
        print("\n🛑 Collector stopped by user.")
    finally:
        data_queue.put(None)
        bridge.close()

if __name__ == "__main__":
    run_collector()