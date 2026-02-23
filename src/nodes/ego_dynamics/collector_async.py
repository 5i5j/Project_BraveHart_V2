import uuid
import time
import pandas as pd
import os
import queue
import threading
from ...drivers.serial_bridge_pico import SerialBridgePico

# 增大缓冲区，防止后台写入过慢
data_queue = queue.Queue(maxsize=100)

def writer_worker(columns):
    while True:
        chunk = data_queue.get()
        if chunk is None: break
        
        # 产生分段文件，避免合并旧文件带来的巨大 I/O
        filename = f"data/ego_seg_{int(time.time())}_{uuid.uuid4().hex[:4]}.parquet"
        try:
            df = pd.DataFrame(chunk, columns=columns)
            df.to_parquet(filename, engine='pyarrow', index=False)
        except Exception as e:
            print(f"Write Error: {e}")
        finally:
            data_queue.task_done()

def run_collector(port='/dev/ttyACM0', baud=115200):
    # 确保 data 目录存在
    if not os.path.exists('data'):
        os.makedirs('data')

    bridge = SerialBridgePico(port, baud)
    data_buffer = []
    columns = ['seq', 'pico_us', 'ax', 'ay', 'az', 'qi', 'qj', 'qk', 'qr', 'pi_ns']
    
    # 启动后台线程
    writer_thread = threading.Thread(target=writer_worker, args=(columns,), daemon=True)
    writer_thread.start()

    print(f"📡 High-Performance Collector (Pi 3B Optimized) started.")
    print("Using async partitioned writing. Files saved in 'data/' directory.")

    try:
        while True:
            packet = bridge.read_packet()
            if packet:
                # 尽可能快地打上本地时间戳
                arrival_time = time.time_ns()
                data_buffer.append(list(packet) + [arrival_time])
                print(".", end='', flush=True)

            # 每 1000 条数据提交一次后台写入
            if len(data_buffer) >= 1000:
                data_queue.put(list(data_buffer))
                data_buffer = []
                print(f"\n📦 Buffer committed to background writer.")

    except KeyboardInterrupt:
        if data_buffer:
            data_queue.put(list(data_buffer))
        print("\n🛑 Stopping collector...")
    finally:
        data_queue.put(None) # 关闭写入线程
        bridge.close()

if __name__ == "__main__":
    run_collector()