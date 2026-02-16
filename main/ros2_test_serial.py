import serial
import time
import sys

# デバイス名の設定 (Jetsonの場合、通常は /dev/ttyACM0 または /dev/ttyUSB0)
PORT = '/dev/ttyACM0'
BAUD = 115200

def send_command(cmd):
    try:
        with serial.Serial(PORT, BAUD, timeout=1) as ser:
            print(f"Sending command: {cmd}")
            # コマンドの末尾に改行コードを付与して送信
            ser.write(f"{cmd}\n".encode('utf-8'))
            time.sleep(0.1)
            # レスポンスがあれば表示
            while ser.in_waiting:
                print(f"Response: {ser.readline().decode('utf-8').strip()}")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == '__main__':
    commands = ['ready', 'start_seq', 'move', 'grip', 'alert', 'finish']
    
    if len(sys.argv) > 1:
        cmd = sys.argv[1]
        if cmd in commands:
            send_command(cmd)
        else:
            print(f"Invalid command. Available: {', '.join(commands)}")
    else:
        print("Usage: python3 ros2_test_serial.py <command>")
        print(f"Available commands: {', '.join(commands)}")
        
        # デフォルトで ready を実行
        print("\nTesting 'ready' command...")
        send_command('ready')
